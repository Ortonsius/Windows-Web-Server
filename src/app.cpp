#include "../lib/json.hpp"
using json = nlohmann::json;

#include <iostream>
#include <string>
#include <chrono>
#include <ctime>
#include <vector>
#include <bitset>
#include <fstream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <algorithm>
#include <Windows.h>
#include <tchar.h>
#include <stack>

#pragma comment(lib,"ws2_32.lib")

#include "../lib/root.cpp"
#include "../lib/encryption.cpp"

// WebServer
#include "../lib/WebServer/var.cpp"
// #include "../lib/WebServer/http_parse.cpp"
#include "../lib/WebServer/core.cpp"

#include "section.cpp"


bool PreDataCheck(json data){
    if(!data.contains("general")) return false;
    if(!data["general"].contains("name")) return false;
    if(!data["general"].contains("engine-type")) return false;

    return true;
}

int main(int argc, char** argv){
    {
        try{
            std::cout << "START" << std::endl;

            // Check for argument entry
            if(argc != 2){
                std::cout << "[-] Invalid argument !!" << '\n';
                return 1;
            }

            // Read file
            std::string ss_filename = argv[1];
            std::ifstream ssf(ss_filename);

            // Now this project in development.
            // When pre-publish, encrypt the ssf

            // Check file exist
            if(!ssf.good()){
                std::cout << "[-] File not exist !!" << '\n';
                return 1;
            }

            // Parse data to json
            json ssdata;
            ssf >> ssdata;

            // Close file
            ssf.close();

            // Check data general
            if(!PreDataCheck(ssdata)){
                std::cout << "[-] File is corrupted !!" << '\n';
                return 1;
            }

            // Split engine section
            std::string secerr = "";
            switch(ssdata["general"]["engine-type"].get<int>()){
                case EnType::HTTP_SERVER :
                    secerr = SectionHTTPServer(ssdata);
                    std::cout << "[-] " << secerr << '\n';
                    break;
                default:
                    std::cout << "[-] File is corrupted !!" << '\n';
                    break;
            }
        }catch(...){
        }
    }

    return 0;
}