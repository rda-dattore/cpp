#include <string>
#include <vector>
#include <curl/curl.h>
#include <xml.hpp>

namespace s3 {

class CanonicalRequest
{
public:
  enum class PayloadType {STRING_=0,FILE_};

  CanonicalRequest() = delete;
  CanonicalRequest(std::string method,std::string uri,std::string host,std::string payload = "",PayloadType payload_type = PayloadType::STRING_);
  std::string date() const { return timestamp_.substr(0,8); }
  std::string hashed() const;
  std::string headers() const { return headers_; }
  std::string host() const { return host_; }
  std::string method() const { return method_; }
  std::string payload() const { return payload_; }
  PayloadType payload_type() const { return payload_type_; }
  std::string query_string() const { return query_string_; }
  std::string range_bytes() const { return range_bytes_; }
  void reset(std::string method,std::string uri,std::string host,std::string payload = "",PayloadType payload_type = PayloadType::STRING_);
  void set_range(std::string range_bytes) { range_bytes_=range_bytes; }
  void show() const;
  std::string signed_headers() const { return signed_headers_; }
  std::string timestamp() const { return timestamp_; }
  std::string uri() const { return uri_; }

private:
  std::string method_,uri_,host_,payload_,payload_hash_;
  std::string query_string_,headers_,signed_headers_,timestamp_;
  PayloadType payload_type_;
  std::string range_bytes_;
};

class Bucket
{
public:
  Bucket() = delete;
  Bucket(std::string name);
  std::string name() const { return name_; }
  void reset(std::string name);

private:
  std::string name_;
};

class Object
{
public:
  Object() = delete;
  Object(std::string key,std::string last_modified,std::string etag,long long size,std::string owner_name,std::string storage_class);
  std::string etag() const { return etag_; }
  std::string key() const { return key_; }
  std::string last_modified() const { return last_modified_; }
  std::string owner_name() const { return owner_name_; }
  void reset(std::string key,std::string last_modified,std::string etag,long long size,std::string owner_name,std::string storage_class);
  long long size() const { return size_; }
  void show() const;
  std::string storage_class() const { return storage_class_; }

private:
  std::string key_,last_modified_,etag_;
  long long size_;
  std::string owner_name_,storage_class_;
};

class Session
{
public:
  Session() = delete;
  Session(std::string host,std::string access_key,std::string secret_key,std::string region,std::string terminal);
  bool bucket_exists(std::string bucket_name);
  std::vector<Bucket> buckets();
  std::string curl_response() const { return curl_response_; }
  std::string create_bucket(std::string bucket_name);
  bool delete_bucket(std::string bucket_name);
  bool delete_object(std::string bucket_name,std::string key);
  bool download_file(std::string bucket_name,std::string key,std::string filename);
  bool download_range(std::string bucket_name,std::string key,std::string range_bytes,std::string filename);
  std::vector<Object> objects(std::string bucket_name);
  bool object_exists(std::string bucket_name,std::string key);
  const unsigned char *signing_key() const { return signing_key_; }
  bool upload_file(std::string filename,std::string bucket_name,std::string key);
  bool upload_file_multi(std::string filename,std::string bucket_name,std::string key,std::string tmp_directory);

private:
  void get_bucket_xml(std::string bucket_name);
  void parse_s3_response(XMLSnippet& xmls);

  std::string host_,access_key_,secret_key_,region_,terminal_;
  CanonicalRequest canonical_request;
  unsigned char signing_key_[32];
  std::unique_ptr<CURL,void(*)(CURL *)> curl_handle_;
  std::string curl_response_;
};

extern unsigned char *signature(unsigned char *signing_key,const CanonicalRequest& canonical_request,std::string region,std::string terminal);
extern unsigned char *signing_key(std::string secret_key,std::string date,std::string region,std::string terminal);

extern std::string curl_request(const CanonicalRequest& canonical_request,std::string access_key,std::string region,std::string terminal,std::string signature);
extern std::string perform_curl_request(CURL *curl_handle,const CanonicalRequest& canonical_request,std::string access_key,std::string region,std::string terminal,std::string signature,FILE *output = nullptr);
extern std::string uhash_to_string(const unsigned char *uhash,size_t len);

extern void show_hex(std::ostream& outs,const unsigned char *s,size_t len);

} // end namespace s3
