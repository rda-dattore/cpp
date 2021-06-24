#include <iostream>
#include <iomanip>
#include <s3.hpp>
#include <myssl.hpp>

namespace s3 {

unsigned char *signature(unsigned char *signing_key,const CanonicalRequest& canonical_request,std::string region,std::string terminal,unsigned char (&message_digest)[32])
{
  std::string string_to_sign="AWS4-HMAC-SHA256\n"+canonical_request.timestamp()+"\n"+canonical_request.date()+"/"+region+"/s3/"+terminal+"\n"+canonical_request.hashed();
  return myssl::hmac_sha256(signing_key,32,string_to_sign,message_digest);
}

unsigned char *signing_key(std::string secret_key,std::string date,std::string region,std::string terminal,unsigned char (&message_digest)[32])
{
  unsigned char md[3][32];
  myssl::hmac_sha256("AWS4"+secret_key,date,md[0]);
  myssl::hmac_sha256(md[0],32,region,md[1]);
  myssl::hmac_sha256(md[1],32,"s3",md[2]);
  return myssl::hmac_sha256(md[2],32,terminal,message_digest);
}

} // end namespace s3
