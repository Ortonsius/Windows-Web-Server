namespace SocketSetting{
    char NET_UDP = 'a';
    char NET_TCP = 'b';
}

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

namespace ClientOP{
    char RECV = 'a';
    char PRE_INTERPRET = 'b';
    char INTERPRET = 'c';
    char POST_INTERPRET = 'd';
    char SEND = 'e';
    char ERR = 'f';
    char CLOSE = 'g';
}

namespace ClientStat{
    char SAFE = 'a';
    char OVERSIZED_BUFF = 'b';
    char UNERROR = 'c';
}

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

struct ClientOL{
    WSAOVERLAPPED overlapped;

    char Operation;
    char Status;
    bool Recvd;

    WebRequest Request;
    WebResponse Response;

    std::string Input;
    std::string Output;
}

struct ClientData{
    SOCKET Sock;
    struct sockaddr_in AddrInfo;

    ClientOL Data;

    ~ClientData(){
        std::cout << "DELETE CLIENT\n";
        if(this->Sock != NULL && this->Sock != INVALID_SOCKET){
            shutdown(this->Sock,SD_BOTH);
            closesocket(this->Sock);
        }
    }
};