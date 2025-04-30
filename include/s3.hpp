#ifndef S3_H
#define   S3_H

#include <string>
#include <sstream>
#include <vector>
#include <curl/curl.h>
#include <xml.hpp>
#include <tempfile.hpp>

namespace s3 {

class CanonicalRequest {
public:
  enum class PayloadType { STRING_=0, FILE_, SIGNED_FILE_ };

  CanonicalRequest() = delete;
  CanonicalRequest(std::string method, std::string uri, std::string host,
      std::string payload = "", PayloadType payload_type =
      PayloadType::STRING_);
  std::string canonical_query_string() const;
  std::string date() const { return timestamp_.substr(0, 8); }
  std::string hashed() const;
  std::string headers() const { return headers_; }
  std::string host() const { return host_; }
  std::string method() const { return method_; }
  std::string payload() const { return payload_; }
  PayloadType payload_type() const { return payload_type_; }
  std::string query_string() const { return query_string_; }
  std::string range_bytes() const { return range_bytes_; }
  void reset(std::string method, std::string uri, std::string host, std::string
      payload = "", PayloadType payload_type = PayloadType::STRING_);
  void set_range(std::string range_bytes) { range_bytes_ = range_bytes; }
  std::pair<long long, double> sha_benchmark() const {
    return std::make_pair(sha_bytes_, sha_time_);
  }
  void show() const;
  std::string signed_headers() const { return signed_headers_; }
  std::string timestamp() const { return timestamp_; }
  std::string uri() const { return uri_; }

private:
  std::string method_, uri_, host_, payload_, payload_hash_;
  std::string query_string_, headers_, signed_headers_, timestamp_;
  PayloadType payload_type_;
  std::string range_bytes_;
  long long sha_bytes_;
  double sha_time_;
};

class Bucket {
public:
  Bucket() = delete;
  Bucket(std::string name);
  std::string name() const { return name_; }
  void reset(std::string name);

private:
  std::string name_;
};

class Object {
public:
  Object() = delete;
  Object(std::string key, std::string last_modified, std::string etag, long long
      size, std::string owner_name, std::string storage_class);
  std::string etag() const { return etag_; }
  std::string key() const { return key_; }
  std::string last_modified() const { return last_modified_; }
  std::string owner_name() const { return owner_name_; }
  void reset(std::string key, std::string last_modified, std::string etag,
      long long size, std::string owner_name, std::string storage_class);
  long long size() const { return size_; }
  void show() const;
  std::string storage_class() const { return storage_class_; }

private:
  std::string key_, last_modified_, etag_;
  long long size_;
  std::string owner_name_, storage_class_;
};

class Session {
public:
  Session() = delete;
  Session(std::string host, std::string access_key, std::string secret_key,
      std::string region, std::string terminal);
  bool bucket_exists(std::string bucket_name);
  std::vector<Bucket> buckets();
  std::string curl_response() const { return curl_response_; }
  std::string create_bucket(std::string bucket_name);
  bool delete_bucket(std::string bucket_name);
  bool delete_object(std::string bucket_name, std::string key);
  bool download_file(std::string bucket_name, std::string key, std::string
      filename);
  bool download_range(std::string bucket_name, std::string key, std::string
      range_bytes, std::string filename);
  bool download_range(std::string bucket_name, std::string key, std::string
      range_bytes, std::unique_ptr<unsigned char[]>& buffer, size_t& BUF_LEN,
      std::string& error);
  bool object_exists(std::string bucket_name, std::string key);
  std::string object_metadata(std::string bucket_name, std::string key);
  std::vector<Object> objects(std::string bucket_name);
  std::pair<long long, double> sha_benchmark() const {
    return std::make_pair(sha_bytes_, sha_time_);
  }
  const unsigned char *signing_key() const { return signing_key_; }
  std::pair<long long, double> upload_benchmark() const {
    return std::make_pair(upload_bytes_, upload_time_);
  }
  bool upload_file(std::string filename, std::string bucket_name, std::string
      key, std::string& error);
  bool upload_file_multi(std::string filename, std::string bucket_name,
      std::string key, std::string tmp_directory, std::string& error);
  bool upload_file_multi_threaded(std::string filename, std::string bucket_name,
      std::string key, size_t max_num_threads, std::string& error);

  friend void *thread_upload_file_multi(void *ts);

private:
  void get_bucket_xml(std::string bucket_name);
  void parse_s3_response(XMLSnippet& xmls);

  std::string host_, access_key_, secret_key_, region_, terminal_;
  CanonicalRequest canonical_request;
  unsigned char signing_key_[32];
  std::unique_ptr<CURL, void(*)(CURL *)> curl_handle_;
  std::string curl_response_;
  long long sha_bytes_, upload_bytes_;
  double sha_time_, upload_time_;
};

// set the MINIMUM_PART_SIZE to at least the MinSizeAllowed parameter of NCAR's
//   S3 storage device
const size_t MINIMUM_PART_SIZE = 5242880;

struct UploadFileThreadStruct {
  UploadFileThreadStruct() : session(nullptr), filename(), file_pos(0),
      bytes_to_transfer(0), host(), bucket_name(), key(), upload_id(),
      part_number(), upload_etag(), tid(0), tmpsize(0), req(), times(),
      sha_time(0), upload_time(0.) { }

  std::unique_ptr<Session> session;
  std::string filename;
  long long file_pos, bytes_to_transfer;
  std::string host, bucket_name, key, upload_id, part_number, upload_etag;
  pthread_t tid;
long long tmpsize;
std::string req;
std::stringstream times;
  double sha_time, upload_time;
};

extern unsigned char *signature(unsigned char *signing_key, const
    CanonicalRequest& canonical_request, std::string region, std::string
    terminal,unsigned char (&message_digest)[32]);
extern unsigned char *signing_key(std::string secret_key, std::string date,
    std::string region, std::string terminal, unsigned char
    (&message_digest)[32]);

extern std::string curl_request(const CanonicalRequest& canonical_request,
    std::string access_key, std::string region, std::string terminal,
    std::string signature);
extern std::string perform_curl_request(CURL *curl_handle, const
    CanonicalRequest& canonical_request, std::string access_key, std::string
    region, std::string terminal, std::string signature, FILE *output =
    nullptr);
extern std::string perform_curl_request(CURL *curl_handle, const
    CanonicalRequest& canonical_request, std::string access_key, std::string
    region, std::string terminal, std::string signature, std::unique_ptr<
    unsigned char[]>& buffer, size_t& BUF_LEN);

extern std::pair<std::string, std::string> get_credentials(std::string
    server_name);

extern void show_hex(std::ostream& outs, const unsigned char *s, size_t len);

} // end namespace s3

class is3stream {
public:
  is3stream() : session(nullptr), bucket(), key(), num_read(0), curr_offset(0)
      { }
  void close() { session.reset(nullptr); }
  bool is_open() const { return session != nullptr; }
  size_t number_read() const { return num_read; }
  bool open(std::string url);
  int read(unsigned char *buffer, size_t num_bytes);
  void rewind() { curr_offset = 0; }
  void seek(off_t off) { curr_offset = off; }
  off_t tell() const { return curr_offset; }

private:
  std::unique_ptr<s3::Session> session;
  std::string bucket, key;
  size_t num_read;
  off_t curr_offset;
};

#endif
