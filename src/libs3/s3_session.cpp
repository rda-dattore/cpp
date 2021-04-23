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
using std::runtime_error;
using std::string;
using strutils::trim;

namespace s3 {

Session::Session(std::string host,std::string access_key,std::string secret_key,std::string region,std::string terminal) : host_(),access_key_(),secret_key_(),region_(),terminal_(),canonical_request("","","","",CanonicalRequest::PayloadType::STRING_),signing_key_(),curl_handle_(curl_easy_init(),&curl_easy_cleanup),curl_response_(),sha_bytes_(0),upload_bytes_(0),sha_time_(0.),upload_time_(0.)
{
  host_=host;
  access_key_=access_key;
  secret_key_=secret_key;
  region_=region;
  terminal_=terminal;
  unsigned char message_digest[32];
  auto skey=s3::signing_key(secret_key_,canonical_request.date(),region_,terminal_,message_digest);
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
  unsigned char message_digest[32];
  s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_,message_digest);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,myssl::message_digest32_to_string(message_digest));
}

std::vector<Bucket> Session::buckets()
{
  canonical_request.reset("GET","/",host_);
  unsigned char message_digest[32];
  s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_,message_digest);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,myssl::message_digest32_to_string(message_digest));
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
  unsigned char message_digest[32];
  s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_,message_digest);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,myssl::message_digest32_to_string(message_digest));
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
  unsigned char message_digest[32];
  s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_,message_digest);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,myssl::message_digest32_to_string(message_digest));
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
  unsigned char message_digest[32];
  s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_,message_digest);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,myssl::message_digest32_to_string(message_digest));
  if (curl_response_.empty()) {
    return true;
  }
  else {
    return false;
  }
}

bool Session::download_file(string bucket_name, string key, string filename) {
  if (bucket_name.empty() || key.empty()) {
    return false;
  }
  canonical_request.reset("HEAD", "/" + bucket_name + "/" + key, host_);
  unsigned char message_digest[32];
  signature(const_cast<unsigned char *>(signing_key_), canonical_request,
      region_, terminal_, message_digest);
  auto head_response = perform_curl_request(curl_handle_.get(),
      canonical_request, access_key_, region_, terminal_,
      message_digest32_to_string(message_digest));
  string etag;
  auto headers = strutils::split(head_response, "\n");
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
  FILE *output = fopen(filename.c_str(), "w");
  if (output == nullptr) {
    throw runtime_error("download_file(): could not open '" + filename +
        "' for output");
  }
//  unsigned char message_digest[32];
  signature(const_cast<unsigned char *>(signing_key_), canonical_request,
      region_, terminal_, message_digest);
  curl_response_ = perform_curl_request(curl_handle_.get(), canonical_request,
      access_key_, region_, terminal_, message_digest32_to_string(
      message_digest), output);
  fclose(output);
  trim(curl_response_);
  if (curl_response_ == etag) {
    return true;
  }
  return false;
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
  unsigned char message_digest[32];
  s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_,message_digest);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,myssl::message_digest32_to_string(message_digest),output);
  fclose(output);
  strutils::trim(curl_response_);
  if (std::regex_search(curl_response_,std::regex("^ETag: \"(.){32}\"$"))) {
    return true;
  }
  else {
    if (curl_response_.empty()) {
	std::ifstream ifs(filename);
	std::stringstream ss;
	ss << ifs.rdbuf();
	curl_response_=ss.str();
    }
    return false;
  }
}

bool Session::download_range(std::string bucket_name,std::string key,std::string range_bytes,std::unique_ptr<unsigned char[]>& buffer,size_t& BUF_LEN,std::string& error)
{
  error="";
  if (bucket_name.empty()) {
    error="Bucket not specified";
    return false;
  }
  if (key.empty()) {
    error="Key not specified";
    return false;
  }
  canonical_request.reset("GET","/"+bucket_name+"/"+key,host_);
  canonical_request.set_range(range_bytes);
  unsigned char message_digest[32];
  s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_,message_digest);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,myssl::message_digest32_to_string(message_digest),buffer,BUF_LEN);
  long http_code;
  curl_easy_getinfo(curl_handle_.get(),CURLINFO_RESPONSE_CODE,&http_code);
  if (http_code == 206) {
    return true;
  }
  else {
    error=strutils::itos(http_code);
    if (!curl_response_.empty()) {
	error+" '"+curl_response_+"'";
    }
    return false;
  }
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

std::string Session::object_metadata(std::string bucket_name,std::string key)
{
  if (bucket_name.empty()) {
    return "No bucket specified";
  }
  if (key.empty()) {
    return "No key specified";
  }
  canonical_request.reset("HEAD","/"+bucket_name+"/"+key,host_);
  unsigned char message_digest[32];
  s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_,message_digest);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,myssl::message_digest32_to_string(message_digest));
  std::string json_string;
  if (!curl_response_.empty()) {
    auto lines=strutils::split(curl_response_,"\r\n");
    for (const auto& line : lines) {
	auto key_value_pair=strutils::split(line,": ");
	if (key_value_pair.size() == 2) {
	  if (!json_string.empty()) {
	    json_string+=", ";
	  }
	  json_string+="\""+key_value_pair.front()+"\": \""+strutils::substitute(key_value_pair.back(),"\"","\\\"")+"\"";
	}
    }
    json_string="{"+json_string+"}";
  }
  return json_string;
}

std::vector<Object> Session::objects(std::string bucket_name)
{
  std::vector<Object> objects;
  if (!bucket_name.empty()) {
    auto is_truncated=true;
    std::string next_marker;
    while (is_truncated) {
	canonical_request.reset("GET","/"+bucket_name+"?delimiter=!&marker="+next_marker,host_);
	unsigned char message_digest[32];
	s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_,message_digest);
	curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,myssl::message_digest32_to_string(message_digest));
	XMLSnippet xmls;
	parse_s3_response(xmls);
	auto elist=xmls.element_list("ListBucketResult/Contents");
	for (const auto& e : elist) {
	  objects.emplace_back(e.element("Key").content(),e.element("LastModified").content(),e.element("ETag").content(),std::stoll(e.element("Size").content()),e.element("Owner/DisplayName").content(),e.element("StorageClass").content());
	}
	is_truncated= (strutils::to_lower(xmls.element("ListBucketResult/IsTruncated").content()) == "true") ? true : false;
	if (is_truncated) {
	  next_marker=xmls.element("ListBucketResult/NextMarker").content();
	}
    }
  }
  return objects;
}

void Session::parse_s3_response(XMLSnippet& xmls)
{
  if (!curl_response_.empty()) {
    auto idx=curl_response_.find("?>");
    if (idx != std::string::npos) {
	xmls.fill(curl_response_.substr(idx+2));
    }
    else {
	xmls.fill(curl_response_);
    }
    if (!xmls) {
	throw std::runtime_error("Response: '"+curl_response_+"'\nParse error: '"+xmls.parse_error()+"'");
    }
  }
}

bool Session::upload_file(std::string filename,std::string bucket_name,std::string key,std::string& error)
{
  error="";
  struct stat buf;
  if (stat(filename.c_str(),&buf) != 0) {
    error="Local file does not exist";
    return false;
  }
  canonical_request.reset("PUT","/"+bucket_name+"/"+key,host_,filename,CanonicalRequest::PayloadType::FILE_);
  auto sha_benchmark=canonical_request.sha_benchmark();
  sha_bytes_+=sha_benchmark.first;
  sha_time_+=sha_benchmark.second;
  unsigned char message_digest[32];
  struct timespec time_start;
  clock_gettime(CLOCK_MONOTONIC,&time_start);
  s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_,message_digest);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,myssl::message_digest32_to_string(message_digest));
  struct timespec time_end;
  clock_gettime(CLOCK_MONOTONIC,&time_end);
  upload_bytes_+=buf.st_size;
  upload_time_+=(time_end.tv_sec-time_start.tv_sec)+(time_end.tv_nsec-time_start.tv_nsec)/1000000000.;
  long http_code;
  curl_easy_getinfo(curl_handle_.get(),CURLINFO_RESPONSE_CODE,&http_code);
  if (http_code == 200) {
    return true;
  }
  else {
    error=strutils::itos(http_code);
    if (!curl_response_.empty()) {
	error=" '"+curl_response_+"'";
    }
    return false;
  }
}

bool Session::upload_file_multi(std::string filename,std::string bucket_name,std::string key,std::string tmp_directory,std::string& error)
{
  struct stat buf;
  if (stat(filename.c_str(),&buf) != 0) {
    error="Local file does not exist";
    return false;
  }
  canonical_request.reset("POST","/"+bucket_name+"/"+key+"?uploads=",host_);
  unsigned char message_digest[32];
  s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_,message_digest);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,myssl::message_digest32_to_string(message_digest));
  XMLSnippet xmls;
  parse_s3_response(xmls);
  auto bucket_element=xmls.element("InitiateMultipartUploadResult/Bucket");
  auto key_element=xmls.element("InitiateMultipartUploadResult/Key");
  auto upload_id_element=xmls.element("InitiateMultipartUploadResult/UploadId");
  if (!bucket_element.name().empty() && bucket_element.content() == bucket_name && !key_element.name().empty() && key_element.content() == key && !upload_id_element.name().empty()) {
    auto upload_id=upload_id_element.content();
    const size_t BUF_LEN=500000000;
    auto num_parts=(buf.st_size/BUF_LEN)+1;
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
	unsigned char message_digest[32];
	s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_,message_digest);
	curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,myssl::message_digest32_to_string(message_digest));
	auto lines=strutils::split(curl_response_,"\n");
	for (const auto& line : lines) {
	  if (std::regex_search(line,etag_re)) {
	    auto etag=strutils::split(line,":");
	    strutils::trim(etag.back());
	    strutils::replace_all(etag.back(),"\"","");
	    completion_request+="<Part><PartNumber>"+part_number+"</PartNumber><ETag>"+etag.back()+"</ETag></Part>";
	    break;
	  }
	}
    }
    completion_request+="</CompleteMultipartUpload>";
    canonical_request.reset("POST","/"+bucket_name+"/"+key+"?uploadId="+upload_id,host_,completion_request);
    unsigned char message_digest[32];
    s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_,message_digest);
    curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,myssl::message_digest32_to_string(message_digest));
    XMLSnippet xmls;
    parse_s3_response(xmls);
    bucket_element=xmls.element("CompleteMultipartUploadResult/Bucket");
    key_element=xmls.element("CompleteMultipartUploadResult/Key");
    if (!bucket_element.name().empty() && bucket_element.content() == bucket_name && !key_element.name().empty() && key_element.content() == key) {
	return true;
    }
    else {
	error=curl_response_;
	return false;
    }
  }
  else {
    throw std::runtime_error("upload_file_multi(): unexpected response: "+curl_response_);
  }
}

void *thread_upload_file_multi(void *ts)
{
  const std::string THIS_FUNC=__func__;
  UploadFileThreadStruct *t=reinterpret_cast<UploadFileThreadStruct *>(ts);
  struct timespec time_start;
  clock_gettime(CLOCK_MONOTONIC,&time_start);
  t->session->canonical_request.reset("PUT"+strutils::lltos(t->file_pos)+"-"+strutils::lltos(t->file_pos+t->bytes_to_transfer-1),"/"+t->bucket_name+"/"+t->key+"?partNumber="+t->part_number+"&uploadId="+t->upload_id,t->host,t->filename,CanonicalRequest::PayloadType::FILE_);
  struct timespec time_end;
  clock_gettime(CLOCK_MONOTONIC,&time_end);
  t->sha_time=(time_end.tv_sec-time_start.tv_sec)+(time_end.tv_nsec-time_start.tv_nsec)/1000000000.;
  unsigned char message_digest[32];
  clock_gettime(CLOCK_MONOTONIC,&time_start);
  s3::signature(const_cast<unsigned char *>(t->session->signing_key_),t->session->canonical_request,t->session->region_,t->session->terminal_,message_digest);
  t->session->curl_response_=perform_curl_request(t->session->curl_handle_.get(),t->session->canonical_request,t->session->access_key_,t->session->region_,t->session->terminal_,myssl::message_digest32_to_string(message_digest));
  clock_gettime(CLOCK_MONOTONIC,&time_end);
  t->upload_time=(time_end.tv_sec-time_start.tv_sec)+(time_end.tv_nsec-time_start.tv_nsec)/1000000000.;
  auto lines=strutils::split(t->session->curl_response_,"\n");
  for (const auto& line : lines) {
    if (std::regex_search(line,std::regex("^ETag:"))) {
	auto etag_parts=strutils::split(line,":");
	t->upload_etag=etag_parts.back();
	strutils::trim(t->upload_etag);
	strutils::replace_all(t->upload_etag,"\"","");
	break;
    }
  }
  return nullptr;
}

bool Session::upload_file_multi_threaded(std::string filename,std::string bucket_name,std::string key,size_t max_num_threads,std::string& error)
{
  struct stat buf;
  if (stat(filename.c_str(),&buf) != 0) {
    error="Local file does not exist";
    return false;
  }
  canonical_request.reset("POST","/"+bucket_name+"/"+key+"?uploads=",host_);
  unsigned char message_digest[32];
  s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_,message_digest);
  curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,myssl::message_digest32_to_string(message_digest));
  XMLSnippet xmls;
  parse_s3_response(xmls);
  auto bucket_element=xmls.element("InitiateMultipartUploadResult/Bucket");
  auto key_element=xmls.element("InitiateMultipartUploadResult/Key");
  auto upload_id_element=xmls.element("InitiateMultipartUploadResult/UploadId");
  if (!bucket_element.name().empty() && bucket_element.content() == bucket_name && !key_element.name().empty() && key_element.content() == key && !upload_id_element.name().empty()) {
    auto upload_id=upload_id_element.content();
    size_t PART_SIZE=buf.st_size/max_num_threads;
    if (PART_SIZE < MINIMUM_PART_SIZE) {
	max_num_threads=lround(1.+static_cast<double>(buf.st_size)/MINIMUM_PART_SIZE);
	PART_SIZE=buf.st_size/max_num_threads;
    }
    if (max_num_threads == 1) {
// if the file only requires one thread, use the unthreaded upload
	return upload_file(filename,bucket_name,key,error);
    }
    else {
	const size_t NUM_THREADS_1=max_num_threads-1;
	auto last_part_size=buf.st_size-PART_SIZE*NUM_THREADS_1;
	std::vector<UploadFileThreadStruct> threads(max_num_threads);
	for (size_t n=0; n < NUM_THREADS_1; ++n) {
	  threads[n].session.reset(new Session(host_,access_key_,secret_key_,region_,terminal_));
	  threads[n].filename=filename;
	  threads[n].file_pos=n*PART_SIZE;
	  threads[n].bytes_to_transfer=PART_SIZE;
	  threads[n].host=host_;
	  threads[n].bucket_name=bucket_name;
	  threads[n].key=key;
	  threads[n].upload_id=upload_id;
	  threads[n].part_number=strutils::itos(n+1);
	  pthread_create(&threads[n].tid,nullptr,thread_upload_file_multi,&threads[n]);
	}
	threads[NUM_THREADS_1].session.reset(new Session(host_,access_key_,secret_key_,region_,terminal_));
	threads[NUM_THREADS_1].filename=filename;
	threads[NUM_THREADS_1].file_pos=NUM_THREADS_1*PART_SIZE;
	threads[NUM_THREADS_1].bytes_to_transfer=last_part_size;
	threads[NUM_THREADS_1].host=host_;
	threads[NUM_THREADS_1].bucket_name=bucket_name;
	threads[NUM_THREADS_1].key=key;
	threads[NUM_THREADS_1].upload_id=upload_id;
	threads[NUM_THREADS_1].part_number=strutils::itos(max_num_threads);
	pthread_create(&threads[NUM_THREADS_1].tid,nullptr,thread_upload_file_multi,&threads[NUM_THREADS_1]);
	std::string completion_request="<CompleteMultipartUpload>";
	for (size_t n=0; n < max_num_threads; ++n) {
	  pthread_join(threads[n].tid,nullptr);
	  completion_request+="<Part><PartNumber>"+threads[n].part_number+"</PartNumber><ETag>"+threads[n].upload_etag+"</ETag></Part>";
	  sha_bytes_+=threads[n].bytes_to_transfer;
	  sha_time_+=threads[n].sha_time;
	  upload_bytes_+=threads[n].bytes_to_transfer;
	  upload_time_+=threads[n].upload_time;
	}
	completion_request+="</CompleteMultipartUpload>";
	canonical_request.reset("POST","/"+bucket_name+"/"+key+"?uploadId="+upload_id,host_,completion_request);
	unsigned char message_digest[32];
	s3::signature(const_cast<unsigned char *>(signing_key_),canonical_request,region_,terminal_,message_digest);
	curl_response_=perform_curl_request(curl_handle_.get(),canonical_request,access_key_,region_,terminal_,myssl::message_digest32_to_string(message_digest));
	XMLSnippet xmls;
	parse_s3_response(xmls);
	bucket_element=xmls.element("CompleteMultipartUploadResult/Bucket");
	key_element=xmls.element("CompleteMultipartUploadResult/Key");
	if (!bucket_element.name().empty() && bucket_element.content() == bucket_name && !key_element.name().empty() && key_element.content() == key) {
	  return true;
	}
	else {
	  error=curl_response_;
	  return false;
	}
    }
  }
  else {
    throw std::runtime_error("upload_file_multi_threaded(): unexpected response: "+curl_response_);
  }
}

} // end namespace s3
