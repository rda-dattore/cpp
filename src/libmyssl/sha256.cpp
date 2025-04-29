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

namespace myssl {

unsigned char *sha256(std::string s,unsigned char (&message_digest)[32])
{
  if (SHA256(reinterpret_cast<unsigned char *>(const_cast<char *>(s.c_str())),s.length(),message_digest) == nullptr) {
    message_digest[0]=0;
  }
  return message_digest;
}

unsigned char *sha256_file(std::string filename,unsigned char (&message_digest)[32],long long *benchmark_bytes,double *benchmark_time)
{
  std::ifstream ifs(filename.c_str());
  if (!ifs.is_open()) {
    message_digest[0]=0;
  }
  else {
    struct timespec time_start;
    clock_gettime(CLOCK_MONOTONIC,&time_start);
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    struct stat buf;
    stat(filename.c_str(),&buf);
    const size_t BUF_LEN=std::min(buf.st_size,static_cast<off_t>(100000000));
    std::unique_ptr<char[]> buffer(new char[BUF_LEN]);
    ifs.read(buffer.get(),BUF_LEN);
    while (!ifs.eof()) {
	SHA256_Update(&ctx,buffer.get(),ifs.gcount());
	if (benchmark_bytes != nullptr) {
	  (*benchmark_bytes)+=ifs.gcount();
	}
	ifs.read(buffer.get(),BUF_LEN);
    }
    SHA256_Update(&ctx,buffer.get(),ifs.gcount());
    if (benchmark_bytes != nullptr) {
	(*benchmark_bytes)+=ifs.gcount();
    }
    ifs.close();
    SHA256_Final(message_digest,&ctx);
    struct timespec time_end;
    clock_gettime(CLOCK_MONOTONIC,&time_end);
    if (benchmark_time != nullptr) {
	(*benchmark_time)+=(time_end.tv_sec-time_start.tv_sec)+(time_end.tv_nsec-time_start.tv_nsec)/1000000000.;
    }
  }
  return message_digest;
}

unsigned char *sha256_file_range(std::string filename,std::string range_bytes,unsigned char (&message_digest)[32],long long *benchmark_bytes,double *benchmark_time)
{
  std::ifstream ifs(filename.c_str());
  if (!ifs.is_open()) {
    message_digest[0]=0;
  }
  else {
    struct timespec time_start;
    clock_gettime(CLOCK_MONOTONIC,&time_start);
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    auto bytes=strutils::split(range_bytes,"-");
    auto file_offset=std::stoll(bytes.front());
    ifs.seekg(file_offset,std::ios::beg);
    auto num_bytes=std::stoll(bytes.back())-file_offset+1;
    const int BUF_LEN= (num_bytes < 100000000) ? num_bytes : 100000000;
    std::unique_ptr<char[]> buffer(new char[BUF_LEN]);
    ifs.read(buffer.get(),BUF_LEN);
    long long num_read=0;
    while (!ifs.eof()) {
	num_read+=ifs.gcount();
	if (num_read > num_bytes) {
	  break;
	}
	SHA256_Update(&ctx,buffer.get(),ifs.gcount());
	if (benchmark_bytes != nullptr) {
	  (*benchmark_bytes)+=ifs.gcount();
	}
	ifs.read(buffer.get(),BUF_LEN);
    }
    if (ifs.gcount() < BUF_LEN) {
	SHA256_Update(&ctx,buffer.get(),ifs.gcount());
    }
    else {
	SHA256_Update(&ctx,buffer.get(),ifs.gcount()-num_read+num_bytes);
    }
    if (benchmark_bytes != nullptr) {
	(*benchmark_bytes)+=ifs.gcount();
    }
    ifs.close();
    SHA256_Final(message_digest,&ctx);
    struct timespec time_end;
    clock_gettime(CLOCK_MONOTONIC,&time_end);
    if (benchmark_time != nullptr) {
	(*benchmark_time)+=(time_end.tv_sec-time_start.tv_sec)+(time_end.tv_nsec-time_start.tv_nsec)/1000000000.;
    }
  }
  return message_digest;
}

unsigned char *hmac_sha256(unsigned char *key,size_t key_len,std::string data,unsigned char (&message_digest)[32])
{
  unsigned int md_len;
  if (HMAC(EVP_sha256(),key,key_len,reinterpret_cast<unsigned char *>(const_cast<char *>(data.c_str())),data.length(),message_digest,&md_len) == nullptr) {
    message_digest[0]=0;
  }
  return message_digest;
}

unsigned char *hmac_sha256(std::string key,std::string data,unsigned char (&message_digest)[32])
{
  return hmac_sha256(reinterpret_cast<unsigned char *>(const_cast<char *>(key.c_str())),key.length(),data,message_digest);
}

std::string message_digest32_to_string(const unsigned char (&message_digest)[32])
{
  std::stringstream string_ss;
  string_ss << std::setfill('0') << std::hex;
  for (size_t n=0; n < 32; ++n) {
    string_ss << std::setw(2) << static_cast<int>(message_digest[n]);
  }
  string_ss << std::dec;
  return string_ss.str();
}

} // end namespace myssl
