// BitSwitch
// 76543210
// 15037426
// 
// 76543210
// 30624175

class Enc : public Root{
    private:
        static unsigned char* EnBitSwitch(unsigned char* text){
            int tlen = Root::UCLen(text);
            unsigned char* res = (unsigned char*)malloc(tlen);
            for(int i = 0; i < tlen;i++){
                std::string tmp = "";
                tmp += std::to_string(((text[i] >> 1)  & 0x01));
                tmp += std::to_string(((text[i] >> 5)  & 0x01));
                tmp += std::to_string(((text[i] >> 0)  & 0x01));
                tmp += std::to_string(((text[i] >> 3)  & 0x01));
                tmp += std::to_string(((text[i] >> 7)  & 0x01));
                tmp += std::to_string(((text[i] >> 4)  & 0x01));
                tmp += std::to_string(((text[i] >> 2)  & 0x01));
                tmp += std::to_string(((text[i] >> 6)  & 0x01));
                std::bitset<8> b(tmp);
                unsigned char c = (b.to_ulong() & 0xFF);
                res[i] = c;
            }
            res[tlen] = '\0';
            return res;
        }

        static unsigned char* DeBitSwitch(unsigned char* text){
            int tlen = Root::UCLen(text);
            unsigned char* res = (unsigned char*)malloc(tlen);
            for(int i = 0; i < tlen;i++){
                std::string tmp = "";
                tmp += std::to_string(((text[i] >> 3)  & 0x01));
                tmp += std::to_string(((text[i] >> 0)  & 0x01));
                tmp += std::to_string(((text[i] >> 6)  & 0x01));
                tmp += std::to_string(((text[i] >> 2)  & 0x01));
                tmp += std::to_string(((text[i] >> 4)  & 0x01));
                tmp += std::to_string(((text[i] >> 1)  & 0x01));
                tmp += std::to_string(((text[i] >> 7)  & 0x01));
                tmp += std::to_string(((text[i] >> 5)  & 0x01));
                std::bitset<8> b(tmp);
                unsigned char c = (b.to_ulong() & 0xFF);
                res[i] = c;
            }
            res[tlen] = '\0';
            return res;
        }

        static unsigned char* XORING(unsigned char* text){
            int tlen = Root::UCLen(text);
            unsigned char* res = (unsigned char*)malloc(tlen);
            std::string ck = "10111011";
            for(int i = 0; i < tlen;i++){
                std::string tmp = "";
                std::string ntmp = "";
                for(int j = 7;j >= 0;j--){
                    tmp += std::to_string(((text[i] >> j)  & 0x01));
                }

                for(int k = 0; k < 8; k++){
                    if(tmp[k] == ck[k]){
                        ntmp += "1";
                    }else{
                        ntmp += "0";
                    }
                }
                std::bitset<8> b(ntmp);
                unsigned char c = (b.to_ulong() & 0xFF);
                res[i] = c;
            }
            res[tlen] = '\0';
            return res;
        }

        static unsigned char* KeyXOR(unsigned char* text,unsigned char* key){
            int tlen = Root::UCLen(text);
            unsigned char* res = (unsigned char*)malloc(tlen);
            int klen = Root::UCLen(key);
            int counter = 0;
            for(int i = 0; i < tlen;i++){
                std::string ktmp = "";
                std::string tmp = "";
                std::string ntmp = "";
                for(int j = 7;j >= 0;j--){
                    tmp += std::to_string(((text[i] >> j)  & 0x01));
                    ktmp += std::to_string(((key[counter] >> j)  & 0x01));
                }

                for(int k = 0; k < 8; k++){
                    if(tmp[k] == ktmp[k]){
                        ntmp += "1";
                    }else{
                        ntmp += "0";
                    }
                }
                std::bitset<8> b(ntmp);
                unsigned char c = (b.to_ulong() & 0xFF);
                res[i] = c;

                if(counter >= klen - 1){
                    counter = 0;
                }else{
                    counter++;
                }
            }
            res[tlen] = '\0';
            return res;
        }
    public:
        static unsigned char* EncNK(unsigned char* text){
            return EnBitSwitch(XORING(text));
        }

        static unsigned char* DecNK(unsigned char* text){
            return XORING(DeBitSwitch(text));
        }

        static unsigned char* EncWK(unsigned char* text,unsigned char* key){
            return KeyXOR(XORING(text),key);
        }

        static unsigned char* DecWK(unsigned char* text,unsigned char* key){
            return XORING(KeyXOR(text,key));
        }
};