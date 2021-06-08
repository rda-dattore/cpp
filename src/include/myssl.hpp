#ifndef MYSSL_H
#define   MYSSL_H

#include <string>

namespace myssl {

extern unsigned char *blake3_file(std::string filename, unsigned char
    (&message_digest)[32], long long *benchmark_bytes = nullptr, double
    *benchmark_time = nullptr);
extern unsigned char *hmac_sha256(unsigned char *key, size_t key_len,
    std::string data, unsigned char (&message_digest)[32]);
extern unsigned char *hmac_sha256(std::string key, std::string data, unsigned
    char (&message_digest)[32]);
extern unsigned char *sha256(std::string s, unsigned char (&message_digest)[
    32]);
extern unsigned char *sha256_file(std::string filename, unsigned char
    (&message_digest)[32], long long *benchmark_bytes = nullptr, double
    *benchmark_time = nullptr);
extern unsigned char *sha256_file_range(std::string filename, std::string
    range_bytes, unsigned char (&message_digest)[32], long long *benchmark_bytes
    = nullptr, double *benchmark_time = nullptr);

extern std::string message_digest32_to_string(const unsigned char
    (&message_digest)[32]);

} // end namespace myssl;

#endif
