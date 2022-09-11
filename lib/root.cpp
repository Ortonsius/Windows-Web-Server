namespace EnType{
    const int HTTP_SERVER = 0;
    const int HTTP_CLIENT = 1;
}

class Root{
    public:
        static int UCLen(unsigned char* text){
            int x = 0;
            while(text[x] != '\0') x++;
            return x;
        }

        static unsigned char* UCCat(unsigned char* f,unsigned char* l){
            int flen = UCLen(f);
            int llen = UCLen(l);

            unsigned char* res = (unsigned char*)malloc(flen + llen);
            for(int i = 0; i < flen; i++){
                res[i] = f[i];
            }

            for(int j = 0; j < llen; j++){
                res[j+flen] = l[j];
            }

            res[flen+llen] = '\0';
            return res;
        }

        static unsigned char* UCSub(unsigned char* data,int pos,int len){
            int dlen = UCLen(data);
            unsigned char* res = (unsigned char*)malloc(len);
            if(dlen >= pos+len-1){
                for(int i = pos;i < pos+len-1;i++){
                    res[i-pos] = data[i];
                }
                return res;
            }
            free(res);
            return NULL;
        }

        static std::vector<unsigned char*> UCSplit(unsigned char* data,unsigned char* sep){
            std::vector<unsigned char*> res;
            int dlen = UCLen(data);
            int slen = UCLen(sep);
            int ll = dlen - (slen - 1);

            int sc = 0;
            int chunkcount = 1;
            unsigned char* tmp = (unsigned char*)malloc(chunkcount);
            for(int i = 0;i < dlen;i++){
                if(i >= ll && sc == 0){
                    tmp[chunkcount - 1] = data[i];
                    chunkcount++;
                    tmp = (unsigned char*)realloc(tmp,chunkcount);
                    continue;
                }

                if(data[i] == sep[sc]){
                    if(sc >= slen - 1){
                        sc = 0;
                        tmp[chunkcount - 1] = '\0';
                        res.push_back(tmp);
                        chunkcount = 1;
                        tmp = (unsigned char*)malloc(1);
                    }else{
                        sc++;
                    }
                }else{
                    tmp[chunkcount - 1] = data[i];
                    sc = 0;
                    chunkcount++;
                    tmp = (unsigned char*)realloc(tmp,chunkcount);                    
                }
            }

            tmp[chunkcount - 1] = '\0';
            res.push_back(tmp);
            return res;
        }

        static bool UCCompare(unsigned char* s1,unsigned char* s2){
            int fl = Root::UCLen(s1);
            int sl = Root::UCLen(s2);
            if(fl == sl){
                for(int i = 0; i < fl; i++){
                    if(s1[i] != s2[i]){
                        return false;
                    }
                }

                return true;
            }

            return false;
        }

        static std::string UC2Str(unsigned char* s){
            // Vulnerable
            std::string res = "";
            for(int i = 0; i < Root::UCLen(s); i++){
                res.append(1,(char)s[i]);
            }
            return res;
        }

        static unsigned char * c2uc(char* data,int len){
            unsigned char *res = (unsigned char*)malloc(0);
            for(int i = 0; i < len; i++){
                // std::cout << (int)data[i] << " = ";
                if((int)data[i] >= 0 && (int)data[i] < 128){
                    res = (unsigned char*)realloc(res,i + 1);
                    res[i] = (int)data[i];
                    std::cout << (int)data[i] << std::endl;
                }else{
                    res = (unsigned char*)realloc(res,i + 1);
                    res[i] = 256 + (int)data[i];
                    std::cout << 256 + (int)data[i] << std::endl;
                }
            }
            return res;
        }

        static std::string getEpochTime(){
            const auto t = std::chrono::system_clock::now();
            return std::to_string(std::chrono::duration_cast<std::chrono::seconds>(t.time_since_epoch()).count());
        }

        static std::string FixTime(std::string date){
            int l = date.length();
            if(l == 1){
                return "0" + date;
            }
            return date;
        }

        static std::string getFileExt(std::string filename){
            int ind = filename.find_last_of("\\/");
            if(ind != std::string::npos){
                std::string s2 = filename.substr(ind + 1);
                int dot = s2.find_last_of(".");
                if(dot != std::string::npos){
                    return s2.substr(dot + 1);
                }
            }
            return "";
        }
};

