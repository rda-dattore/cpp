#include <iostream>
#include <regex>
#include <s3.hpp>
#include <datetime.hpp>
#include <myssl.hpp>
#include <strutils.hpp>

namespace s3 {

CanonicalRequest::CanonicalRequest(std::string method,std::string uri,std::string host,std::string payload,PayloadType payload_type) : method_(),uri_(),host_(),payload_(),payload_hash_(),query_string_(),headers_(),signed_headers_(),timestamp_(),payload_type_(PayloadType::STRING_),range_bytes_(),sha_bytes_(0),sha_time_(0.)
{
  reset(method,uri,host,payload,payload_type);
}

std::string CanonicalRequest::canonical_query_string() const
{
  std::string canonical_query_string;
  auto pairs=strutils::split(strutils::substitute(query_string_,"&amp;","<!>AMP<!>"),"&");
  for (auto& pair : pairs) {
    strutils::replace_all(pair,"<!>AMP<!>","&amp;");
    strutils::replace_all(pair,"+","%20");
    strutils::replace_all(pair,"!","%21");
    strutils::replace_all(pair,",","%2C");
    strutils::replace_all(pair,"/","%2F");
    strutils::replace_all(pair,"[","%5B");
    strutils::replace_all(pair,"]","%5D");
  }
  std::sort(pairs.begin(),pairs.end(),
  [](const std::string& left,const std::string& right) -> bool
  {
    if (left <= right) {
	return true;
    }
    else {
	return false;
    }
  });
  for (const auto& pair : pairs) {
    if (!canonical_query_string.empty()) {
	canonical_query_string+="&";
    }
    canonical_query_string+=pair;
  }
  return canonical_query_string;
}

std::string CanonicalRequest::hashed() const
{
  unsigned char message_digest[32];
  myssl::sha256(method_+"\n"+uri_+"\n"+canonical_query_string()+"\n"+headers_+"\n\n"+signed_headers_+"\n"+payload_hash_,message_digest);
  return myssl::message_digest32_to_string(message_digest);
}

void CanonicalRequest::reset(std::string method,std::string uri,std::string host,std::string payload,PayloadType payload_type)
{
  if (std::regex_search(method,std::regex("^PUT([0-9]){1,}-([0-9]){1,}$"))) {
    method_="PUT";
    range_bytes_=method.substr(3);
  }
  else {
    method_=method;
    range_bytes_="";
  }
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
    unsigned char message_digest[32];
    if (payload_type_ == PayloadType::STRING_) {
	myssl::sha256(payload_,message_digest);
	payload_hash_=myssl::message_digest32_to_string(message_digest);
    }
    else if (payload_type_ == PayloadType::FILE_) {
	payload_hash_="UNSIGNED-PAYLOAD";
    }
    else if (payload_type_ == PayloadType::SIGNED_FILE_) {
	if (range_bytes_.empty()) {
	  myssl::sha256_file(payload_,message_digest,&sha_bytes_,&sha_time_);
	}
	else {
	  myssl::sha256_file_range(payload_,range_bytes_,message_digest,&sha_bytes_,&sha_time_);
	}
	payload_hash_=myssl::message_digest32_to_string(message_digest);
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
  auto env=getenv("GROUP");
  if (env != nullptr) {
    headers_+="\nx-amz-meta-group:"+std::string(env);
    signed_headers_+=";x-amz-meta-group";
  }
  env=getenv("USER");
  if (env != nullptr) {
    headers_+="\nx-amz-meta-user:"+std::string(env);
    signed_headers_+=";x-amz-meta-user";
  }
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
