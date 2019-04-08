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
#include <tempfile.hpp>

namespace s3 {

Session::Session(std::string host,std::string access_key,std::string secret_key,std::string region,std::string terminal) : host_(),access_key_(),secret_key_(),region_(),terminal_(),canonical_request("","","","",CanonicalRequest::PayloadType::STRING_),signing_key_(),curl_handle_(curl_easy_init(),&curl_easy_cleanup),curl_response_()
{
  host_=host;
  access_key_=access_key;
  secret_key_=secret_key;
  region_=region;
  terminal_=terminal;
  auto skey=s3::signing_key(secret_key_,canonical_request.date(),region_,terminal_);
  std::copy(&skey[0],&skey[32],&signing_key_[0]);
}

bool Session::bucket_exists(std::string bucket_name)
{
  if (bucket_name.empty()) {
    return false;
  }
  get_bucket_xml(bucket_name);
  if (curl_response_.empty()) {
    return false;
  }
  else {
    XMLSnippet xmls;
    parse_s3_response(xmls);
    auto e=xmls.element("ListBucketResult/Name");
    if (!e.name().empty()) {
	if (e.content() == bucket_name) {
	  return true;
	}
	else {
	  throw std::runtime_error("bucket_exists(): unexpected xml: "+e.content());
	}
    }
    else {
	e=xmls.element("Error/Code");
	if (!e.name().empty()) {
	  if (e.content() == "NoSuchBucket") {
	    return false;
	  }
	  else {
	    throw std::runtime_error("bucket_exists(): unexpected error: "+curl_response_);
	  }
	}
	else {
	  throw std::runtime_error("bucket_exists(): unexpected xml response: "+curl_response_);
	}
    }
  }
}

void Session::get_bucket_xml(std::string bucket_name)
{
  if (bucket_name.empty()) {
    return;
  }
  canonical_request.reset("GET","/"+bucket_name,host_);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,s3::uhash_to_string(s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_),32));
}

std::vector<Bucket> Session::buckets()
{
  canonical_request.reset("GET","/",host_);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,s3::uhash_to_string(s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_),32));
  XMLSnippet xmls;
  parse_s3_response(xmls);
  auto e=xmls.element("ListAllMyBucketsResult/Buckets");
  if (e.name() == "Buckets") {
    auto bucket_list=e.element_list("Bucket");
    std::vector<Bucket> buckets;
    for (const auto& bucket : bucket_list) {
	auto e=bucket.element("Name");
	buckets.emplace_back(e.content());
    }
    return buckets;
  }
  else {
    throw std::runtime_error("buckets(): unexpected xml response: "+curl_response_);
  }
}

std::string Session::create_bucket(std::string bucket_name)
{
  canonical_request.reset("PUT","/"+bucket_name,host_);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,s3::uhash_to_string(s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_),32));
  if (curl_response_.empty()) {
    return bucket_name;
  }
  else {
    return "";
  }
}

bool Session::delete_bucket(std::string bucket_name)
{
  if (!bucket_exists(bucket_name)) {
    return false;
  }
  canonical_request.reset("DELETE","/"+bucket_name,host_);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,s3::uhash_to_string(s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_),32));
  if (curl_response_.empty()) {
    return true;
  }
  else {
    return false;
  }
}

bool Session::delete_object(std::string bucket_name,std::string key)
{
  if (!object_exists(bucket_name,key)) {
    return false;
  }
  canonical_request.reset("DELETE","/"+bucket_name+"/"+key,host_);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,s3::uhash_to_string(s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_),32));
  if (curl_response_.empty()) {
    return true;
  }
  else {
    return false;
  }
}

bool Session::download_file(std::string bucket_name,std::string key,std::string filename)
{
  if (bucket_name.empty() || key.empty()) {
    return false;
  }
  canonical_request.reset("GET","/"+bucket_name+"/"+key,host_);
  FILE *output=fopen(filename.c_str(),"w");
  if (output == nullptr) {
    throw std::runtime_error("download_file(): could not open '"+filename+"' for output");
  }
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,s3::uhash_to_string(s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_),32),output);
  fclose(output);
  if (curl_response_.empty()) {
    return true;
  }
  else {
    return false;
  }
}

bool Session::download_range(std::string bucket_name,std::string key,std::string range_bytes,std::string filename)
{
  if (bucket_name.empty() || key.empty()) {
    return false;
  }
  canonical_request.reset("GET","/"+bucket_name+"/"+key,host_);
  canonical_request.set_range(range_bytes);
  FILE *output=fopen(filename.c_str(),"w");
  if (output == nullptr) {
    throw std::runtime_error("download_range(): could not open '"+filename+"' for output");
  }
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,s3::uhash_to_string(s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_),32),output);
  fclose(output);
  if (curl_response_.empty()) {
    return true;
  }
  else {
    return false;
  }
}

std::vector<Object> Session::objects(std::string bucket_name)
{
  std::vector<Object> objects;
  if (!bucket_name.empty()) {
    canonical_request.reset("GET","/"+bucket_name,host_);
    curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,s3::uhash_to_string(s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_),32));
    XMLSnippet xmls;
    parse_s3_response(xmls);
    auto elist=xmls.element_list("ListBucketResult/Contents");
    for (const auto& e : elist) {
	objects.emplace_back(e.element("Key").content(),e.element("LastModified").content(),e.element("ETag").content(),std::stoll(e.element("Size").content()),e.element("Owner/DisplayName").content(),e.element("StorageClass").content());
    }
  }
  return objects;
}

bool Session::object_exists(std::string bucket_name,std::string key)
{
  if (bucket_name.empty() || key.empty()) {
    return false;
  }
  get_bucket_xml(bucket_name);
  XMLSnippet xmls;
  parse_s3_response(xmls);
  auto elist=xmls.element_list("ListBucketResult/Contents/Key");
  if (elist.size() == 0) {
    return false;
  }
  else {
    for (const auto& e : elist) {
	if (e.content() == key) {
	  return true;
	}
    }
    return false;
  }
}

void Session::parse_s3_response(XMLSnippet& xmls)
{
  if (!curl_response_.empty()) {
    auto idx=curl_response_.find("?>\n");
    if (idx != std::string::npos) {
	xmls.fill(curl_response_.substr(idx+3));
    }
    else {
      xmls.fill(curl_response_);
    }
    if (!xmls) {
	throw std::runtime_error("Response: '"+curl_response_+"'\nParse error: '"+xmls.parse_error()+"'");
    }
  }
}

bool Session::upload_file(std::string filename,std::string bucket_name,std::string key)
{
  struct stat buf;
  if (stat(filename.c_str(),&buf) != 0) {
    return false;
  }
  canonical_request.reset("PUT","/"+bucket_name+"/"+key,host_,filename,CanonicalRequest::PayloadType::FILE_);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,s3::uhash_to_string(s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_),32));
  if (curl_response_.empty()) {
    return true;
  }
  else {
    return false;
  }
}

bool Session::upload_file_multi(std::string filename,std::string bucket_name,std::string key,std::string tmp_directory)
{
  struct stat buf;
  if (stat(filename.c_str(),&buf) != 0) {
    return false;
  }
  canonical_request.reset("POST","/"+bucket_name+"/"+key+"?uploads=",host_);
canonical_request.show();
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,s3::uhash_to_string(s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_),32));
  XMLSnippet xmls;
  parse_s3_response(xmls);
  auto b=xmls.element("InitiateMultipartUploadResult/Bucket");
  auto k=xmls.element("InitiateMultipartUploadResult/Key");
  auto id=xmls.element("InitiateMultipartUploadResult/UploadId");
  if (!b.name().empty() && b.content() == bucket_name && !k.name().empty() && k.content() == key && !id.name().empty()) {
	auto upload_id=id.content();
	const size_t BUF_LEN=500000000;
	auto num_parts=(buf.st_size/BUF_LEN)+1;
std::cerr << upload_id << " " << buf.st_size << " " << num_parts << std::endl;
	std::ifstream ifs(filename.c_str());
	if (!ifs.is_open()) {
	  throw std::runtime_error("upload_file_multi(): unable to open '"+filename+"' for upload");
	}
	std::unique_ptr<char[]> buffer(new char[BUF_LEN]);
	TempFile tfile(tmp_directory);
	auto etag_re=std::regex("^ETag:");
	std::string completion_request="<CompleteMultipartUpload>";
	for (size_t n=0; n < num_parts; ++n) {
	  std::ofstream ofs(tfile.name());
	  if (!ofs.is_open()) {
	    throw std::runtime_error("upload_file_multi(): unable to create temporary file in '"+tmp_directory+"'");
	  }
	  ifs.read(buffer.get(),BUF_LEN);
	  ofs.write(buffer.get(),ifs.gcount());
	  ofs.close();
	  auto part_number=strutils::itos(n+1);
	  canonical_request.reset("PUT","/"+bucket_name+"/"+key+"?partNumber="+part_number+"&uploadId="+upload_id,host_,tfile.name(),CanonicalRequest::PayloadType::FILE_);
canonical_request.show();
	  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,s3::uhash_to_string(s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_),32));
	    auto lines=strutils::split(curl_response_,"\n");
	    for (const auto& line : lines) {
		if (std::regex_search(line,etag_re)) {
		  auto etag=strutils::split(line,":");
		  strutils::trim(etag.back());
		  strutils::replace_all(etag.back(),"\"","");
std::cerr << upload_id << " " << part_number << " " << etag.back() << std::endl;
		  completion_request+="<Part><PartNumber>"+part_number+"</PartNumber><ETag>"+etag.back()+"</ETag></Part>";
		  break;
		}
	    }
	}
	completion_request+="</CompleteMultipartUpload>";
std::cerr << completion_request << std::endl;
	canonical_request.reset("POST","/"+bucket_name+"/"+key+"?uploadId="+upload_id,host_,completion_request);
canonical_request.show();
	curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,s3::uhash_to_string(s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_),32));
	XMLSnippet xmls;
	parse_s3_response(xmls);
	auto b=xmls.element("CompleteMultipartUploadResult/Bucket");
	auto k=xmls.element("CompleteMultipartUploadResult/Key");
	if (!b.name().empty() && b.content() == bucket_name && !k.name().empty() && k.content() == key) {
	  return true;
	}
	else {
	  return false;
	}
  }
  else {
    throw std::runtime_error("upload_file_multi(): unexpected response: "+curl_response_);
  }
}

} // end namespace s3
