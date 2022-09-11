DWORD WINAPI CreateWorker(LPVOID p);

class HTTPServer{
    private:
    SOCKET s;
    HANDLE ghIOCP;
    int TotalThread;
    CRITICAL_SECTION cs;
    std::map<std::string,HMODULE> gLIB;
    std::string LastSysErrMsg;
    std::string LastIntErrMsg;

    void SysLogError(std::string msg,int code){
        std::string _err = std::to_string(code) + " | " + msg;
        EnterCriticalSection(&this->cs);
        if(_err != this->LastSysErrMsg){
            time_t t = time(NULL);
            struct tm tm;
            localtime_s(&tm,&t);
            std::ofstream f("syserr.log",std::ios_base::app);
            f << Root::FixTime(std::to_string(tm.tm_mday)) << "-" << Root::FixTime(std::to_string(tm.tm_mon+1)) << "-" << std::to_string(tm.tm_year+1900) << " " << Root::FixTime(std::to_string(tm.tm_hour)) << ":" << Root::FixTime(std::to_string(tm.tm_min)) << ":" << Root::FixTime(std::to_string(tm.tm_sec)) << " | " << code << " | " << msg << "\n";
            f.close();
            this->LastSysErrMsg = _err;
        }
        LeaveCriticalSection(&this->cs);
    }

    void LogError(std::string msg,int code){
        std::string _err = std::to_string(code) + " | " + msg;
        EnterCriticalSection(&this->cs);
        if(_err != this->LastIntErrMsg){
            time_t t = time(NULL);
            struct tm tm;
            localtime_s(&tm,&t);
            std::ofstream f("interr.log",std::ios_base::app);
            f << Root::FixTime(std::to_string(tm.tm_mday)) << "-" << Root::FixTime(std::to_string(tm.tm_mon+1)) << "-" << std::to_string(tm.tm_year+1900) << " " << Root::FixTime(std::to_string(tm.tm_hour)) << ":" << Root::FixTime(std::to_string(tm.tm_min)) << ":" << Root::FixTime(std::to_string(tm.tm_sec)) << " | " << code << " | " << msg << "\n";
            f.close();
            this->LastIntErrMsg = _err;
        }
        LeaveCriticalSection(&this->cs);
    }

    bool TriggerNext(ClientData* client){
        DWORD size = 0;
        WSABUF _tmp = {0,0};
        int res = WSASend(client->Sock,&_tmp,1,&size,0,&client->Data.overlapped,NULL);
        if(WSAGetLastError() != WSA_IO_PENDING && res == SOCKET_ERROR){
            this->SysLogError("Failed to trigger client for next processing",(int)GetLastError());
            delete client;
            return false;
        }

        return true;
    }

    public:

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

        GetSystemInfo(&si);
        this->TotalThread = si.dwNumberOfProcessors * 2;
        // this->TotalThread = 1;
        if(this->TotalThread >= 1024) return "Total thread exceeds the limit";

        this->ghIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE,NULL,0,0);
        if(this->ghIOCP == NULL) return "Failed to create io completion port";

        int tCounter = 0;
        while(tCounter < this->TotalThread){
            DWORD TID = 0;
            HANDLE ht = CreateThread(NULL,0,CreateWorker,this,0,&TID);
            if(ht != NULL && ht != INVALID_HANDLE_VALUE) tCounter++;
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
        if(this->Protocol == SocketSetting::NET_UDP){
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

    void Worker(){
        while(true){
            ClientData *client;
            ClientOL *col;
            DWORD bTransferred;
            bool bRes = GetQueuedCompletionStatus(this->ghIOCP,&bTransferred,(PULONG_PTR)&client,(LPOVERLAPPED*)&col,INFINITE);
            if(!bRes){
                if(client != NULL){
                    if(bTransferred == 0){
                        delete client;
                        continue;
                    }
                    delete client;
                    this->SysLogError("Failed to get queue",(int)GetLastError());
                    continue;
                }else{
                    this->SysLogError("Failed to get queue",(int)GetLastError());
                    continue;
                }
            }

            if(col->Operation == ClientOP::RECV){
                std::cout << "CLIENT: RECV\n";
                if(col->Status == ClientStat::SAFE){
                    char buffer[this->ChunkSize];
                    WSABUF buf;
                    buf.len = this->ChunkSize;
                    buf.buf = buffer;

                    DWORD size = 0;
                    DWORD flag = 0;

                    int rRes = WSARecv(client->Sock,&buf,1,&size,&flag,&col->overlapped,NULL);
                    if(rRes == SOCKET_ERROR){
                        int wsaerr = WSAGetLastError();
                        if(wsaerr == WSA_IO_PENDING){
                            if(col->Recvd){
                                col->Operation = ClientOP::PRE_INTERPRET;
                            }
                            this->TriggerNext(client);
                        }else{
                            
                        }
                    }else{
                        col->Recvd = true;
                        if(size > 0 && size < this->ChunkSize){
                            col->Operation = ClientOP::PRE_INTERPRET;
                            long befsize = col->Input.size();
                            col->Input += buffer;
                            col->Input.resize(befsize + size);
                            this->TriggerNext(client);
                        }else if(size == this->ChunkSize){
                            col->Operation = ClientOP::RECV;
                            long befsize = col->Input.size();
                            col->Input += buffer;
                            col->Input.resize(befsize + size);
                            this->TriggerNext(client);
                        }else{
                            delete client;
                        }
                    }
                }else{
                    col->Status = ClientStat::UNERROR;
                    this->TriggerNext(client);
                }
            }else if(col->Operation == ClientOperation::PRE_INTERPRET){

            }else if(col->Operation == ClientOperation::INTERPRET){

            }else if(col->Operation == ClientOperation::POST_INTERPRET){

            }else if(col->Operation == ClientOperation::ERR){

            }
        }
    }

    void Run(){
        while(true){
            struct sockaddr_in addrinfo;
            SecureZeroMemory(&addrinfo,sizeof(addrinfo));
            int cls = sizeof(addrinfo);
            SOCKET c = WSAAccept(this->s,(SOCKADDR*)&addrinfo,&cls,NULL,NULL);
            if(c == INVALID_SOCKET){
                std::string err = "Error while accepting client (" + std::to_string(WSAGetLastError()) + ")";
                throw(err);
            }

            ClientData* client = new ClientData();
            client->Sock = c;
            client->AddrInfo = addrinfo;
            client->Data.Status = ClientStat::SAFE;
            client->Data.Operation = ClientOP::RECV;

            this->ghIOCP = CreateIoCompletionPort((HANDLE)client->Sock,this->ghIOCP,(DWORD_PTR)client,0);
            if(this->ghIOCP == NULL){
                this->SysLogError("Failed to add client data to IOCP",(int)GetLastError());
                delete client;
            }

            this->TriggerNext(client);
        }
    }
};

DWORD WINAPI CreateWorker(LPVOID p){
    HTTPServer* so = (HTTPServer*)p;
    so->Worker();
    return 0;
}