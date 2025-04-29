#include <s3.hpp>
#include <regex>
#include <bfstream.hpp>
#include <strutils.hpp>

using std::regex;
using std::regex_search;
using std::string;
using std::stringstream;
using std::unique_ptr;
using strutils::split;

bool is3stream::open(string url) {
  if (!regex_search(url, regex("^http(s){0,1}://((.+)\\.){1,}(.+)(/(.+))"
      "{2,}"))) {
    return false;
  }
  auto x = url.find("://");
  url = url.substr(x + 3);
  auto sp = split(url, "/");
  string ak, sk;
  std::tie(ak, sk) = s3::get_credentials(sp[0]);
  session.reset(new s3::Session(sp[0], ak, sk, "us-east-1", "aws4_request"));
  bucket = sp[1];
  key = url.substr(url.find(bucket) + bucket.length() + 1);
  curr_offset = 0;
  num_read = 0;
  return true;
}

int is3stream::read(unsigned char *buffer, size_t num_bytes) {
  static unique_ptr<unsigned char[]> b;
  static size_t cap = 0;
  if (num_bytes > cap) {
    cap = num_bytes;
    b.reset(new unsigned char[cap]);
  }
  stringstream rss;
  rss << curr_offset << "-" << curr_offset + num_bytes - 1;
  string e;
  if (session->download_range(bucket, key, rss.str(), b, cap, e)) {
    std::copy(&b[0], &b[num_bytes], buffer);
    ++num_read;
    curr_offset += num_bytes;
    return num_bytes;
  } else {
    if (e == "416") {
      return bfstream::eof;
    } else {
      return bfstream::error;
    }
  }
}
