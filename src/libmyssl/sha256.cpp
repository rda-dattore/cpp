#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <memory>
#include <sstream>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <sys/stat.h>
#include <strutils.hpp>

using std::min;
using std::stoll;
using std::string;
using std::stringstream;
using std::unique_ptr;
using strutils::split;

namespace myssl {

unsigned char *sha256(string s, unsigned char (&message_digest)[32]) {
  const unsigned char *d = reinterpret_cast<const unsigned char *>(s.c_str());
  if (SHA256(d, s.length(), message_digest) == nullptr) {
    message_digest[0] = 0;
  }
  return message_digest;
}

unsigned char *sha256_file(string filename, unsigned char (&message_digest)[32],
    long long *benchmark_bytes, double *benchmark_time) {
  std::ifstream ifs(filename.c_str());
  if (!ifs.is_open()) {
    message_digest[0] = 0;
    return message_digest;
  }
  struct timespec time_start;
  clock_gettime(CLOCK_MONOTONIC, &time_start);
  SHA256_CTX ctx;
  SHA256_Init(&ctx);
  struct stat buf;
  stat(filename.c_str(), &buf);
  const size_t BUF_LEN = min(buf.st_size, static_cast<off_t>(100000000));
  unique_ptr<char[]> buffer(new char[BUF_LEN]);
  ifs.read(buffer.get(), BUF_LEN);
  while (!ifs.eof()) {
    SHA256_Update(&ctx, buffer.get(), ifs.gcount());
    if (benchmark_bytes != nullptr) {
      (*benchmark_bytes) += ifs.gcount();
    }
    ifs.read(buffer.get(), BUF_LEN);
  }
  SHA256_Update(&ctx, buffer.get(), ifs.gcount());
  if (benchmark_bytes != nullptr) {
    (*benchmark_bytes) += ifs.gcount();
  }
  ifs.close();
  SHA256_Final(message_digest, &ctx);
  struct timespec time_end;
  clock_gettime(CLOCK_MONOTONIC, &time_end);
  if (benchmark_time != nullptr) {
    (*benchmark_time) += (time_end.tv_sec - time_start.tv_sec) + (time_end.
        tv_nsec - time_start.tv_nsec) / 1000000000.;
  }
  return message_digest;
}

unsigned char *sha256_file_range(string filename, string range_bytes, unsigned
    char (&message_digest)[32], long long *benchmark_bytes, double
    *benchmark_time) {
  std::ifstream ifs(filename.c_str());
  if (!ifs.is_open()) {
    message_digest[0]=0;
    return message_digest;
  }
  struct timespec time_start;
  clock_gettime(CLOCK_MONOTONIC, &time_start);
  SHA256_CTX ctx;
  SHA256_Init(&ctx);
  auto bytes = split(range_bytes, "-");
  auto file_offset = stoll(bytes.front());
  ifs.seekg(file_offset, std::ios::beg);
  auto num_bytes = stoll(bytes.back()) - file_offset + 1;
  const int BUF_LEN = (num_bytes < 100000000) ? num_bytes : 100000000;
  unique_ptr<char[]> buffer(new char[BUF_LEN]);
  ifs.read(buffer.get(), BUF_LEN);
  long long num_read = 0;
  while (!ifs.eof()) {
    num_read += ifs.gcount();
    if (num_read > num_bytes) {
      break;
    }
    SHA256_Update(&ctx, buffer.get(), ifs.gcount());
    if (benchmark_bytes != nullptr) {
      (*benchmark_bytes) += ifs.gcount();
    }
    ifs.read(buffer.get(), BUF_LEN);
  }
  if (ifs.gcount() < BUF_LEN) {
    SHA256_Update(&ctx, buffer.get(), ifs.gcount());
  } else {
    SHA256_Update(&ctx, buffer.get(), ifs.gcount() - num_read + num_bytes);
  }
  if (benchmark_bytes != nullptr) {
    (*benchmark_bytes) += ifs.gcount();
  }
  ifs.close();
  SHA256_Final(message_digest, &ctx);
  struct timespec time_end;
  clock_gettime(CLOCK_MONOTONIC, &time_end);
  if (benchmark_time != nullptr) {
    (*benchmark_time) += (time_end.tv_sec - time_start.tv_sec) + (time_end.
        tv_nsec - time_start.tv_nsec) / 1000000000.;
  }
  return message_digest;
}

unsigned char *hmac_sha256(unsigned char *key, size_t key_len, string data,
    unsigned char (&message_digest)[32]) {
  const unsigned char *d = reinterpret_cast<const unsigned char *>(
      data.c_str());
  unsigned int md_len;
  if (HMAC(EVP_sha256(), key, key_len, d, data.length(), message_digest,
      &md_len) == nullptr) {
    message_digest[0] = 0;
  }
  return message_digest;
}

unsigned char *hmac_sha256(string key, string data, unsigned char
    (&message_digest)[32]) {
  unsigned char *d = reinterpret_cast<unsigned char *>(const_cast<char *>(key.
      c_str()));
  return hmac_sha256(d, key.length(), data, message_digest);
}

string message_digest32_to_string(const unsigned char (&message_digest)[32]) {
  stringstream string_ss;
  string_ss << std::setfill('0') << std::hex;
  for (size_t n = 0; n < 32; ++n) {
    string_ss << std::setw(2) << static_cast<int>(message_digest[n]);
  }
  string_ss << std::dec;
  return string_ss.str();
}

} // end namespace myssl
