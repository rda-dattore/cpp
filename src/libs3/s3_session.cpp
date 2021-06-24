#include <iostream>
#include <sstream>
#include <stdexcept>
#include <regex>
#include <sys/stat.h>
#include <curl/curl.h>
#include <s3.hpp>
#include <datetime.hpp>
#include <myssl.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <timer.hpp>
#include <tempfile.hpp>

using myssl::message_digest32_to_string;
using std::move;
using std::pair;
using std::regex;
using std::runtime_error;
using std::string;
using std::stringstream;
using strutils::itos;
using strutils::replace_all;
using strutils::split;
using strutils::trim;

namespace s3 {

Session::Session(string host, string access_key, string secret_key, string
    region, string terminal) : host_(host), access_key_(access_key),
    secret_key_(secret_key), region_(region), terminal_(terminal),
    canonical_request("", "", "", "", CanonicalRequest::PayloadType::STRING_),
    signing_key_(), curl_handle_(curl_easy_init(), &curl_easy_cleanup),
    curl_response_(), sha_bytes_(0), upload_bytes_(0), sha_time_(0.),
    upload_time_(0.) {
  unsigned char md[32];
  auto s = s3::signing_key(secret_key_, canonical_request.date(), region_,
      terminal_, md);
  std::copy(&s[0], &s[32], &signing_key_[0]);
}

bool Session::bucket_exists(string bucket_name) {
  if (bucket_name.empty()) {
    return false;
  }
  get_bucket_xml(bucket_name);
  if (curl_response_.empty()) {
    return false;
  }
  XMLSnippet x;
  parse_s3_response(x);
  auto e = x.element("ListBucketResult/Name");
  if (!e.name().empty()) {
    if (e.content() == bucket_name) {
      return true;
    } else {
      throw runtime_error("bucket_exists(): unexpected xml: " + e.content());
    }
  } else {
    e = x.element("Error/Code");
    if (!e.name().empty()) {
      if (e.content() == "NoSuchBucket") {
        return false;
      } else {
        throw runtime_error("bucket_exists(): unexpected error: " +
            curl_response_);
      }
    } else {
      throw runtime_error("bucket_exists(): unexpected xml response: " +
          curl_response_);
    }
  }
}

void Session::get_bucket_xml(string bucket_name) {
  if (bucket_name.empty()) {
    return;
  }
  canonical_request.reset("GET", "/" + bucket_name, host_);
  unsigned char md[32];
  s3::signature(const_cast<unsigned char *>(signing_key_), canonical_request,
      region_, terminal_, md);
  curl_response_ = perform_curl_request(curl_handle_.get(), canonical_request,
      access_key_, region_, terminal_, message_digest32_to_string(md));
}

std::vector<Bucket> Session::buckets() {
  canonical_request.reset("GET", "/", host_);
  unsigned char md[32];
  s3::signature(const_cast<unsigned char *>(signing_key_), canonical_request,
      region_, terminal_, md);
  curl_response_ = perform_curl_request(curl_handle_.get(), canonical_request,
      access_key_, region_, terminal_,message_digest32_to_string(md));
  XMLSnippet x;
  parse_s3_response(x);
  auto e = x.element("ListAllMyBucketsResult/Buckets");
  if (e.name() == "Buckets") {
    auto blst = e.element_list("Bucket");
    std::vector<Bucket> v; // return value
    for (const auto& b : blst) {
      auto e = b.element("Name");
      v.emplace_back(e.content());
    }
    return move(v);
  } else {
    throw runtime_error("buckets(): unexpected xml response: " +
        curl_response_);
  }
}

string Session::create_bucket(string bucket_name) {
  canonical_request.reset("PUT", "/" + bucket_name, host_);
  unsigned char md[32];
  s3::signature(const_cast<unsigned char *>(signing_key_), canonical_request,
      region_, terminal_, md);
  curl_response_ = perform_curl_request(curl_handle_.get(), canonical_request,
      access_key_, region_, terminal_, message_digest32_to_string(md));
  if (curl_response_.empty()) {
    return move(bucket_name);
  }
  return "";
}

bool Session::delete_bucket(string bucket_name) {
  if (!bucket_exists(bucket_name)) {
    return false;
  }
  canonical_request.reset("DELETE", "/" + bucket_name, host_);
  unsigned char md[32];
  s3::signature(const_cast<unsigned char *>(signing_key_), canonical_request,
      region_, terminal_, md);
  curl_response_ = perform_curl_request(curl_handle_.get(), canonical_request,
      access_key_, region_, terminal_, message_digest32_to_string(md));
  if (curl_response_.empty()) {
    return true;
  }
  return false;
}

bool Session::delete_object(string bucket_name, string key) {
  if (!object_exists(bucket_name, key)) {
    return false;
  }
  canonical_request.reset("DELETE", "/" + bucket_name+"/" + key, host_);
  unsigned char md[32];
  s3::signature(const_cast<unsigned char *>(signing_key_), canonical_request,
      region_, terminal_, md);
  curl_response_ = perform_curl_request(curl_handle_.get(), canonical_request,
      access_key_, region_, terminal_, message_digest32_to_string(md));
  if (curl_response_.empty()) {
    return true;
  }
  return false;
}

bool Session::download_file(string bucket_name, string key, string filename) {
  if (bucket_name.empty() || key.empty()) {
    return false;
  }
  canonical_request.reset("HEAD", "/" + bucket_name + "/" + key, host_);
  unsigned char md[32];
  signature(const_cast<unsigned char *>(signing_key_), canonical_request,
      region_, terminal_, md);
  auto head_response = perform_curl_request(curl_handle_.get(),
      canonical_request, access_key_, region_, terminal_,
      message_digest32_to_string(md));
  string etag;
  auto headers = split(head_response, "\n");
  for (const auto& header : headers) {
    if (header.substr(0, 5) == "ETag:") {
      etag = header;
      trim(etag);
      break;
    }
  } 
  if (etag.empty()) {
    return false;
  }
  canonical_request.reset("GET", "/" + bucket_name + "/" + key, host_);
  FILE *outs = fopen(filename.c_str(), "w");
  if (outs == nullptr) {
    throw runtime_error("download_file(): could not open '" + filename +
        "' for output");
  }
  signature(const_cast<unsigned char *>(signing_key_), canonical_request,
      region_, terminal_, md);
  curl_response_ = perform_curl_request(curl_handle_.get(), canonical_request,
      access_key_, region_, terminal_, message_digest32_to_string(md), outs);
  fclose(outs);
  trim(curl_response_);
  if (curl_response_ == etag) {
    return true;
  }
  return false;
}

bool Session::download_range(string bucket_name, string key, string range_bytes,
    string filename) {
  if (bucket_name.empty() || key.empty()) {
    return false;
  }
  canonical_request.reset("GET", "/" + bucket_name+"/" + key, host_);
  canonical_request.set_range(range_bytes);
  FILE *outs=fopen(filename.c_str(), "w");
  if (outs == nullptr) {
    throw runtime_error("download_range(): could not open '" + filename +
        "' for output");
  }
  unsigned char md[32];
  s3::signature(const_cast<unsigned char *>(signing_key_), canonical_request,
      region_, terminal_, md);
  curl_response_ = perform_curl_request(curl_handle_.get(), canonical_request,
      access_key_, region_, terminal_, message_digest32_to_string(md), outs);
  fclose(outs);
  trim(curl_response_);
  if (regex_search(curl_response_, regex("^ETag: \"(.){32}\"$"))) {
    return true;
  }
  if (curl_response_.empty()) {
    std::ifstream ifs(filename);
    stringstream ss;
    ss << ifs.rdbuf();
    curl_response_ = ss.str();
  }
  return false;
}

bool Session::download_range(string bucket_name, string key, string range_bytes,
    std::unique_ptr<unsigned char[]>& buffer, size_t& buffer_capacity, string&
    error) {
  error = "";
  if (bucket_name.empty()) {
    error = "Bucket not specified";
    return false;
  }
  if (key.empty()) {
    error = "Key not specified";
    return false;
  }
  canonical_request.reset("GET", "/" + bucket_name+"/" + key, host_);
  canonical_request.set_range(range_bytes);
  unsigned char md[32];
  s3::signature(const_cast<unsigned char *>(signing_key_), canonical_request,
      region_, terminal_, md);
  curl_response_ = perform_curl_request(curl_handle_.get(), canonical_request,
      access_key_, region_, terminal_, message_digest32_to_string(md), buffer,
      buffer_capacity);
  long stat;
  curl_easy_getinfo(curl_handle_.get(), CURLINFO_RESPONSE_CODE, &stat);
  if (stat == 206) {
    return true;
  } else if (stat == 416) {
    error = itos(stat);
    return false;
  }
  error = "HTTP Status Code: " + itos(stat);
  if (!curl_response_.empty()) {
    error += " '" + curl_response_ + "'";
  }
  return false;
}

bool Session::object_exists(string bucket_name, string key) {
  if (bucket_name.empty() || key.empty()) {
    return false;
  }
  get_bucket_xml(bucket_name);
  XMLSnippet x;
  parse_s3_response(x);
  auto elist = x.element_list("ListBucketResult/Contents/Key");
  if (elist.size() == 0) {
    return false;
  }
  for (const auto& e : elist) {
    if (e.content() == key) {
      return true;
    }
  }
  return false;
}

string Session::object_metadata(string bucket_name, string key) {
  if (bucket_name.empty()) {
    return "No bucket specified";
  }
  if (key.empty()) {
    return "No key specified";
  }
  canonical_request.reset("HEAD", "/" + bucket_name + "/" + key, host_);
  unsigned char md[32];
  s3::signature(const_cast<unsigned char *>(signing_key_), canonical_request,
      region_, terminal_, md);
  curl_response_ = perform_curl_request(curl_handle_.get(), canonical_request,
      access_key_, region_, terminal_, message_digest32_to_string(md));
  string s; // return value
  if (!curl_response_.empty()) {
    auto sp = split(curl_response_, "\r\n");
    for (const auto& e : sp) {
      auto kv = split(e, ": ");
      if (kv.size() == 2) {
        if (!s.empty()) {
          s += ", ";
        }
        s += "\"" + kv.front() + "\": \"" + strutils::substitute(kv.back(),
            "\"", "\\\"") + "\"";
      }
    }
    s = "{" + s + "}";
  }
  return move(s);
}

std::vector<Object> Session::objects(string bucket_name) {
  std::vector<Object> v; // return value
  if (!bucket_name.empty()) {
    auto trunc = true;
    string next;
    while (trunc) {
      canonical_request.reset("GET", "/" + bucket_name + "?delimiter=!&marker="
          + next, host_);
      unsigned char md[32];
      s3::signature(const_cast<unsigned char *>(signing_key_),
          canonical_request, region_, terminal_, md);
      curl_response_ = perform_curl_request(curl_handle_.get(),
          canonical_request, access_key_, region_, terminal_,
          message_digest32_to_string(md));
      XMLSnippet x;
      parse_s3_response(x);
      auto elist = x.element_list("ListBucketResult/Contents");
      for (const auto& e : elist) {
        v.emplace_back(e.element("Key").content(), e.element("LastModified")
            .content(), e.element("ETag").content(), std::stoll(e.element(
            "Size").content()), e.element("Owner/DisplayName").content(), e
            .element("StorageClass").content());
      }
      trunc = strutils::to_lower(x.element("ListBucketResult/IsTruncated")
          .content()) == "true" ? true : false;
      if (trunc) {
        next = x.element("ListBucketResult/NextMarker").content();
      }
    }
  }
  return move(v);
}

void Session::parse_s3_response(XMLSnippet& xmls) {
  if (!curl_response_.empty()) {
    auto x = curl_response_.find("?>");
    if (x != string::npos) {
      xmls.fill(curl_response_.substr(x + 2));
    } else {
      xmls.fill(curl_response_);
    }
    if (!xmls) {
      throw runtime_error("Response: '" + curl_response_ + "'\nParse error: '" +
          xmls.parse_error() + "'");
    }
  }
}

bool Session::upload_file(string filename, string bucket_name, string key,
    string& error) {
  error = "";
  struct stat buf;
  if (stat(filename.c_str(), &buf) != 0) {
    error = "Local file does not exist";
    return false;
  }
  canonical_request.reset("PUT", "/" + bucket_name + "/" + key, host_, filename,
      CanonicalRequest::PayloadType::FILE_);
  std::tie(sha_bytes_, sha_time_) = canonical_request.sha_benchmark();
  unsigned char md[32];
  struct timespec t1;
  clock_gettime(CLOCK_MONOTONIC, &t1);
  s3::signature(const_cast<unsigned char *>(signing_key_), canonical_request,
      region_, terminal_, md);
  curl_response_ = perform_curl_request(curl_handle_.get(), canonical_request,
      access_key_, region_, terminal_, message_digest32_to_string(md));
  struct timespec t2;
  clock_gettime(CLOCK_MONOTONIC, &t2);
  upload_bytes_ += buf.st_size;
  upload_time_ += (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) /
      1000000000.;
  long stat;
  curl_easy_getinfo(curl_handle_.get(), CURLINFO_RESPONSE_CODE, &stat);
  if (stat == 200) {
    return true;
  }
  error = itos(stat);
  if (!curl_response_.empty()) {
    error += " '" + curl_response_ + "'";
  }
  return false;
}

bool Session::upload_file_multi(string filename, string bucket_name, string key,
    string tmp_directory, string& error) {
  struct stat buf;
  if (stat(filename.c_str(), &buf) != 0) {
    error = "Local file does not exist";
    return false;
  }
  canonical_request.reset("POST", "/" + bucket_name + "/" + key + "?uploads=",
      host_);
  unsigned char md[32];
  s3::signature(const_cast<unsigned char *>(signing_key_), canonical_request,
      region_, terminal_, md);
  curl_response_ = perform_curl_request(curl_handle_.get(), canonical_request,
      access_key_, region_, terminal_, message_digest32_to_string(md));
  XMLSnippet x;
  parse_s3_response(x);
  auto b = x.element("InitiateMultipartUploadResult/Bucket");
  auto k = x.element("InitiateMultipartUploadResult/Key");
  auto uid = x.element("InitiateMultipartUploadResult/UploadId");
  if (!b.name().empty() && b.content() == bucket_name && !k.name().empty() && k
      .content() == key && !uid.name().empty()) {
    auto u = uid.content();
    const size_t BUF_LEN = 500000000;
    auto np = (buf.st_size / BUF_LEN) + 1;
    std::ifstream ifs(filename.c_str());
    if (!ifs.is_open()) {
      throw runtime_error("upload_file_multi(): unable to open '" + filename +
          "' for upload");
    }
    std::unique_ptr<char[]> buffer(new char[BUF_LEN]);
    TempFile tfile(tmp_directory);
    auto etag_re = regex("^ETag:");
    string p = "<CompleteMultipartUpload>";
    for (size_t n = 0; n < np; ++n) {
      std::ofstream ofs(tfile.name());
      if (!ofs.is_open()) {
        throw runtime_error("upload_file_multi(): unable to create temporary "
            "file in '" + tmp_directory + "'");
      }
      ifs.read(buffer.get(), BUF_LEN);
      ofs.write(buffer.get(), ifs.gcount());
      ofs.close();
      auto part_number = itos(n + 1);
      canonical_request.reset("PUT", "/" + bucket_name + "/" + key +
          "?partNumber=" + part_number + "&uploadId=" + u, host_, tfile.name(),
          CanonicalRequest::PayloadType::FILE_);
      unsigned char md[32];
      s3::signature(const_cast<unsigned char *>(signing_key_),
          canonical_request, region_, terminal_, md);
      curl_response_ = perform_curl_request(curl_handle_.get(),
          canonical_request, access_key_, region_, terminal_,
          message_digest32_to_string(md));
      auto sp = split(curl_response_, "\n");
      for (const auto& e : sp) {
        if (regex_search(e, etag_re)) {
          auto etag = split(e, ":");
          trim(etag.back());
          replace_all(etag.back(), "\"", "");
          p += "<Part><PartNumber>" + part_number + "</PartNumber><ETag>" + etag
              .back() + "</ETag></Part>";
          break;
        }
      }
    }
    p += "</CompleteMultipartUpload>";
    canonical_request.reset("POST", "/" + bucket_name + "/" + key + "?uploadId="
        + u, host_, p);
    unsigned char md[32];
    s3::signature(const_cast<unsigned char *>(signing_key_), canonical_request,
        region_, terminal_, md);
    curl_response_ = perform_curl_request(curl_handle_.get(), canonical_request,
        access_key_, region_, terminal_, message_digest32_to_string(md));
    XMLSnippet x;
    parse_s3_response(x);
    b = x.element("CompleteMultipartUploadResult/Bucket");
    k = x.element("CompleteMultipartUploadResult/Key");
    if (!b.name().empty() && b.content() == bucket_name && !k.name().empty() &&
        k.content() == key) {
      return true;
    }
    error = curl_response_;
    return false;
  }
  throw runtime_error("upload_file_multi(): unexpected response: " +
      curl_response_);
}

void *thread_upload_file_multi(void *ts) {
  auto *t = reinterpret_cast<UploadFileThreadStruct *>(ts);
  struct timespec t1;
  clock_gettime(CLOCK_MONOTONIC, &t1);
  t->session->canonical_request.reset("PUT" + strutils::lltos(t->file_pos) + "-"
      + strutils::lltos(t->file_pos + t->bytes_to_transfer - 1), "/" + t->
      bucket_name + "/" + t->key + "?partNumber=" + t->part_number +
      "&uploadId=" + t->upload_id, t->host, t->filename, CanonicalRequest::
      PayloadType::FILE_);
  struct timespec t2;
  clock_gettime(CLOCK_MONOTONIC, &t2);
  t->sha_time = (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) /
      1000000000.;
  unsigned char md[32];
  clock_gettime(CLOCK_MONOTONIC, &t1);
  s3::signature(const_cast<unsigned char *>(t->session->signing_key_), t->
      session->canonical_request, t->session->region_, t->session->terminal_,
      md);
  t->session->curl_response_ = perform_curl_request(t->session->curl_handle_
      .get(), t->session->canonical_request, t->session->access_key_, t->
      session->region_, t->session->terminal_, message_digest32_to_string(md));
  clock_gettime(CLOCK_MONOTONIC, &t2);
  t->upload_time = (t2.tv_sec - t1.tv_sec) + (t2.tv_nsec - t1.tv_nsec) /
      1000000000.;
  auto sp=split(t->session->curl_response_, "\n");
  for (const auto& e : sp) {
    if (regex_search(e, regex("^ETag:"))) {
      auto sp2=split(e, ":");
      t->upload_etag = sp2.back();
      trim(t->upload_etag);
      replace_all(t->upload_etag, "\"", "");
      break;
    }
  }
  return nullptr;
}

bool Session::upload_file_multi_threaded(string filename, string bucket_name,
    string key, size_t max_num_threads, string& error) {
  struct stat buf;
  if (stat(filename.c_str(), &buf) != 0) {
    error = "Local file does not exist";
    return false;
  }
  canonical_request.reset("POST", "/" + bucket_name + "/" + key + "?uploads=",
      host_);
  unsigned char md[32];
  s3::signature(const_cast<unsigned char *>(signing_key_), canonical_request,
      region_, terminal_, md);
  curl_response_ = perform_curl_request(curl_handle_.get(), canonical_request,
      access_key_, region_, terminal_, message_digest32_to_string(md));
  XMLSnippet x;
  parse_s3_response(x);
  auto b = x.element("InitiateMultipartUploadResult/Bucket");
  auto k = x.element("InitiateMultipartUploadResult/Key");
  auto uid = x.element("InitiateMultipartUploadResult/UploadId");
  if (!b.name().empty() && b.content() == bucket_name && !k.name().empty() && k
      .content() == key && !uid.name().empty()) {
    auto u = uid.content();
    size_t s = buf.st_size / max_num_threads;
    if (s < MINIMUM_PART_SIZE) {
      max_num_threads = lround(1. + static_cast<double>(buf.st_size) /
          MINIMUM_PART_SIZE);
      s = buf.st_size / max_num_threads;
    }
    if (max_num_threads == 1) {

      // if the file only requires one thread, use the unthreaded upload
      return upload_file(filename,bucket_name,key,error);
    }
    const size_t NUM_THREADS_1 = max_num_threads-1;
    std::vector<UploadFileThreadStruct> threads(max_num_threads);
    for (size_t n = 0; n < NUM_THREADS_1; ++n) {
      threads[n].session.reset(new Session(host_, access_key_, secret_key_,
          region_, terminal_));
      threads[n].filename = filename;
      threads[n].file_pos = n * s;
      threads[n].bytes_to_transfer = s;
      threads[n].host = host_;
      threads[n].bucket_name = bucket_name;
      threads[n].key = key;
      threads[n].upload_id = u;
      threads[n].part_number = itos(n + 1);
      pthread_create(&threads[n].tid, nullptr, thread_upload_file_multi,
          &threads[n]);
    }
    threads[NUM_THREADS_1].session.reset(new Session(host_, access_key_,
        secret_key_, region_, terminal_));
    threads[NUM_THREADS_1].filename = filename;
    threads[NUM_THREADS_1].file_pos = NUM_THREADS_1 * s;
    threads[NUM_THREADS_1].bytes_to_transfer = buf.st_size - s * NUM_THREADS_1;
    threads[NUM_THREADS_1].host = host_;
    threads[NUM_THREADS_1].bucket_name = bucket_name;
    threads[NUM_THREADS_1].key = key;
    threads[NUM_THREADS_1].upload_id = u;
    threads[NUM_THREADS_1].part_number = itos(max_num_threads);
    pthread_create(&threads[NUM_THREADS_1].tid, nullptr,
        thread_upload_file_multi, &threads[NUM_THREADS_1]);
    string p = "<CompleteMultipartUpload>";
    for (size_t n = 0; n < max_num_threads; ++n) {
      pthread_join(threads[n].tid, nullptr);
      p += "<Part><PartNumber>" + threads[n].part_number + "</PartNumber><ETag>"
          + threads[n].upload_etag + "</ETag></Part>";
      sha_bytes_+=threads[n].bytes_to_transfer;
      sha_time_+=threads[n].sha_time;
      upload_bytes_+=threads[n].bytes_to_transfer;
      upload_time_+=threads[n].upload_time;
    }
    p += "</CompleteMultipartUpload>";
    canonical_request.reset("POST", "/" + bucket_name + "/" + key + "?uploadId="
        + u, host_, p);
    unsigned char md[32];
    s3::signature(const_cast<unsigned char *>(signing_key_), canonical_request,
        region_, terminal_, md);
    curl_response_ = perform_curl_request(curl_handle_.get(), canonical_request,
        access_key_, region_, terminal_, message_digest32_to_string(md));
    XMLSnippet x;
    parse_s3_response(x);
    b = x.element("CompleteMultipartUploadResult/Bucket");
    k = x.element("CompleteMultipartUploadResult/Key");
    if (!b.name().empty() && b.content() == bucket_name && !k.name().empty() &&
        k.content() == key) {
      return true;
    }
    error = curl_response_;
    return false;
  } else {
    throw runtime_error("upload_file_multi_threaded(): unexpected response: " +
        curl_response_);
  }
}

pair<string, string> get_credentials(string server_name) {
  pair<string, string> p; // return value
  auto e = getenv("HOME");
  if (e == nullptr) {
    throw runtime_error("HOME: undefined variable.");
  }
  std::ifstream ifs(string(e) + "/.s3config");
  if (!ifs.is_open()) {
    throw runtime_error("no .s3config in HOME directory.");
  }
  auto s_re = regex("\\[((.){1,}\\.){1,}(.){1,}\\]");
  std::string cs;
  auto ak_re = regex("access_key\\s*=");
  auto sk_re = regex("secret_key\\s*=");
  char l[80];
  ifs.getline(l, 80);
  while (!ifs.eof()) {
    if (regex_search(l, s_re)) {
      cs = l;
      replace_all(cs, "[", "");
      replace_all(cs, "]", "");
      trim(cs);
    } else if (regex_search(l, ak_re)) {
      if (cs == server_name) {
        p.first = l;
        replace_all(p.first, "access_key", "");
        trim(p.first);
        if (p.first[0] == '=') {
          p.first.erase(0, 1);
          trim(p.first);
        }
      }
    } else if (regex_search(l, sk_re)) {
      if (cs == server_name) {
        p.second = l;
        replace_all(p.second, "secret_key", "");
        trim(p.second);
        if (p.second[0] == '=') {
          p.second.erase(0, 1);
          trim(p.second);
        }
      }
    }
    if (cs == server_name && !p.first.empty() && !p.second.empty()) {
      break;
    }
    ifs.getline(l, 80);
  }
  ifs.close();
  return move(p);
}

} // end namespace s3
