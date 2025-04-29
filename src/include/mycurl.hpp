#include <cstdio>
#include <string>
#include <memory>
#include <curl/curl.h>

namespace mycurl {

struct Response {
  Response() : http_status(0), body() {}
  Response(const Response&& source)
      : http_status(source.http_status), body(std::move(source.body)) {}
  Response& operator=(const Response&& source);

  long http_status;
  std::string body;

};

class Request {
public:
  enum class Method {
    kGet,
    kPost,
    kPut,
    kDelete,
  };

  Request();
  Request(const Request& source) = delete;
  ~Request();
  Request& operator=(const Request& source) = delete;
  void create(Method method, std::string url);
  Response submit();

private:
  CURL *curl_handle;
  Method method_;
  std::string url_;

};

extern size_t capture_response(char *ptr, size_t size, size_t nmemb, void *userdata);

} // end namespace mycurl
