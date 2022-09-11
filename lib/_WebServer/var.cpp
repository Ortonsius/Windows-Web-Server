namespace EnType{
    const int HTTP_SERVER = 0;
    const int HTTP_CLIENT = 1;
}

namespace NetSchemeType{
    const int UDP = 0;
    const int TCP = 1;
}

// namespace WebProtocol{
//     const int WebSocket = 0;
//     const int HTTP = 1;
// }

namespace WebError{
    const int UNKNOWN_ERROR         = -1;                       // Unknown Error
    const int SAFE                  = 0;                        // No error
    const int READ_TIMEOUT          = 1;                        // Receive Timeout
    const int WRITE_TIMEOUT         = 2;                        // Send Timeout
    const int BUFFER_EXCEEDS_LIMIT  = 3;                        // Buffer size exceeds limit   
}

namespace HSOperation{
    const int STATE_READ = 0;
    const int STATE_WRITE = 1;
    const int STATE_CLOSE = 2;
}

namespace WebRules{
    const std::vector<std::string> AvailableMethod = {"GET","POST","HEAD","PUT","DELETE","CONNECT","OPTIONS","TRACE","PATCH"};
}



// Web Server Objects
struct HSCon{
    WSAOVERLAPPED overlapped;

    // Client Status
    int Operation = 0;
    int StatusCode = 0;
    int WebProtocol = 0;
    bool HasRecv = false;
    bool Interpreted = false;
    bool PreInterpret = false;


    // Client Buffer Information
    WSABUF In;
    WSABUF Out;
    int InDataLen = 0;
    int OutDataLen = 0;
    std::vector<WSABUF> InData;
    std::string OutData; 
};

struct HSSock{
    // GENERAL INFO
    SOCKET Sock;
    struct sockaddr_in AddrInfo;
    HSCon Con;

    ~HSSock(){
        for(WSABUF i : Con.InData){
            if(i.buf != NULL) free(i.buf);
        }
        // if(Con.Out.buf != NULL) free(Con.Out.buf);
        if(Sock != INVALID_SOCKET){
            shutdown(Sock,SD_BOTH);
            closesocket(Sock);
            Sock = INVALID_SOCKET;
        }
        SecureZeroMemory(&AddrInfo,sizeof(AddrInfo));
        SecureZeroMemory(&Con.In,sizeof(Con.In));
        SecureZeroMemory(&Con.Out,sizeof(Con.Out));
    }
};

struct WebRequest{
    std::string URL = "";
    std::string FullURL = "";
    std::string Method = "";
    std::string Version = "";
    std::map<std::string,std::string> Header = {};
    std::string Body = "";
};

struct WebResponse{
    std::string Version = "";
    int Status = 200;
    std::map<std::string,std::string> Header = {};
    std::string Body;
};


std::map<std::string,std::string> SetupWCT(){
    std::map<std::string,std::string> WebContentType;
    WebContentType["acc"] = "audio/acc";
    WebContentType["abw"] = "application/x-abiword";
    WebContentType["arc"] = "application/x-freearc";
    WebContentType["avif"] = "image/avif";
    WebContentType["avi"] = "video/x-msvideo";
    WebContentType["azw"] = "application/vnd.amazon.ebook";
    WebContentType["bin"] = "application/octet-stream";
    WebContentType["bmp"] = "image/bmp";
    WebContentType["bz"] = "application/x-bzip";
    WebContentType["bz2"] = "application/x-bzip2";
    WebContentType["cda"] = "application/x-cdf";
    WebContentType["csh"] = "application/x-csh";
    WebContentType["css"] = "text/css";
    WebContentType["csv"] = "text/csv";
    WebContentType["doc"] = "application/msword";
    WebContentType["docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
    WebContentType["eot"] = "application/vnd.ms-fontobject";
    WebContentType["epub"] = "application/epub+zip";
    WebContentType["gz"] = "application/gzip";
    WebContentType["gif"] = "image/gif";
    WebContentType["htm"] = "text/html";
    WebContentType["html"] = "text/html";
    WebContentType["ico"] = "image/vnd.microsoft.icon";
    WebContentType["ics"] = "text/calendar";
    WebContentType["jar"] = "application/java-archive";
    WebContentType["jpg"] = "image/jpeg";
    WebContentType["jpeg"] = "image/jpeg";
    WebContentType["js"] = "text/javascript";
    WebContentType["json"] = "application/json";
    WebContentType["jsonld"] = "application/ld+json";
    WebContentType["mid"] = "audio/midi";
    WebContentType["midi"] = "audio/midi";
    WebContentType["mjs"] = "text/javascript";
    WebContentType["mp3"] = "audio/mpeg";
    WebContentType["mp4"] = "video/mp4";
    WebContentType["mpeg"] = "audio/mpeg";
    WebContentType["mpkg"] = "application/vnd.apple.installer+xml";
    WebContentType["odp"] = "application/vnd.oasis.opendocument.presentation";
    WebContentType["ods"] = "application/vnd.oasis.opendocument.spreadsheet";
    WebContentType["odt"] = "application/vnd.oasis.opendocument.text";
    WebContentType["oga"] = "audio/ogg";
    WebContentType["ogv"] = "video/ogg";
    WebContentType["ogx"] = "application/ogg";
    WebContentType["opus"] = "audio/opus";
    WebContentType["otf"] = "font/otf";
    WebContentType["png"] = "image/png";
    WebContentType["pdf"] = "application/pdf";
    WebContentType["php"] = "application/x-httpd-php";
    WebContentType["ppt"] = "application/vnd.ms-powerpoint";
    WebContentType["pptx"] = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
    WebContentType["rar"] = "application/vnd.rar";
    WebContentType["rtf"] = "application/rtf";
    WebContentType["sh"] = "application/x-sh";
    WebContentType["svg"] = "image/svg+xml";
    WebContentType["swf"] = "application/x-shockwave-flash";
    WebContentType["tar"] = "application/x-tar";
    WebContentType["tif"] = "image/tiff";
    WebContentType["tiff"] = "image/tiff";
    WebContentType["ts"] = "video/mp2t";
    WebContentType["ttf"] = "font/ttf";
    WebContentType["txt"] = "text/plain";
    WebContentType["vsd"] = "application/vnd.visio";
    WebContentType["wav"] = "audio/wav";
    WebContentType["weba"] = "audio/webm";
    WebContentType["webm"] = "video/webm";
    WebContentType["webp"] = "image/webp";
    WebContentType["woff"] = "font/woff";
    WebContentType["woff2"] = "font/woff2";
    WebContentType["xhtml"] = "application/xhtml+xml";
    WebContentType["xls"] = "application/vnd.ms-excel";
    WebContentType["xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    WebContentType["xml"] = "application/xml";
    WebContentType["xul"] = "application/vnd.mozilla.xul+xml";
    WebContentType["zip"] = "application/zip";
    WebContentType["3gp"] = "video/3gpp";
    WebContentType["3g2"] = "video/3gpp2";
    WebContentType["7z"] = "application/x-7z-compressed";

    return WebContentType;
}

// Interpreter Func
typedef void* (__stdcall *FNL)(void*,void*,void*,int*,void*);       // Req,Res,msg,Status,arg

// Pre Interpreter Func
typedef void* (__stdcall *FNP)(char*,void*,int*,bool*);       // ip,arg,go_to_proc