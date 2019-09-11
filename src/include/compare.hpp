// FILE: compare.h

#ifndef COMPARE_H
#define   COMPARE_H

#include <iostream>
#include <mymap.hpp>

#define COMPARE_NUM_LEVELS  3

struct CompareTolerances {
  short elev,hgt,dd;
  float lat,lon,temp,ff;
};

class Compare {
public:
  Compare() : stn(0),nlvls(0),elev(0),pres(),dd(),hgt(),lat(0.),lon(0.),temp(),ff() {}
  virtual ~Compare() {}

protected:
  size_t stn;
  short nlvls,elev,pres[COMPARE_NUM_LEVELS],dd[COMPARE_NUM_LEVELS];
  int hgt[COMPARE_NUM_LEVELS];
  float lat,lon,temp[COMPARE_NUM_LEVELS],ff[COMPARE_NUM_LEVELS];
};

class CompareRecord;
class CompareDifferences : public Compare {
public:
  CompareDifferences() : min_stn(0xffffffff),max_stn(0),date(0),min_date(0xffffffff),max_date(0),stn_table(),date_table() {}
  void countStationNonCompare(size_t key);
  void countDateNonCompare(size_t key);
  void printSummary();
  friend bool isGoodCompare(const CompareRecord& a,const CompareRecord& b,const CompareTolerances& tol,CompareDifferences& comp_diff);
  friend std::ostream& operator<<(std::ostream& out_stream,const CompareDifferences& source);

private:
  size_t min_stn,max_stn,date,min_date,max_date;
  struct Count {
    Count() : key(),nncomp(0),ncomp(0),ngood(0),nbad(0),nblat(0),nblon(0),nbelev(0),nbpres(),nbhgt(),nbtemp(),nbdd(),nbff(),elev_diff(0.),pres_diff(),hgt_diff(),temp_diff(),dd_diff(),ff_diff(),nelev_diff(0.),npres_diff(),nhgt_diff(),ntemp_diff(),ndd_diff(),nff_diff() {}

    size_t key;
    size_t nncomp,ncomp,ngood,nbad;
    size_t nblat,nblon,nbelev;
    size_t nbpres[COMPARE_NUM_LEVELS],nbhgt[COMPARE_NUM_LEVELS],nbtemp[COMPARE_NUM_LEVELS],nbdd[COMPARE_NUM_LEVELS],nbff[COMPARE_NUM_LEVELS];
    float elev_diff,pres_diff[COMPARE_NUM_LEVELS],hgt_diff[COMPARE_NUM_LEVELS],temp_diff[COMPARE_NUM_LEVELS],dd_diff[COMPARE_NUM_LEVELS],ff_diff[COMPARE_NUM_LEVELS];
    float nelev_diff,npres_diff[COMPARE_NUM_LEVELS],nhgt_diff[COMPARE_NUM_LEVELS],ntemp_diff[COMPARE_NUM_LEVELS],ndd_diff[COMPARE_NUM_LEVELS],nff_diff[COMPARE_NUM_LEVELS];
  };
  my::map<Count> stn_table,date_table;
};

class CompareRecord : public Compare {
public:
// MODIFICATION MEMBER FUNCTIONS:
  void fill(const char *input_buffer);

// CONSTANT MEMBER FUNCTIONS:
  size_t getStation() const { return stn; }
  size_t getDate() const;

// FRIEND FUNCTIONS:
  friend bool operator==(const CompareRecord& a,const CompareRecord& b);
  friend bool operator!=(const CompareRecord& a,const CompareRecord& b)
  { return !(a == b); }
  friend bool operator<(const CompareRecord& a,const CompareRecord& b);
  friend bool operator>(const CompareRecord& a,const CompareRecord& b);
  friend bool isGoodCompare(const CompareRecord& a,const CompareRecord& b,
    const CompareTolerances& tol,CompareDifferences& comp_diff);
  friend std::ostream& operator<<(std::ostream& out_stream,const CompareRecord& source);

private:
  short src,yr,mo,dy,hr;
};

#endif
