#include <string>
#include <regex>
#include <curl/curl.h>
#include <sys/stat.h>
#include <s3.hpp>
#include <strutils.hpp>

namespace s3 {

std::string curl_request(const CanonicalRequest& canonical_request,std::string access_key,std::string region,std::string terminal,std::string signature)
{
  std::string curl_request_="-X "+canonical_request.method()+" -H 'Authorization: AWS4-HMAC-SHA256 Credential="+access_key+"/"+canonical_request.date()+"/"+region+"/s3/"+terminal+", SignedHeaders="+canonical_request.signed_headers()+", Signature="+signature+"'";
  auto headers=strutils::split(canonical_request.headers(),"\n");
  for (const auto& header : headers) {
    curl_request_+=" -H '"+header+"'";
  }
  curl_request_+=" 'http://"+canonical_request.host()+canonical_request.uri();
  if (!canonical_request.query_string().empty()) {
    curl_request_+="?"+canonical_request.query_string();
  }
  curl_request_+="'";
  return curl_request_;
}

int debug_callback(CURL *handle,curl_infotype type,char *data,size_t size,void *userptr)
{
  switch (type) {
    case CURLINFO_HEADER_IN:
    {
	std::cerr << "< ";
	std::cerr.write(data,size);
	break;
    }
    case CURLINFO_HEADER_OUT:
    {
	std::string header_out(data,size);
	auto headers=strutils::split(header_out,"\n");
	for (const auto& header : headers) {
	  std::cerr << "> " << header << std::endl;
	}
	break;
    }
    case CURLINFO_DATA_OUT:
    {
/*
	std::cerr << "D> ";
	std::cerr.write(data,size);
	std::cerr << std::endl;
*/
	break;
    }
    default:
    {}
  }
  return 0;
}

size_t full_header_callback(char *buffer,size_t size,size_t nitems,void *userdata)
{
  std::string header(buffer,size*nitems);
  if (header.find(":") != std::string::npos) {
    std::string *s=reinterpret_cast<std::string *>(userdata);
    s->append(header);
  }
  return size*nitems;
}

size_t etag_header_callback(char *buffer,size_t size,size_t nitems,void *userdata)
{
  static std::regex etag_re("^ETag:");
  std::string header(buffer,size*nitems);
  if (std::regex_search(header,etag_re)) {
    std::string *s=reinterpret_cast<std::string *>(userdata);
    s->append(header+"\n");
  }
  return size*nitems;
}

size_t handle_s3_response(char *ptr,size_t size,size_t nmemb,void *userdata)
{
  std::string *s=reinterpret_cast<std::string *>(userdata);
  *s+=std::string(ptr,size*nmemb);
  return size*nmemb;
}

struct UserData {
  UserData() : ifs(),file_offset(-1),upload_size(0),bytes_buffered(0) {}

  std::ifstream ifs;
  long long file_offset,upload_size,bytes_buffered;
};

size_t range_read(void *ptr,size_t size,size_t nmemb,void *userdata)
{
  auto user_data=reinterpret_cast<UserData *>(userdata);
  if (user_data->bytes_buffered >= user_data->upload_size) {
    return 0;
  }
  user_data->ifs.read(reinterpret_cast<char *>(ptr),size*nmemb);
  user_data->bytes_buffered+=user_data->ifs.gcount();
  if (user_data->bytes_buffered > user_data->upload_size) {
    return user_data->ifs.gcount()-(user_data->bytes_buffered-user_data->upload_size);
  }
  else {
    return user_data->ifs.gcount();
  }
}

std::string perform_curl_request(CURL *curl_handle,const CanonicalRequest& canonical_request,std::string access_key,std::string region,std::string terminal,std::string signature,FILE *output)
{
  static char error_buffer[CURL_ERROR_SIZE];
  curl_easy_reset(curl_handle);
  curl_easy_setopt(curl_handle,CURLOPT_ERRORBUFFER,error_buffer);
  curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION,handle_s3_response);
  std::string server_response;
  curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,&server_response);
  curl_easy_setopt(curl_handle,CURLOPT_HEADERFUNCTION,etag_header_callback);
  curl_easy_setopt(curl_handle,CURLOPT_HEADERDATA,&server_response);
  struct curl_slist *header_list=nullptr;
  header_list=curl_slist_append(header_list,("Authorization: AWS4-HMAC-SHA256 Credential="+access_key+"/"+canonical_request.date()+"/"+region+"/s3/"+terminal+", SignedHeaders="+canonical_request.signed_headers()+", Signature="+signature).c_str());
  auto headers=strutils::split(canonical_request.headers(),"\n");
  for (const auto& header : headers) {
    header_list=curl_slist_append(header_list,header.c_str());
  }
  curl_easy_setopt(curl_handle,CURLOPT_HTTPHEADER,header_list);
  std::string url="http://"+canonical_request.host()+canonical_request.uri();
  if (!canonical_request.query_string().empty()) {
    url+="?"+canonical_request.query_string();
  }
  curl_easy_setopt(curl_handle,CURLOPT_URL,url.c_str());
  FILE *fp=nullptr;
  if (canonical_request.method() == "GET") {
    if (output != nullptr) {
	curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION,nullptr);
	curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,output);
	if (!canonical_request.range_bytes().empty()) {
	  curl_easy_setopt(curl_handle,CURLOPT_RANGE,canonical_request.range_bytes().c_str());
	}
    }
  }
  else if (canonical_request.method() == "DELETE") {
    curl_easy_setopt(curl_handle,CURLOPT_CUSTOMREQUEST,"DELETE");
  }
  else if (canonical_request.method() == "PUT") {
    if (canonical_request.payload_type() == CanonicalRequest::PayloadType::STRING_) {
	curl_easy_setopt(curl_handle,CURLOPT_CUSTOMREQUEST,"PUT");
    }
    else if (canonical_request.payload_type() == CanonicalRequest::PayloadType::FILE_) {
	curl_easy_setopt(curl_handle,CURLOPT_UPLOAD,1);
	if (canonical_request.range_bytes().empty()) {
	  struct stat buf;
	  stat(canonical_request.payload().c_str(),&buf);
	  curl_easy_setopt(curl_handle,CURLOPT_INFILESIZE_LARGE,static_cast<curl_off_t>(buf.st_size));
	  fp=fopen(canonical_request.payload().c_str(),"r");
	  curl_easy_setopt(curl_handle,CURLOPT_READDATA,fp);
	}
	else {
	  auto user_data=new UserData;
	  user_data->ifs.open(canonical_request.payload().c_str());
	  if (!user_data->ifs.is_open()) {
	    throw std::runtime_error("perform_curl_request(): unable to open '"+canonical_request.payload()+"' for upload");
	  }
	  auto range_bytes=strutils::split(canonical_request.range_bytes(),"-");
	  auto file_offset=std::stoll(range_bytes.front());
	  user_data->file_offset=file_offset;
	  user_data->ifs.seekg(file_offset,std::ios::beg);
	  auto bytes_to_read=std::stoll(range_bytes.back())-file_offset+1;
	  user_data->upload_size=bytes_to_read;
	  curl_easy_setopt(curl_handle,CURLOPT_INFILESIZE_LARGE,static_cast<curl_off_t>(bytes_to_read));
	  curl_easy_setopt(curl_handle,CURLOPT_READFUNCTION,range_read);
	  curl_easy_setopt(curl_handle,CURLOPT_READDATA,user_data);
	}
/*
curl_easy_setopt(curl_handle,CURLOPT_VERBOSE,1);
curl_easy_setopt(curl_handle,CURLOPT_DEBUGFUNCTION,debug_callback);
*/
    }
  }
  else if (canonical_request.method() == "POST") {
    curl_easy_setopt(curl_handle,CURLOPT_POST,1);
    curl_easy_setopt(curl_handle,CURLOPT_POSTFIELDSIZE,canonical_request.payload().length());
    curl_easy_setopt(curl_handle,CURLOPT_COPYPOSTFIELDS,canonical_request.payload().c_str());
/*
curl_easy_setopt(curl_handle,CURLOPT_VERBOSE,1);
curl_easy_setopt(curl_handle,CURLOPT_DEBUGFUNCTION,debug_callback);
*/
  }
  else if (canonical_request.method() == "HEAD") {
    curl_easy_setopt(curl_handle,CURLOPT_CUSTOMREQUEST,"HEAD");
    curl_easy_setopt(curl_handle,CURLOPT_NOBODY,1);
    curl_easy_setopt(curl_handle,CURLOPT_HEADERFUNCTION,full_header_callback);
  }
  else {
    throw std::runtime_error("invalid request method '"+canonical_request.method()+"'");
  }
  auto status=curl_easy_perform(curl_handle);
  if (status != CURLE_OK) {
    throw std::runtime_error("curl error code: "+strutils::itos(status)+", error message: "+error_buffer);
  }
  if (fp != nullptr) {
    fclose(fp);
  }
  return server_response;
}

size_t write_to_buffer(char *ptr,size_t size,size_t nmemb,void *vector_address)
{
  size_t num_bytes=nmemb*size;
  std::vector<unsigned char> *v=reinterpret_cast<std::vector<unsigned char> *>(vector_address);
  auto curr_size=v->size();
  v->resize(curr_size+num_bytes);
  std::copy(ptr,ptr+num_bytes,&(*v)[curr_size]);
  return num_bytes;
}

std::string perform_curl_request(CURL *curl_handle,const CanonicalRequest& canonical_request,std::string access_key,std::string region,std::string terminal,std::string signature,std::unique_ptr<unsigned char[]>& buffer,size_t& buffer_capacity)
{
  static char error_buffer[CURL_ERROR_SIZE];
  curl_easy_reset(curl_handle);
  curl_easy_setopt(curl_handle,CURLOPT_ERRORBUFFER,error_buffer);
  curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION,handle_s3_response);
  std::string server_response;
  curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,&server_response);
  curl_easy_setopt(curl_handle,CURLOPT_HEADERFUNCTION,etag_header_callback);
  curl_easy_setopt(curl_handle,CURLOPT_HEADERDATA,&server_response);
  struct curl_slist *header_list=nullptr;
  header_list=curl_slist_append(header_list,("Authorization: AWS4-HMAC-SHA256 Credential="+access_key+"/"+canonical_request.date()+"/"+region+"/s3/"+terminal+", SignedHeaders="+canonical_request.signed_headers()+", Signature="+signature).c_str());
  auto headers=strutils::split(canonical_request.headers(),"\n");
  for (const auto& header : headers) {
    header_list=curl_slist_append(header_list,header.c_str());
  }
  curl_easy_setopt(curl_handle,CURLOPT_HTTPHEADER,header_list);
  std::string url="http://"+canonical_request.host()+canonical_request.uri();
  if (!canonical_request.query_string().empty()) {
    url+="?"+canonical_request.query_string();
  }
  curl_easy_setopt(curl_handle,CURLOPT_URL,url.c_str());
  std::vector<unsigned char> v;
  if (canonical_request.method() == "GET") {
    curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION,write_to_buffer);
    curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,&v);
    if (!canonical_request.range_bytes().empty()) {
	curl_easy_setopt(curl_handle,CURLOPT_RANGE,canonical_request.range_bytes().c_str());
    }
  }
  else {
    throw std::runtime_error("invalid request method '"+canonical_request.method()+"'");
  }
  auto status=curl_easy_perform(curl_handle);
  if (status != CURLE_OK) {
    throw std::runtime_error("curl error code: "+strutils::itos(status)+", error message: "+error_buffer);
  }
  if (v.size() > buffer_capacity) {
    buffer_capacity=v.size();
    buffer.reset(new unsigned char[buffer_capacity]);
  }
  std::copy(v.begin(),v.end(),buffer.get());
  return server_response;
}

} // end namespace s3
