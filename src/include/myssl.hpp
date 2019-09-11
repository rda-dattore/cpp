#include <string>

extern unsigned char *hmac_sha256(unsigned char *key,size_t key_len,std::string data);
extern unsigned char *hmac_sha256(std::string key,std::string data);
extern unsigned char *sha256(std::string s);
extern unsigned char *sha256_file(std::string filename);
