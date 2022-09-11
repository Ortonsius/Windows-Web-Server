DWORD WINAPI CreateWorker(LPVOID p);

class HTTPServer : public Root{
    private:
        SOCKET s;
        HANDLE ghIOCP;
        HANDLE Threads[1024];
        int TotalThread;
        bool WorkerStatus;
        CRITICAL_SECTION cs;
        std::map<std::string,HMODULE> gLIB;

        void LogRequest(char* ip,WebRequest *r){
            time_t t = time(NULL);
            struct tm tm;
            localtime_s(&tm,&t);
            EnterCriticalSection(&this->cs);
            std::ofstream f("request.log",std::ios_base::app);
            f << Root::FixTime(std::to_string(tm.tm_mday)) << "-" << Root::FixTime(std::to_string(tm.tm_mon+1)) << "-" << std::to_string(tm.tm_year+1900) << " " << Root::FixTime(std::to_string(tm.tm_hour)) << ":" << Root::FixTime(std::to_string(tm.tm_min)) << ":" << Root::FixTime(std::to_string(tm.tm_sec)) << " | " << ip << " => " << r->Method << " " << r->URL << '\n';
            f.close();
            LeaveCriticalSection(&this->cs);
        }

        void LogError(std::string msg){
            time_t t = time(NULL);
            struct tm tm;
            localtime_s(&tm,&t);
            // SET CRITICAL SECTION
            EnterCriticalSection(&this->cs);
            std::ofstream f("error.log",std::ios_base::app);
            f << Root::FixTime(std::to_string(tm.tm_mday)) << "-" << Root::FixTime(std::to_string(tm.tm_mon+1)) << "-" << std::to_string(tm.tm_year+1900) << " " << Root::FixTime(std::to_string(tm.tm_hour)) << ":" << Root::FixTime(std::to_string(tm.tm_min)) << ":" << Root::FixTime(std::to_string(tm.tm_sec)) << " | " << msg << '\n';
            f.close();
            LeaveCriticalSection(&this->cs);
            // END CRITICAL SECTION
        }

        std::string* TextareaParser(std::string text,std::map<std::string,void*> g_RET,json sch){
            int pos = 0;
            std::string *res = new std::string();
            while(true){
                pos = text.find("$(",pos);
                if(pos == std::string::npos) break;

                if(text[pos - 1] == '\\'){
                    pos++;
                    continue;
                }

                int ind = -1;
                ind = text.find(")",pos);
                if(ind == std::string::npos) break;

                std::string cmdid = text.substr(pos + 2,(ind - pos) - 2);
                if(!sch.contains(cmdid)){
                    // ERROR
                    if(this->Loge){
                        std::string _err = "Undefine command id '" + cmdid + "'";
                        this->LogError(_err);
                    }
                    return res;
                }
                    
                if(sch[cmdid]["ret"] != "str"){
                    // ERROR
                    if(this->Loge){
                        std::string _err = "Invalid data type '" + cmdid + "'. Data type must be 'str'.";
                        this->LogError(_err);
                    }
                    return res;
                }

                std::string *gres = (std::string*)g_RET[cmdid];
                
                text.replace(pos,(ind - pos) + 1,*gres);
                pos++;
            }

            *res += text;
            return res;
        }

    public:
        void PreInterpret(char* ip,bool* go_to_proc){
            json ev = this->Events["pre-process"];
            std::string currentId = ev["start"].get<std::string>();
            std::map<std::string,void*> g_RET;

            while(true){
                if(currentId == "END"){
                    for(auto i : g_RET) delete (char*)i.second;
                    break;
                }

                if(ev["main"].contains(currentId)){
                    json fnID = ev["main"][currentId];
                    std::string fn = fnID["fn"].get<std::string>();
                    int fndelim = fn.find("/");
                    std::string lib = fn.substr(0,fndelim);
                    std::string fnname = fn.substr(fndelim + 1);
                    HMODULE dll;
                    if(this->gLIB.find(lib) != this->gLIB.end()){
                        dll = this->gLIB[lib];
                    }else{
                        std::string _err = "Failed to load library: '" + ("lib/"+lib) + "'";
                        if(this->Loge) this->LogError(_err);
                        *go_to_proc = false;
                        for(auto i : g_RET) delete (char*)i.second;
                        return;
                    }

                    if(!dll || dll == INVALID_HANDLE_VALUE){
                        std::string _err = "Failed to load library: '" + ("lib/"+lib) + "'";
                        if(this->Loge) this->LogError(_err);
                        *go_to_proc = false;
                        for(auto i : g_RET) delete (char*)i.second;
                        return;
                    }

                    FNP func = (FNP)GetProcAddress(dll,fnname.c_str());
                    if(!func){
                        std::string _err = "Failed to load function '" + fnname + "' from library '" + ("lib/"+lib) + "'";
                        if(this->Loge) this->LogError(_err);
                        *go_to_proc = false;
                        for(auto i : g_RET) delete (char*)i.second;
                        return;
                    }

                    int fnstatus = -1;
                    std::map<std::string,void*> fnarg;
                    std::map<std::string,std::string> fnarg_stamp;

                    std::map<std::string,std::vector<std::string>> barg = fnID["arg"].get<std::map<std::string,std::vector<std::string>>>();
                    std::string _errmsg = "";
                    for(auto it : barg){
                        if(it.second[1] == "r"){
                            std::string datatype = it.second[2];
                            std::string preid = it.second[0];
                            if(datatype != ev["main"][preid]["ret"].get<std::string>()){
                                // ERROR HERE
                                _errmsg = "Invalid data type (Pre-Process)";
                                break;
                            }
                            if(g_RET.find(preid) == g_RET.end()){
                                // ERROR LAGI
                                _errmsg = "Cannot find previous returned data '"+preid+"' (Pre-Process)";
                                break;
                            }
                            void *darg = g_RET[preid];
                            fnarg[it.first] = darg;
                            fnarg_stamp[it.first] = it.second[1];
                        }else if(it.second[1] == "c"){
                            std::string* fval = new std::string();
                            *fval += it.second[0];
                            fnarg[it.first] = fval;
                            fnarg_stamp[it.first] = it.second[1];
                        }else if(it.second[1] == "t"){
                            std::string* fval = this->TextareaParser(it.second[0],g_RET,ev["main"]);
                            if(fval == NULL){
                                _errmsg = "Failed to Parse textarea data";
                                break;
                            }
                            fnarg[it.first] = fval;
                            fnarg_stamp[it.first] = it.second[1];
                        }
                    }

                    if(_errmsg != ""){
                        if(this->Loge) this->LogError(_errmsg);
                        *go_to_proc = false;
                        for(auto fa : fnarg){
                            if(fnarg_stamp[fa.first] != "r"){
                                delete (char*)fa.second;
                            }
                        }
                        for(auto i : g_RET) delete (char*)i.second;
                        return;
                    }

                    // SET CRITICAL SECTION
                    EnterCriticalSection(&this->cs);
                    void* data = func(ip,&fnarg,&fnstatus,go_to_proc);
                    LeaveCriticalSection(&this->cs);
                    // END CRITICAL SECTION

                    if(fnID["out-type"] == "m"){
                        if(fnstatus == -1){
                            std::string _err = "Invalid function routing";
                            if(this->Loge) this->LogError(_err);
                            *go_to_proc = false;
                            for(auto fa : fnarg){
                                if(fnarg_stamp[fa.first] != "r"){
                                    delete (char*)fa.second;
                                }
                            }
                            for(auto i : g_RET) delete (char*)i.second;
                            return;
                        }
                        
                        json nr = fnID["out"]["OUT-" + std::to_string(fnstatus)];
                        if(nr["bring-ret"] == "y"){
                            g_RET[currentId] = data;
                        }
                        currentId = nr["link"];
                    }else{
                        if(fnID["out"]["bring-ret"] == "y"){
                            g_RET[currentId] = data;
                        }
                        currentId = fnID["out"]["link"];
                    }

                    for(auto fa : fnarg){
                        if(fnarg_stamp[fa.first] != "r"){
                            delete (char*)fa.second;
                        }
                    }
                }
            }
        }

        void Interpreter(WebRequest* r,WebResponse*& w,json* poe){
            if(this->StaticURL.contains(r->URL)){
                json url = this->StaticURL[r->URL];
                if(url["method"].get<std::string>() == r->Method){
                    if(url["schema"] == "HTTP"){
                        w->Header["Content-Type"] = url["content-type"].get<std::string>();
                        json bp = url["b"];
                        json prog = bp["main"];
                        std::string currentId = bp["start"];
                        HMODULE dll;
                        std::map<std::string,void*> g_RET;
                        while(true){
                            if(currentId == "END"){
                                for(auto i : g_RET) delete (char*)i.second;
                                break;
                            }

                            if(prog.contains(currentId.c_str())){
                                json fnID = prog[currentId.c_str()];
                                std::string fn = fnID["fn"].get<std::string>();
                                int fndelim = fn.find("/");

                                std::string lib = fn.substr(0,fndelim);
                                std::string fnname = fn.substr(fndelim + 1);

                                HMODULE dll;
                                if(this->gLIB.find(lib) != this->gLIB.end()){
                                    dll = this->gLIB[lib];
                                }else{
                                    std::string _err = "Failed to load library: '" + ("lib/"+lib) + "'";
                                    if(this->Loge) this->LogError(_err);
                                    w->Status = 500;
                                    w->Body = "500";
                                    for(auto i : g_RET) delete (char*)i.second;
                                    return;
                                }

                                FNL func = (FNL)GetProcAddress(dll,fnname.c_str());
                                if(!func){
                                    std::string _err = "Failed to load function '" + fnname + "' from library '" + ("lib/"+lib) + "'";
                                    if(this->Loge) this->LogError(_err);
                                    w->Status = 500;
                                    w->Body = "500";
                                    for(auto i : g_RET) delete (char*)i.second;
                                    return;
                                }

                                std::string fnmsg = "";
                                int fnstatus = -1;
                                std::map<std::string,void*> fnarg;
                                std::map<std::string,std::string> fnarg_stamp;
                                std::map<std::string,std::vector<std::string>> barg = fnID["arg"].get<std::map<std::string,std::vector<std::string>>>();
                                std::string _errmsg = "";

                                for(auto it : barg){
                                    if(it.second[1] == "r"){
                                        std::string datatype = it.second[2];
                                        std::string preid = it.second[0];
                                        if(datatype != prog[preid]["ret"].get<std::string>()){
                                            // ERROR HERE
                                            _errmsg = "Invalid data type (On-Process)";
                                            break;
                                        }
                                        if(g_RET.find(preid) == g_RET.end()){
                                            // ERROR LAGI
                                            _errmsg = "Cannot find previous returned data '"+preid+"' (On-Process)";
                                            break;
                                        }
                                        void *darg = g_RET[preid];
                                        fnarg[it.first] = darg;
                                        fnarg_stamp[it.first] = it.second[1];
                                    }else if(it.second[1] == "c"){
                                        std::string* fval = new std::string();
                                        *fval += it.second[0];
                                        fnarg[it.first] = fval;
                                        fnarg_stamp[it.first] = it.second[1];
                                    }else if(it.second[1] == "t"){
                                        std::string* fval = this->TextareaParser(it.second[0],g_RET,prog);
                                        if(fval == NULL){
                                            _errmsg = "Failed to parse textarea";
                                            break;
                                        }
                                        fnarg[it.first] = fval;
                                        fnarg_stamp[it.first] = it.second[1];
                                    }
                                }

                                if(_errmsg != ""){
                                    if(this->Loge) this->LogError(_errmsg);
                                    w->Status = 500;
                                    w->Body = "500";
                                    for(auto fa : fnarg){
                                        if(fnarg_stamp[fa.first] != "r"){
                                            delete (char*)fa.second;
                                        }
                                    }
                                    for(auto i : g_RET) delete (char*)i.second;
                                    return;
                                }

                                // SET CRITICAL SECTION
                                EnterCriticalSection(&this->cs);
                                void* data = func(r,w,&fnmsg,&fnstatus,&fnarg);
                                LeaveCriticalSection(&this->cs);
                                // END CRITICAL SECTION

                                if(fnmsg != ""){
                                    if(this->Loge) this->LogError(fnmsg);
                                    w->Status = 500;
                                    w->Body = "500";
                                    for(auto fa : fnarg){
                                        if(fnarg_stamp[fa.first] != "r"){
                                            delete (char*)fa.second;
                                        }
                                    }
                                    for(auto i : g_RET) delete (char*)i.second;
                                    return;
                                }

                                if(fnID["out-type"] == "m"){
                                    if(fnstatus == -1){
                                        std::string _err = "Invalid function routing";
                                        if(this->Loge) this->LogError(_err);
                                        w->Status = 500;
                                        w->Body = "500";
                                        for(auto fa : fnarg){
                                            if(fnarg_stamp[fa.first] != "r"){
                                                delete (char*)fa.second;
                                            }
                                        }
                                        for(auto i : g_RET) delete (char*)i.second;
                                        return;
                                    }
                                    
                                    json nr = fnID["out"]["OUT-" + std::to_string(fnstatus)];
                                    if(nr["bring-ret"] == "y"){
                                        g_RET[currentId] = data;
                                    }
                                    currentId = nr["link"];
                                }else{
                                    if(fnID["out"]["bring-ret"] == "y"){
                                        g_RET[currentId] = data;
                                    }
                                    currentId = fnID["out"]["link"];
                                }

                                for(auto fa : fnarg){
                                    if(fnarg_stamp[fa.first] != "r"){
                                        delete (char*)fa.second;
                                    }
                                }
                            }else{
                                w->Status = 500;
                                w->Body = "500";
                                break;
                            }
                        }
                    }else if(url["schema"] == "WS"){
                        
                    }
                }else{
                    w->Status = 405;
                    w->Body = "405";
                }

                *poe = url["events"]["post-process"];
            }else{
                bool hasUrl = false;
                std::string inURL = "";
                int ulen = r->URL.length();
                for(auto i = this->DynamicURL.begin(); i != this->DynamicURL.end();++i){
                    std::string ul = i.key();
                        char ulc = ul.back();
                        if(ulc != '/' && ulc != '\\'){
                            ul += "/";
                        }
                    if(ul.length() < ulen){

                        if(r->URL.find(ul) == 0){
                            hasUrl = true;
                            inURL = ul;
                            break;
                        }
                    }
                }

                if(hasUrl && inURL != "" && this->DynamicURL[inURL]["method"] == r->Method){
                    std::string base_loc = this->DynamicURL[inURL]["route"];

                    std::string rpath = r->URL.substr(inURL.length());
                    if(rpath.length() > 0){
                        if(rpath[0] == '/' || rpath[0] == '\\'){
                            w->Status = 404;
                            w->Body = "404";
                        }else{
                            if(base_loc.length() > 0){
                                /*
                                ON-PROCESS
                                */

                                if(base_loc.back() != '/' && base_loc.back() != '\\'){
                                    base_loc += "/";
                                }
                                
                                std::string dsp = base_loc + rpath;
                                std::ifstream f(dsp,std::ios::in | std::ios::binary);
                                if(!f.good()){
                                    w->Status = 404;
                                    w->Body = "404";
                                    f.close();
                                    return;
                                }
                                
                                std::string ext = Root::getFileExt(dsp);
                                if(ext == ""){
                                    w->Header["Content-Type"] = "text/plain";
                                }

                                if(this->WebContentType.find(ext) != this->WebContentType.end()){
                                    w->Header["Content-Type"] = this->WebContentType[ext];
                                }else{
                                    w->Header["Content-Type"] = "text/plain";
                                }


                                std::string buffer_file;
                                f.seekg(0, std::ios::end);
                                buffer_file.resize(f.tellg());
                                f.seekg(0,std::ios::beg);
                                f.read(&buffer_file[0], buffer_file.size());
                                w->Body = buffer_file;
                                w->Status = 200;
                                f.close();
                            }else{
                                w->Status = 404;
                                w->Body = "404";
                            }
                        }
                    }else{
                        w->Status = 404;
                        w->Body = "404";
                    }

                    *poe = this->DynamicURL[inURL]["events"]["post-process"];
                }else{
                    w->Status = 404;
                    w->Body = "404";
                }
            }
        }

        void PostInterpret(WebRequest* r,WebResponse*& w,json poe){
            json prog = poe["main"];
            std::string currentId = poe["start"];
            HMODULE dll;
            std::map<std::string,void*> g_RET;
            while(true){
                if(currentId == "END"){
                    for(auto i : g_RET) delete (char*)i.second;
                    break;
                }
                if(prog.contains(currentId.c_str())){
                    json fnID = prog[currentId.c_str()];
                    std::string fn = fnID["fn"].get<std::string>();
                    int fndelim = fn.find("/");
                    std::string lib = fn.substr(0,fndelim);
                    std::string fnname = fn.substr(fndelim + 1);
                    HMODULE dll;
                    if(this->gLIB.find(lib) != this->gLIB.end()){
                        dll = this->gLIB[lib];
                    }else{
                        std::string _err = "Failed to load library: '" + ("lib/"+lib) + "'";
                        if(this->Loge) this->LogError(_err);
                        w->Status = 500;
                        w->Body = "500";
                        for(auto i : g_RET) delete (char*)i.second;
                        return;
                    }
                    FNL func = (FNL)GetProcAddress(dll,fnname.c_str());
                    if(!func){
                        std::string _err = "Failed to load function '" + fnname + "' from library '" + ("lib/"+lib) + "'";
                        if(this->Loge) this->LogError(_err);
                        w->Status = 500;
                        w->Body = "500";
                        for(auto i : g_RET) delete (char*)i.second;
                        return;
                    }
                    std::string fnmsg = "";
                    int fnstatus = -1;
                    std::map<std::string,void*> fnarg;
                    std::map<std::string,std::string> fnarg_stamp;
                    std::map<std::string,std::vector<std::string>> barg = fnID["arg"].get<std::map<std::string,std::vector<std::string>>>();
                    std::string _errmsg = "";
                    for(auto it : barg){
                        if(it.second[1] == "r"){
                            std::string datatype = it.second[2];
                            std::string preid = it.second[0];
                            if(datatype != prog[preid]["ret"].get<std::string>()){
                                // ERROR HERE
                                _errmsg = "Invalid data type (Post-Process)";
                                break;
                            }
                            if(g_RET.find(preid) == g_RET.end()){
                                // ERROR LAGI
                                _errmsg = "Cannot find previous returned data '"+preid+"' (Post-Process)";
                                break;
                            }
                            void *darg = g_RET[preid];
                            fnarg[it.first] = darg;
                            fnarg_stamp[it.first] = it.second[1];
                        }else if(it.second[1] == "c"){
                            std::string* fval = new std::string();
                            *fval += it.second[0];
                            fnarg[it.first] = fval;
                            fnarg_stamp[it.first] = it.second[1];
                        }else if(it.second[1] == "t"){
                            std::string* fval = this->TextareaParser(it.second[0],g_RET,prog);
                            if(fval == NULL){
                                _errmsg = "Failed to Parse textarea data";
                                break;
                            }
                            fnarg[it.first] = fval;
                            fnarg_stamp[it.first] = it.second[1];
                        }
                    }

                    if(_errmsg != ""){
                        if(this->Loge) this->LogError(fnmsg);
                        w->Status = 500;
                        w->Body = "500";
                        for(auto fa : fnarg){
                            if(fnarg_stamp[fa.first] != "r"){
                                delete (char*)fa.second;
                            }
                        }
                        for(auto i : g_RET) delete (char*)i.second;
                        return;
                    }

                    // SET CRITICAL SECTION
                    EnterCriticalSection(&this->cs);
                    void* data = func(r,w,&fnmsg,&fnstatus,&fnarg);
                    LeaveCriticalSection(&this->cs);
                    // END CRITICAL SECTION
                    if(fnmsg != ""){
                        if(this->Loge) this->LogError(fnmsg);
                        w->Status = 500;
                        w->Body = "500";
                        for(auto fa : fnarg){
                            if(fnarg_stamp[fa.first] != "r"){
                                delete (char*)fa.second;
                            }
                        }
                        for(auto i : g_RET) delete (char*)i.second;
                        return;
                    }
                    if(fnID["out-type"] == "m"){
                        if(fnstatus == -1){
                            std::string _err = "Invalid function routing";
                            if(this->Loge) this->LogError(_err);
                            w->Status = 500;
                            w->Body = "500";
                            for(auto fa : fnarg){
                                if(fnarg_stamp[fa.first] != "r"){
                                    delete (char*)fa.second;
                                }
                            }
                            for(auto i : g_RET) delete (char*)i.second;
                            return;
                        }
                        
                        json nr = fnID["out"]["OUT-" + std::to_string(fnstatus)];
                        if(nr["bring-ret"] == "y"){
                            g_RET[currentId] = data;
                        }
                        currentId = nr["link"];
                    }else{
                        if(fnID["out"]["bring-ret"] == "y"){
                            g_RET[currentId] = data;
                        }
                        currentId = fnID["out"]["link"];
                    }
                    for(auto fa : fnarg){
                        if(fnarg_stamp[fa.first] != "r"){
                            delete (char*)fa.second;
                        }
                    }
                }else{
                    w->Status = 500;
                    w->Body = "500";
                    break;
                }
            }
        }

        void Processing(HSSock*& client,HSCon*& con,std::vector<unsigned char> in,std::string& out){
            json poe;
            WebRequest* request = NULL;
            WebResponse* response;
            bool isAllocated = false;
            if(con->StatusCode == WebError::SAFE){
                std::string err = "";
                int stacode = 0;
                // CHANGE REQUEST HTTP STRUCTURE AND PARSER
                request = ParseHTTPRequest(&stacode,&err,in);
                response = new WebResponse();
                isAllocated = true;
                if(stacode == 0 && err == ""){
                    char* ip = (char*)malloc(16);
                    InetNtop(AF_INET,&(client->AddrInfo.sin_addr),ip,16);
                    if(this->Logr) this->LogRequest(ip,request);
                    free(ip);
                    this->Interpreter(request,response,&poe);
                }else if(err != "" && stacode != 0){
                    response->Status = stacode;
                    response->Body = std::to_string(stacode);
                }else{
                    response->Status = 500;
                    response->Body = "500";
                }
            }else if(con->StatusCode == WebError::BUFFER_EXCEEDS_LIMIT){
                response->Status = 413;
                response->Body = "413";
            }else{
                response->Status = 500;
                response->Body = "500";
            }


            if(poe.contains("start")){
                this->PostInterpret(request,response,poe);
            }else if(this->Events["post-process"].contains("main")){
                this->PostInterpret(request,response,this->Events["post-process"]);
            }

            response->Version = "HTTP/1.1";

            ParseHTTPResponse(response,out);

            if(request != NULL) delete request;
            if(isAllocated) delete response;
        }

        void Worker(){
            while(this->WorkerStatus){
                try{
                    HSSock *client;
                    HSCon *con;
                    DWORD bTransferred;
                    bool bRes = GetQueuedCompletionStatus(this->ghIOCP,&bTransferred,(PULONG_PTR)&client,(LPOVERLAPPED*)&con,INFINITE);
                    if(!bRes){
                        if(client != NULL){
                            if(bTransferred == 0){
                                delete client;
                                continue;
                            }
                            delete client;
                            std::string err = "Error get queue (" + std::to_string(GetLastError()) + ")";
                            throw(err);
                        }else{
                            std::string err = "Error get queue (" + std::to_string(GetLastError()) + ")";
                            throw(err);
                        }
                    }

                    // if(!con->PreInterpret){
                    //     con->PreInterpret = true;
                    //     char* ip = (char*)malloc(16);
                    //     InetNtop(AF_INET,&(client->AddrInfo.sin_addr),ip,16);
                    //     bool cont = true;
                    //     this->PreInterpret(ip,&cont);
                    //     free(ip);
                    //     if(!cont) con->Operation = HSOperation::STATE_CLOSE;
                    // }

                    std::cout << "OP: " << con->Operation << '\n';
                    DWORD flag = 0;
                    DWORD size = 0;
                    char al[100];
                    WSABUF buf;
                    buf.len = 10;
                    buf.buf = al;
                    WSABUF t = {0,0};
                    int rRes = WSARecv(client->Sock,&buf,1,&size,&flag,&con->overlapped,NULL);
                    int er = WSAGetLastError();
                    std::cout << "RES: " << al << " | " << rRes << " - " << er << '\n';
                    if(er == WSA_IO_PENDING){
                        std::cout << "DONE\n";
                        continue;
                    }
                    Sleep(1000);
                    WSASend(client->Sock,&t,1,&size,0,&con->overlapped,NULL);
                    continue;

                    if(con->Operation == HSOperation::STATE_READ){
                        while(true){

                            char *buffer = (char*)malloc(this->ChunkSize);
                            con->In.buf = buffer;
                            
                            DWORD flag = 0;
                            DWORD size = 0;
                            
                            int rRes = WSARecv(client->Sock,&con->In,1,&size,&flag,&con->overlapped,NULL);
                            if(con->InDataLen >= this->MaxBufferSize){
                                con->Operation = HSOperation::STATE_WRITE;
                                con->StatusCode = WebError::BUFFER_EXCEEDS_LIMIT;
                                break;
                            }
                            int wsaerr = WSAGetLastError();
                            if(rRes == SOCKET_ERROR) {
                                if(wsaerr == WSA_IO_PENDING){
                                    // WAITING
                                    if(con->HasRecv){
                                        if(con->InDataLen > 0){
                                            con->Operation = HSOperation::STATE_WRITE;
                                            con->StatusCode = WebError::SAFE;
                                        }else{
                                            con->Operation = HSOperation::STATE_CLOSE;
                                            con->StatusCode = WebError::UNKNOWN_ERROR;
                                        }
                                    }
                                    break;
                                }else{
                                    // UNKNWON ERROR
                                    con->Operation = HSOperation::STATE_WRITE;
                                    con->StatusCode = WebError::UNKNOWN_ERROR;
                                    break;
                                }
                            }else{
                                // SUCCESS
                                if(size < this->ChunkSize && size > 0){
                                    con->InDataLen += size;
                                    con->HasRecv = true;
                                    con->Operation = HSOperation::STATE_WRITE;
                                    con->StatusCode = WebError::SAFE;
                                    WSABUF res;
                                    res.buf = (char*) realloc(buffer,size);
                                    res.len = size;
                                    con->InData.push_back(res);
                                    break;
                                }else if(size == this->ChunkSize){
                                    con->InDataLen += size;
                                    con->HasRecv = true;
                                    WSABUF res;
                                    res.buf = buffer;
                                    res.len = size;
                                    con->InData.push_back(res);
                                }else{
                                    con->Operation = HSOperation::STATE_CLOSE;
                                    con->StatusCode = WebError::UNKNOWN_ERROR;
                                    break;
                                }
                            }
                        }
                    }else if(con->Operation == HSOperation::STATE_WRITE){
                        if(!con->Interpreted){
                            std::vector<unsigned char> raw_request;
                            int si = 0;

                            for(WSABUF i : con->InData){    
                                int alen = si + i.len;
                                raw_request.resize(alen);
                                if(memmove_s(&raw_request[si],alen,i.buf,i.len) != 0) con->StatusCode = WebError::UNKNOWN_ERROR;
                                si += i.len;
                            }

                            std::string buffer = "";
                            this->Processing(client,con,raw_request,buffer);
                            int reslen = buffer.length();
                            con->OutData = buffer;
                            con->OutDataLen = reslen;
                            con->Interpreted = true;
                        }

                        con->Out.buf = (char*)con->OutData.c_str();

                        if(con->OutDataLen >= this->ChunkSize){
                            con->Out.len = this->ChunkSize;
                            con->OutDataLen -= this->ChunkSize;
                        }else{
                            con->Out.len = con->OutDataLen;
                            con->OutDataLen = 0;
                        }

                        DWORD size = 0;
                        int sRes = WSASend(client->Sock,&con->Out,1,&size,0,&con->overlapped,NULL);
                        int wsaerr = WSAGetLastError();
                        if(wsaerr != 0 && wsaerr != WSA_IO_PENDING){
                            con->Operation = HSOperation::STATE_CLOSE;
                            continue;
                        }
                        if(con->OutDataLen <= 0){
                            con->Operation = HSOperation::STATE_CLOSE;
                            continue;
                        }
                    }else if(con->Operation == HSOperation::STATE_CLOSE){
                        con->Operation = -1;
                        delete client;
                    }
                }catch(std::string msg){
                    if(this->Loge) this->LogError(msg);
                }
            }
        }

        std::string Name;
        std::string IP;
        int Port;
        int Protocol;
        int ReadTimeout;
        int WriteTimeout;
        double MaxBufferSize;
        int Backlog;
        bool Logr;
        bool Loge;
        int ChunkSize;
        std::map<std::string,std::string> WebContentType;
        json StaticURL;
        json DynamicURL;
        json Events;
        std::vector<std::string> Plugins;

        std::string Setup(){
            WSADATA wsa;
            SYSTEM_INFO si;
            struct sockaddr_in server_info;
            SecureZeroMemory(&server_info,sizeof(server_info));

            try{
                InitializeCriticalSectionAndSpinCount(&this->cs,1000);
            }catch(...){
                return "Failed to initialize critical section";
            }

            for(int i = 0; i < 1024; i++){
                Threads[i] = INVALID_HANDLE_VALUE;
            }

            GetSystemInfo(&si);
            this->TotalThread = si.dwNumberOfProcessors * 2;
            // this->TotalThread = 1;
            if(this->TotalThread >= 1024) return "Total thread exceeds the limit";

            this->ghIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0);
            if(this->ghIOCP == NULL) return "Failed to create io completion port";

            int tCounter = 0;
            this->WorkerStatus = true;
            while(tCounter < this->TotalThread){
                DWORD TID = 0;
                HANDLE ht = CreateThread(NULL,0,CreateWorker,this,0,&TID);
                if(ht != NULL && ht != INVALID_HANDLE_VALUE){
                    tCounter++;
                }
            }

            for(std::string i : this->Plugins){
                HMODULE dll = LoadLibrary(_T(("lib/"+i).c_str()));
                if(!dll || dll == INVALID_HANDLE_VALUE){
                    std::string _err = "Failed to load library: '" + ("lib/"+i) + "'";
                    return _err;
                }
                this->gLIB[i] = dll;
            }

            if(WSAStartup(MAKEWORD(2,2),&wsa) != 0){
                std::string err = "Failed to setup socket (";
                err += std::to_string(WSAGetLastError());
                err += ")";
                return err;
            }

            int SockType = SOCK_STREAM;
            int SockProto = IPPROTO_TCP;
            if(this->Protocol == NetSchemeType::UDP){
                SockType = SOCK_DGRAM;
                SockProto = IPPROTO_UDP;
            }
            this->s = WSASocketW(AF_INET,SockType,SockProto,NULL,0,WSA_FLAG_OVERLAPPED);
            if(this->s == INVALID_SOCKET){
                std::string err = "Failed to setup socket (";
                err += std::to_string(WSAGetLastError());
                err += ")";
                WSACleanup();
                return err;
            }

            server_info.sin_family = AF_INET;
            server_info.sin_port = htons(this->Port);
            int ipton = InetPton(AF_INET,this->IP.c_str(),&server_info.sin_addr);
            if(ipton == 0){
                WSACleanup();
                return "Invalid IP Address";
            }else if(ipton == -1){
                std::string err = "Failed to setup socket (";
                err += std::to_string(WSAGetLastError());
                err += ")";
                WSACleanup();
                return err;
            }

            if(bind(this->s,(SOCKADDR*)&server_info,sizeof(server_info)) == INVALID_SOCKET){
                std::string err = "Failed to setup socket (";
                err += std::to_string(WSAGetLastError());
                err += ")";
                WSACleanup();
                return err;
            }

            if(listen(this->s,this->Backlog) == INVALID_SOCKET){
                std::string err = "Failed to setup socket (";
                err += std::to_string(WSAGetLastError());
                err += ")";
                WSACleanup();
                return err;
            }

            return "";
        }

        void Run(){
            while(true){
                try{
                    struct sockaddr_in addrinfo;
                    SecureZeroMemory(&addrinfo,sizeof(addrinfo));
                    int cs = sizeof(addrinfo);
                    SOCKET c = WSAAccept(this->s,(SOCKADDR*)&addrinfo,&cs,NULL,NULL);
                    if(c == INVALID_SOCKET){
                        std::string err = "Error while accepting client (" + std::to_string(WSAGetLastError()) + ")";
                        throw(err);
                    }

                    HSSock* client = new HSSock();
                    client->Sock = c;
                    client->AddrInfo = addrinfo;
                    client->Con.In.len = this->ChunkSize;
                    client->Con.Out.len = this->ChunkSize;
                    client->Con.Operation = HSOperation::STATE_READ;

                    this->ghIOCP = CreateIoCompletionPort((HANDLE)client->Sock,this->ghIOCP,(DWORD_PTR)client,0);
                    if(this->ghIOCP == NULL){
                        std::string err = "Server error (" + std::to_string(WSAGetLastError()) + ")";
                        delete client;
                        throw(err);
                    }

                    WSABUF tmpbuf;
                    tmpbuf.buf = NULL;
                    tmpbuf.len = 0;
                    DWORD size = 0;
                    DWORD flag = 0;
                    int rRes = WSARecv(c,&tmpbuf,1,&size,&flag,&client->Con.overlapped,NULL);
                    int wsaerr = WSAGetLastError();
                    if(rRes == SOCKET_ERROR && wsaerr != WSA_IO_PENDING){
                        std::string err = "Error initial receive (" + std::to_string(wsaerr) + ")";
                        delete client;
                        throw(err);
                    }
                }catch(std::string msg){
                    if(this->Loge) this->LogError(msg);
                }
            }
        }
};

DWORD WINAPI CreateWorker(LPVOID p){
    HTTPServer* so = (HTTPServer*)p;
    so->Worker();
    return 0;
}