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

size_t header_callback(char *buffer,size_t size,size_t nitems,void *userdata)
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

std::string perform_curl_request(CURL *curl_handle,const CanonicalRequest& canonical_request,std::string access_key,std::string region,std::string terminal,std::string signature,FILE *output)
{
  static char error_buffer[CURL_ERROR_SIZE];
  curl_easy_reset(curl_handle);
  curl_easy_setopt(curl_handle,CURLOPT_ERRORBUFFER,error_buffer);
  curl_easy_setopt(curl_handle,CURLOPT_WRITEFUNCTION,handle_s3_response);
  std::string server_response;
  curl_easy_setopt(curl_handle,CURLOPT_WRITEDATA,&server_response);
  curl_easy_setopt(curl_handle,CURLOPT_HEADERFUNCTION,header_callback);
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
	struct stat buf;
	stat(canonical_request.payload().c_str(),&buf);
	curl_easy_setopt(curl_handle,CURLOPT_INFILESIZE_LARGE,static_cast<curl_off_t>(buf.st_size));
	fp=fopen(canonical_request.payload().c_str(),"r");
	curl_easy_setopt(curl_handle,CURLOPT_READDATA,fp);
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

} // end namespace s3
