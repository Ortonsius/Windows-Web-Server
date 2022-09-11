WebRequest* ParseHTTPRequest(int *statcode,std::string* err,std::vector<unsigned char> req){
    int i = 0;
    int max = req.size();
    bool MethodStatus = false;
    bool URLStatus = false;
    bool IsHeaderKey = false;

    // Prop
    WebRequest* Request = new WebRequest();
    WebRequest* empty = NULL;
    

    // Checking request method
    if(i + 7 >= max){
        *statcode = 400;
        *err = "Invalid request body 1";
        delete Request;
        return empty;
    }
    while(i < 7){
        if((int)req[i] >= 32 && (int)req[i] < 128){
            if(req[i] == static_cast<unsigned char>(' ')){
                MethodStatus = true;
                i++;
                break;
            }
            Request->Method += static_cast<char>(req[i]);
            i++;
        }else{
            *statcode = 400;
            *err = "Invalid request method";
            delete Request;
            return empty;
        }
    }

    if(!MethodStatus){
        *statcode = 400;
        *err = "Invalid request method";
        delete Request;
        return empty;
    }
    
    if(std::find(WebRules::AvailableMethod.begin(), WebRules::AvailableMethod.end(), Request->Method) == WebRules::AvailableMethod.end()){
        *err = "Unavailable request method";
        *statcode = 400;
        delete Request;
        return empty;
    }


    // Checking Full URL
    if(req[i] != static_cast<unsigned char>('/')){
        *err = "Invalid URL";
        delete Request;
        return empty;
    }
    int MaxURLLen = i + 2048;
    while(i < MaxURLLen){
        if((int)req[i] >= 32 && (int)req[i] < 128){
            if(i >= max) {
                *statcode = 400;
                *err = "Invalid request body 5";
                delete Request;
                return empty;
            }
            if(req[i] == static_cast<unsigned char>(' ')){
                URLStatus = true;
                i++;
                break;
            }
            Request->FullURL += static_cast<char>(req[i]);
            i++;
        }else{
            *statcode = 400;
            *err = "Invalid request method";
            delete Request;
            return empty;
        }
    }

    if(!URLStatus){
        *statcode = 413;
        *err = "Invalid URL";
        delete Request;
        return empty;
    }
    
    // Get real URL
    int rin = Request->FullURL.find('?');
    Request->URL = Request->FullURL.substr(0,rin);

    // Checking version
    if(i >= i + 8){
        *statcode = 400;
        *err = "Invalid request body 2";
        delete Request;
        return empty;
    }
    int MaxVerLen = i + 9;
    while(i < MaxVerLen){
        if(((int)req[i] >= 32 && (int)req[i] < 128) || req[i] == '\r'){
            if(i >= max) {
                *statcode = 400;
                *err = "Invalid request body 3";
                delete Request;
                return empty;
            }
            if(req[i] == static_cast<unsigned char>('\r')){
                if(req[i + 1] == static_cast<unsigned char>('\n')) {
                    i += 2;
                    break;
                }
            }
            Request->Version += static_cast<char>(req[i]);
            i++;
        }else{
            *statcode = 400;
            *err = "Invalid request method";
            delete Request;
            return empty;
        }
    }

    std::string hkey = "";
    std::string hval = "";
    for(;i < max;i++){
        if(!IsHeaderKey){
            if((int)req[i] >= 32 && (int)req[i] < 128){
                if(req[i] == ':'){
                    IsHeaderKey = true;
                    continue;
                }
                hkey += static_cast<char>(req[i]);
            }else{
                *statcode = 400;
                *err = "Invalid request body 4";
                delete Request;
                return empty;
            }
        }else{
            if((int)req[i] >= 32 && (int)req[i] < 128){
                hval += static_cast<char>(req[i]);
            }else{
                if(i + 4 < max){
                    if(req[i] == '\r' && req[i + 1] == '\n'){
                        if(hkey == "" || hval == ""){
                            *statcode = 400;
                            *err = "Invalid header";
                            delete Request;
                            return empty; 
                        }

                        Request->Header[hkey] = hval;
                        IsHeaderKey = false;
                        hkey = "";
                        hval = "";
                        i++;
                        if(req[i + 2] == '\r' && req[i + 3] == '\n'){
                            i += 4;
                            break;
                        }
                    }else{
                        *statcode = 400;
                        *err = "Invalid request";
                        delete Request;
                        return empty;
                    }
                }else if(i + 2 < max){
                    if(req[i] == '\r' && req[i + 1] == '\n'){
                        if(hkey == "" || hval == ""){
                            *statcode = 400;
                            *err = "Invalid header";
                            delete Request;
                            return empty; 
                        }
                        Request->Header[hkey] = hval;
                        hkey = "";
                        hval = "";
                        i += 2;
                        break;
                    }else{
                        *statcode = 400;
                        *err = "Invalid request";
                        delete Request;
                        return empty;
                    }
                }
            }
        }
    }

    
    // Checking body
    while(i < max){
        Request->Body += (char)req[i];
        i++;
    }

    *err = "";
    *statcode = 0;
    return Request;
}

void ParseHTTPResponse(WebResponse* res,std::string& buffer){
    buffer = "";
    std::string tmp = "";
    tmp += res->Version + " " + std::to_string(res->Status) + "\r\n";
    for(auto i : res->Header){
        tmp += i.first + " : " + i.second + "\r\n";
    }
    tmp += "\r\n";
    tmp += res->Body;
    buffer = tmp;
}