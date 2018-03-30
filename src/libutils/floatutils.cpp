#include <complex>
#include <bits.hpp>
#include <utils.hpp>

namespace floatutils {

bool myequalf(double a,double b)
{
  if (fabs(a-b) < 0.000000001) {
    return true;
  }
  else {
    return false;
  }
}

bool myequalf(double a,double b,double tol)
{
  if (fabs(a-b) < tol) {
    return true;
  }
  else {
    return false;
  }
}

size_t precision(double value)
{
  value=fabs(value);
  value-=value;
  value*=10.;
  size_t precision=1;
  while (precision < 10 && value < 1.) {
    ++precision;
    value*=10.;
  }
  return precision;
}

double crayconv(const unsigned char *buf,size_t off)
{
  int sign;
  bits::get(buf,sign,off,1);
  int exp;
  bits::get(buf,exp,off+1,15);
  long long fr;
  bits::get(buf,fr,off+16,48);
  exp-=0x4000;
  double native_real=pow(2.,-48.)*fr*pow(2.,exp);
  if (sign == 1) {
    native_real=-native_real;
  }
  return native_real;
}

double cdcconv(const unsigned char *buf,size_t off,size_t sign_representation)
{
// sign_representation = 0 for normal ones-complement form
//                       1 for sign magnitude

  int sign,bias=1024;
  long long fr;
  int exp;
  double native_real;

  bits::get(buf,sign,off,1);
  bits::get(buf,exp,off+1,11);
  bits::get(buf,fr,off+12,48);
  if (sign_representation == 0) {
    if (sign == 1) {
	exp=0x7ff-exp;
	fr=0xffffffffffffLL-fr;
    }
  }
  if (exp < bias) bias=1023;
  exp-=bias;
  native_real=fr*pow(2.,exp);
  return (sign == 1) ? -native_real : native_real;
}

long long cdcconv(double native_real,size_t sign_representation)
{
  const double full=pow(2.,48.)-1.,p224=pow(2.,24.);
  int exp=1024;
  size_t fr[2]={0,0};
  long long cdc_real=0;

  if (!myequalf(native_real,0.)) {
    switch (sign_representation) {
	case 0:
	  break;
	case 1:
	  if (native_real < 0.) {
	    bits::set(&cdc_real,1,4,1);
	    native_real=-native_real;
	  }
	  while (exp > 0 && native_real < full) {
	    native_real*=2.;
	    exp--;
	  }
	  while (native_real > full) {
	    native_real/=2.;
	    exp++;
	  }
	  if (exp < 1024) exp--;
	  fr[0]=native_real/p224;
	  fr[1]=native_real-fr[0]*p224;

	  bits::set(&cdc_real,exp,5,11);
	  bits::set(&cdc_real,fr,16,24,0,2);
	  break;
    }
  }
  return cdc_real;
}

double cyberconv(const unsigned char *buf,size_t off)
{
  int exp;
  bits::get(buf,exp,off,8);
  if (exp >= 0x80) {
    exp-=0x100;
  }
  size_t fr;
  bits::get(buf,fr,off+8,24);
  if (fr >= 0x800000) {
    fr-=0x1000000;
  }
  return fr*pow(2.,exp);
}

double cyberconv64(const unsigned char *buf,size_t off)
{
  int exp;
  bits::get(buf,exp,off,16);
  if (exp >= 0x8000) {
    exp-=0x10000;
  }
  long long fr;
  bits::get(buf,fr,off+16,48);
  if (fr >= 0x800000000000LL) {
    fr-=0x1000000000000LL;
  }
  return fr*pow(2.,exp);
}

double ibmconv(const unsigned char *buf,size_t off)
{
  size_t sign;
  bits::get(buf,sign,off,1);
  int exp;
  bits::get(buf,exp,off+1,7);
  exp-=64;
  size_t fr;
  bits::get(buf,fr,off+8,24);
  double native_real=pow(2.,-24.)*fr*pow(16.,exp);
  return (sign == 1) ? -native_real : native_real;
}

size_t ibmconv(double native_real)
{
  size_t ibm_real=0;
  size_t sign=0,fr=0;
  int exp=64;
  const double full=0xffffff;
  size_t size=sizeof(size_t)*8,off=0;

  if (!myequalf(native_real,0.)) {
    if (native_real < 0.) {
	sign=1;
	native_real=-native_real;
    }
    native_real/=pow(2.,-24.);
    while (exp > 0 && native_real < full) {
	native_real*=16.;
	exp--;
    }
    while (native_real > full) {
	native_real/=16.;
	exp++;
    }
    fr=lround(native_real);

    if (size > 32) {
	off=size-32;
	bits::set(&ibm_real,0,0,off);
    }
    bits::set(&ibm_real,sign,off,1);
    bits::set(&ibm_real,exp,off+1,7);
    bits::set(&ibm_real,fr,off+8,24);
  }

  return ibm_real;
}

} // end namespace floatutils
