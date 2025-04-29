#ifndef BITS_HPP
#define   BITS_HPP

#include <iostream>

namespace bits {

template <class MaskType>
inline void create_mask(MaskType& mask,size_t size)
{
  mask=1;
  for (size_t n=1; n < size; n++)
    (mask<<=1)++;
}

template <class BufType,class LocType>
inline void extract(const BufType *buf,LocType *loc,size_t off,const size_t bits,size_t skip = 0,const size_t num = 1)
{
// create a mask to use when right-shifting (necessary because different
// compilers do different things when right-shifting a signed bit-field)
  auto loc_size=sizeof(LocType)*8;
  auto buf_size=sizeof(BufType)*8;
  if (bits > loc_size) {
    std::cerr << "Error: trying to unpack " << bits << " bits into a " << loc_size << "-bit location" << std::endl;
    exit(1);
  }
  else {
    BufType bmask;
    create_mask(bmask,buf_size);
    skip+=bits;
    if (loc_size <= buf_size) {
	for (size_t n=0; n < num; ++n) {
// skip to the word containing the packed field
	  auto wskip=off/buf_size;
// right shift the bits in the packed buffer word to eliminate unneeded bits
	  int rshift=buf_size-(off % buf_size)-bits;
// check for a packed field spanning two words
	  if (rshift < 0) {
	    loc[n]=(buf[wskip]<<-rshift);
	    loc[n]+=(buf[++wskip]>>(buf_size+rshift))&~(bmask<<-rshift);
	  }
	  else {
	    loc[n]=(buf[wskip]>>rshift);
	  }
// remove any unneeded leading bits
	  if (bits != buf_size) loc[n]&=~(bmask<<bits);
	  off+=skip;
	}
    }
    else {
	LocType lmask;
	create_mask(lmask,loc_size);
// get difference in bits between word size of packed buffer and word size of
// unpacked location
	for (size_t n=0; n < num; ++n) {
// skip to the word containing the packed field
	  auto wskip=off/buf_size;
// right shift the bits in the packed buffer word to eliminate unneeded bits
	  int rshift=buf_size-(off % buf_size)-bits;
// check for a packed field spanning multiple words
	  if (rshift < 0) {
	    LocType not_bmask;
	    create_mask(not_bmask,loc_size-buf_size);
	    not_bmask=~(not_bmask<<buf_size);
	    loc[n]=0;
	    while (rshift < 0) {
		auto temp=buf[wskip++]&not_bmask;
		loc[n]+=(temp<<-rshift);
		rshift+=buf_size;
	    }
	    if (rshift != 0) {
		loc[n]+=(buf[wskip]>>rshift)&~(bmask<<(buf_size-rshift));
	    }
	    else {
		loc[n]+=buf[wskip]&not_bmask;
	    }
	  }
	  else {
	    loc[n]=(buf[wskip]>>rshift);
	  }
// remove any unneeded leading bits
	  if (bits != loc_size) loc[n]&=~(lmask<<bits);
	  off+=skip;
	}
    }
  }
}

template <class BufType,class LocType>
void get(const BufType *buf,LocType *loc,const size_t off,const size_t bits,const size_t skip,const size_t num)
{
  if (bits == 0) {
// no work to do
    return;
  }
  extract(buf,loc,off,bits,skip,num);
}

template <class BufType,class LocType>
void get(const BufType *buf,LocType& loc,const size_t off,const size_t bits)
{
  if (bits == 0) {
// no work to do
    return;
  }
  extract(buf,&loc,off,bits);
}

template <class BufType,class SrcType>
inline void put(BufType *buf,const SrcType *src,size_t off,const size_t bits,size_t skip = 0,const size_t num = 1)
{
// create a mask to use when right-shifting (necessary because different
// compilers do different things when right-shifting a signed bit-field)
  auto src_size=sizeof(SrcType)*8;
  auto buf_size=sizeof(BufType)*8;
  if (bits > src_size) {
//    std::cerr << "Error: packing " << bits << " bits from a " << src_size << "-bit field" << std::endl;
    exit(1);
  }
  else {
    BufType bmask;
    create_mask(bmask,buf_size);
    SrcType smask;
    create_mask(smask,src_size);
    skip+=bits;
    for (size_t n=0; n < num; ++n) {
// get number of words and bits to skip before packing begins
	auto wskip=off/buf_size;
	auto bskip=off % buf_size;
	auto lclear=bskip+bits;
	auto rclear=buf_size-bskip;
	auto left= (rclear != buf_size) ? (buf[wskip]&(bmask<<rclear)) : 0;
	if (lclear <= buf_size) {
// all bits to be packed are in the current word
// clear the field to be packed
	  auto right= (lclear != buf_size) ? (buf[wskip]&~(bmask<<(buf_size-lclear))) : 0;
// fill the field to be packed
	  buf[wskip]= (src_size != bits) ? (src[n]&~(smask<<bits)) : src[n];
	  buf[wskip]=left|right|(buf[wskip]<<(rclear-bits));
	}
	else {
// bits to be packed cross a word boundary(ies)
// clear the bit field to be packed
	  auto more=bits-rclear;
//	  buf[wskip]= (buf_size > src_size) ? left|((src[n]>>more)&~(bmask<<(bits-more))) : left|(src[n]>>more);
	  buf[wskip]=(left|((src[n]>>more)&~(smask<<(bits-more))));
// clear the next (or part of the next) word and pack those bits
	  while (more > buf_size) {
	    more-=buf_size;
	    buf[++wskip]=((src[n]>>more)&~(smask<<(src_size-more)));
	  }
	  ++wskip;
	  more=buf_size-more;
	  auto right= (more != buf_size) ? (buf[wskip]&~(bmask<<more)) : 0;
	  buf[wskip]= (buf_size > src_size) ? (src[n]&~(bmask<<src_size)) : src[n];
	  buf[wskip]=right|(buf[wskip]<<more);
	}
	off+=skip;
    }
  }
}

template <class BufType,class SrcType>
void set(BufType *buf,const SrcType *src,const size_t off,const size_t bits,const size_t skip = 0,const size_t num = 1)
{
  if (bits == 0) {
// no work to do
    return;
  }
  put(buf,src,off,bits,skip,num);
}

template <class BufType,class SrcType>
void set(BufType *buf,const SrcType src,const size_t off,const size_t bits)
{
  if (bits == 0) {
// no work to do
    return;
  }
  put(buf,&src,off,bits);
}

} // end namespace bits

#endif
