#include <bits.hpp>

long long checksum(const unsigned char *buf,size_t num_words,size_t word_size,long long& sum)
{
// on return from checksum, the difference between the computed add and carry
//  logical checksum and the one packed into a record is returned, sum points
//  to the location containing the computed checksum(s), and num_sums gives the
//  number of locations containing the checksum(s)
  size_t n;
  long long err;
  long long *cp,over;
  long long OVER=0x10000000;

  OVER=(OVER<<32);
  sum=0;
  cp=new long long[num_words];

// unpack the words in the record
  bits::get(buf,cp,0,word_size,0,num_words);

// loop through and sum the words
  for (n=0; n < num_words-1; n++) {
    switch (word_size) {
	case 60:
	{
	  sum+=cp[n];
	  while (sum >= OVER) {
	    over=sum/OVER;
	    sum-=OVER;
	    sum+=over;
	  }
	  break;
	}
	default:
	{
	  sum+=cp[n];
	}
    }
  }
// compute the error (difference between sums and checksum)
  err=cp[num_words-1]-sum;
  delete[] cp;
  return err;
}
