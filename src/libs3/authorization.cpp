#include <iostream>
#include <iomanip>
#include <s3.hpp>
#include <myssl.hpp>

namespace s3 {

unsigned char *signature(unsigned char *signing_key,const CanonicalRequest& canonical_request,std::string region,std::string terminal)
{
  std::string string_to_sign="AWS4-HMAC-SHA256\n"+canonical_request.timestamp()+"\n"+canonical_request.date()+"/"+region+"/s3/"+terminal+"\n"+canonical_request.hashed();
  return hmac_sha256(signing_key,32,string_to_sign);
}

unsigned char *signing_key(std::string secret_key,std::string date,std::string region,std::string terminal)
{
  return hmac_sha256(hmac_sha256(hmac_sha256(hmac_sha256("AWS4"+secret_key,date),32,region),32,"s3"),32,terminal);
}

} // end namespace s3
