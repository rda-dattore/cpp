#include <iostream>
#include <s3.hpp>
#include <datetime.hpp>
#include <myssl.hpp>
#include <strutils.hpp>

namespace s3 {

CanonicalRequest::CanonicalRequest(std::string method,std::string uri,std::string host,std::string payload,PayloadType payload_type) : method_(),uri_(),host_(),payload_(),payload_hash_(),query_string_(),headers_(),signed_headers_(),timestamp_(),payload_type_(PayloadType::STRING_),range_bytes_()
{
  reset(method,uri,host,payload,payload_type);
}

std::string CanonicalRequest::hashed() const
{
  return uhash_to_string(sha256(method_+"\n"+uri_+"\n"+query_string_+"\n"+headers_+"\n\n"+signed_headers_+"\n"+payload_hash_),32);
}

void CanonicalRequest::reset(std::string method,std::string uri,std::string host,std::string payload,PayloadType payload_type)
{
  method_=method;
  auto idx=uri.find("?");
  if (idx != std::string::npos) {
    uri_=uri.substr(0,idx);
    query_string_=uri.substr(idx+1);
  }
  else {
    uri_=uri;
    query_string_="";
  }
  host_=host;
  headers_="host:"+host+"\nx-amz-content-sha256:";
  payload_=payload;
  payload_type_=payload_type;
  if (payload_.empty()) {
    payload_hash_="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
  }
  else {
    if (payload_type_ == PayloadType::STRING_) {
	payload_hash_=uhash_to_string(sha256(payload_),32);
    }
    else if (payload_type_ == PayloadType::FILE_) {
	payload_hash_=uhash_to_string(sha256_file(payload_),32);
    }
  }
  headers_+=payload_hash_;
  auto dt=dateutils::current_date_time();
  if (dt.utc_offset() > 0) {
    dt.subtract_hours(dt.utc_offset()/100);
  }
  else {
    dt.add_hours(-dt.utc_offset()/100);
  }
  timestamp_=dt.to_string("%Y%m%dT%H%MM%SSZ");
  headers_+="\nx-amz-date:"+timestamp_;
  signed_headers_="host;x-amz-content-sha256;x-amz-date";
  range_bytes_="";
}

void CanonicalRequest::show() const
{
  std::cout << method_ << std::endl;
  std::cout << uri_ << std::endl;
  std::cout << query_string_ << std::endl;
  std::cout << headers_ << std::endl;
  std::cout << std::endl;
  std::cout << signed_headers_ << std::endl;
  std::cout << payload_hash_ << std::endl;
}

} // end namespace s3
