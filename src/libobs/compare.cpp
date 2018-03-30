#include <iomanip>
#include <compare.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <datetime.hpp>

void CompareDifferences::countStationNonCompare(size_t key)
{
  size_t n;
  static Count entry;

  if (!stn_table.found(key,entry)) {
    entry.key=key;
    entry.nncomp=entry.ncomp=entry.ngood=entry.nbad=0;
    entry.nblat=entry.nblon=entry.nbelev=0;
    entry.elev_diff=entry.nelev_diff=0;
    for (n=0; n < COMPARE_NUM_LEVELS; n++) {
	entry.nbpres[n]=entry.nbhgt[n]=entry.nbtemp[n]=entry.nbdd[n]=
        entry.nbff[n]=0;
	entry.pres_diff[n]=entry.hgt_diff[n]=entry.temp_diff[n]=entry.dd_diff[n]=
        entry.ff_diff[n]=0.;
	entry.npres_diff[n]=entry.nhgt_diff[n]=entry.ntemp_diff[n]=
        entry.ndd_diff[n]=entry.nff_diff[n]=0.;
    }
  }
  entry.nncomp++;
  stn_table.replace(entry);
  if (key < min_stn) min_stn=key;
  if (key > max_stn) max_stn=key;
}

void CompareDifferences::countDateNonCompare(size_t key)
{
  size_t n;
  static Count entry;

  if (!date_table.found(key,entry)) {
    entry.key=key;
    entry.nncomp=entry.ncomp=entry.ngood=entry.nbad=0;
    entry.nblat=entry.nblon=entry.nbelev=0;
    entry.elev_diff=entry.nelev_diff=0;
    for (n=0; n < COMPARE_NUM_LEVELS; n++) {
	entry.nbpres[n]=entry.nbhgt[n]=entry.nbtemp[n]=entry.nbdd[n]=
        entry.nbff[n]=0;
	entry.pres_diff[n]=entry.hgt_diff[n]=entry.temp_diff[n]=entry.dd_diff[n]=
        entry.ff_diff[n]=0.;
	entry.npres_diff[n]=entry.nhgt_diff[n]=entry.ntemp_diff[n]=
        entry.ndd_diff[n]=entry.nff_diff[n]=0.;
    }
  }
  entry.nncomp++;
  date_table.replace(entry);
  if (key < min_date) min_date=key;
  if (key > max_date) max_date=key;
}

void CompareDifferences::printSummary()
{
  size_t n,m,num_lines;
  size_t nncomp,ncomp,ngood,nbad,nblat,nblon,nbelev;
  float pres_diff[COMPARE_NUM_LEVELS],hgt_diff[COMPARE_NUM_LEVELS],
    temp_diff[COMPARE_NUM_LEVELS],dd_diff[COMPARE_NUM_LEVELS],
    ff_diff[COMPARE_NUM_LEVELS];
  float npres_diff[COMPARE_NUM_LEVELS],nhgt_diff[COMPARE_NUM_LEVELS],
    ntemp_diff[COMPARE_NUM_LEVELS],ndd_diff[COMPARE_NUM_LEVELS],
    nff_diff[COMPARE_NUM_LEVELS];
  Count table_entry;
  size_t lcnt;

  std::cout.setf(std::ios::fixed);

  std::cout << "SUMMARY OF COMPARES BY STATION" << std::endl;
  std::cout << "  * indicates more than 50% of at least one parameter on more than "
    << "one level failed the comparison" << std::endl;
  std::cout << "  x indicates that no comparisons could be made for that station\n"
    << std::endl;
  std::cout << "             NON-                                       ";
  for (n=0; n < COMPARE_NUM_LEVELS; n++) {
    if (pres[n] != 0)
	std::cout << "  |-------- " << std::setw(4) << pres[n] << " MB ---------|";
    else
	std::cout << "  |----------- SFC ----------|";
  }
  std::cout << std::endl;
  std::cout << " STATION COMPARES COMPARES  GOOD   BAD   LAT   LON  ELEV";
  for (n=0; n < COMPARE_NUM_LEVELS; n++)
    std::cout << "  PRES   HGT  TEMP    DD    FF";
  std::cout << std::endl;
  num_lines=4;
  nncomp=ncomp=ngood=nbad=nblat=nblon=nbelev=0;
  for (n=0; n < COMPARE_NUM_LEVELS; n++) {
    pres_diff[n]=hgt_diff[n]=temp_diff[n]=dd_diff[n]=ff_diff[n]=0.;
    npres_diff[n]=nhgt_diff[n]=ntemp_diff[n]=ndd_diff[n]=nff_diff[n]=0.;
  }
  for (n=min_stn; n <= max_stn; n++) {
    if (stn_table.found(n,table_entry)) {
	nncomp+=table_entry.nncomp;
	ncomp+=table_entry.ncomp;
	ngood+=table_entry.ngood;
	nbad+=table_entry.nbad;
	nblat+=table_entry.nblat;
	nblon+=table_entry.nblon;
	nbelev+=table_entry.nbelev;
	if (table_entry.ncomp == 0)
	  std::cout << " x";
	else
	  std::cout << "  ";
	std::cout << std::setw(6) << std::setfill('0') << n << std::setfill(' ') << std::setw(9) << table_entry.nncomp << std::setw(9) << table_entry.ncomp;
	if (table_entry.ncomp == 0) {
	  std::cout << std::endl;
	  num_lines++;
	}
	else {
	  std::cout << std::setw(6) << table_entry.ngood << std::setw(6) << table_entry.nbad << std::endl;
	  std::cout << "  " << std::setw(6) << std::setfill('0') << n << std::setfill(' ') << ":TOTAL COMPARES                           " << std::setw(6) <<
          table_entry.nelev_diff;
	  if (floatutils::myequalf(table_entry.nelev_diff,0.)) table_entry.nelev_diff=1.;
	  lcnt=0;
	  for (m=0; m < COMPARE_NUM_LEVELS; m++) {
	    if ((table_entry.nbhgt[m]/static_cast<float>(table_entry.nhgt_diff[m])) > 0.49999 || (table_entry.nbtemp[m]/static_cast<float>(table_entry.ntemp_diff[m])) > 0.49999 || (table_entry.nbdd[m]/static_cast<float>(table_entry.ndd_diff[m])) > 0.49999 || (table_entry.nbff[m]/static_cast<float>(table_entry.nff_diff[m])) > 0.49999)
		lcnt++;
	    std::cout << std::setw(6) << table_entry.npres_diff[m] << std::setw(6) <<
            table_entry.nhgt_diff[m] << std::setw(6) << table_entry.ntemp_diff[m] <<
            std::setw(6) << table_entry.ndd_diff[m] << std::setw(6) <<
            table_entry.nff_diff[m];
	    npres_diff[m]+=table_entry.npres_diff[m];
	    if (floatutils::myequalf(table_entry.npres_diff[m],0.)) table_entry.npres_diff[m]=1.;
	    nhgt_diff[m]+=table_entry.nhgt_diff[m];
	    if (floatutils::myequalf(table_entry.nhgt_diff[m],0.)) table_entry.nhgt_diff[m]=1.;
	    ntemp_diff[m]+=table_entry.ntemp_diff[m];
	    if (floatutils::myequalf(table_entry.ntemp_diff[m],0.)) table_entry.ntemp_diff[m]=1.;
	    ndd_diff[m]+=table_entry.ndd_diff[m];
	    if (floatutils::myequalf(table_entry.ndd_diff[m],0.)) table_entry.ndd_diff[m]=1.;
	    nff_diff[m]+=table_entry.nff_diff[m];
	    if (floatutils::myequalf(table_entry.nff_diff[m],0.)) table_entry.nff_diff[m]=1.;
	  }
	  std::cout << std::endl;
	  std::cout.precision(2);
	  std::cout << "  " << std::setw(6) << std::setfill('0') << n << std::setfill(' ') << ":AVERAGE DIFFERENCES                      " << std::setw(6) <<
          table_entry.elev_diff/table_entry.nelev_diff;
	  for (m=0; m < COMPARE_NUM_LEVELS; m++) {
	    pres_diff[m]+=table_entry.pres_diff[m];
	    hgt_diff[m]+=table_entry.hgt_diff[m];
	    temp_diff[m]+=table_entry.temp_diff[m];
	    dd_diff[m]+=table_entry.dd_diff[m];
	    ff_diff[m]+=table_entry.ff_diff[m];
	    std::cout << std::setw(6) << table_entry.pres_diff[m]/table_entry.npres_diff[m]
            << std::setw(6) << table_entry.hgt_diff[m]/table_entry.nhgt_diff[m] <<
            std::setw(6) << table_entry.temp_diff[m]/table_entry.ntemp_diff[m] <<
            std::setw(6) << table_entry.dd_diff[m]/table_entry.ndd_diff[m] << std::setw(6)
            << table_entry.ff_diff[m]/table_entry.nff_diff[m];
	  }
	  std::cout << std::endl;
	  std::cout.precision(0);
	  if (lcnt > 1)
	    std::cout << " *";
	  else
	    std::cout << "  ";
	  std::cout << std::setw(6) << std::setfill('0') << n << std::setfill(' ') << ":BAD COMPARES                 " << std::setw(6) << table_entry.nblat << std::setw(6) << table_entry.nblon << std::setw(6) << table_entry.nbelev;
	  for (m=0; m < COMPARE_NUM_LEVELS; m++)
	    std::cout << std::setw(6) << table_entry.nbpres[m] << std::setw(6) <<
            table_entry.nbhgt[m] << std::setw(6) << table_entry.nbtemp[m] <<
            std::setw(6) << table_entry.nbdd[m] << std::setw(6) << table_entry.nbff[m];
	  std::cout << std::endl;
	  num_lines+=4;
	}
	if (num_lines > 55) {
	  std::cout << "            NON-                                       ";
	  for (m=0; m < COMPARE_NUM_LEVELS; m++) {
	    if (pres[m] != 0)
		std::cout << "  |-------- " << std::setw(4) << pres[m] << " MB ---------|";
	    else
		std::cout << "  |----------- SFC ----------|";
	  }
	  std::cout << std::endl;
	  std::cout << " STATION COMPARES COMPARES  GOOD   BAD   LAT   LON  ELEV";
	  for (m=0; m < COMPARE_NUM_LEVELS; m++)
	    std::cout << "  PRES   HGT  TEMP    DD    FF";
	  std::cout << std::endl;
	  num_lines=2;
	}
    }
  }

  if (num_lines > 55)
    std::cout << " TOTALS";
  else
    std::cout << "  TOTALS";
  std::cout << std::setw(9) << nncomp << std::setw(9) << ncomp << std::setw(6) << ngood << std::setw(6)
    << nbad << std::setw(6) << nblat << std::setw(6) << nblon << std::setw(6) << nbelev <<
    std::endl;
  std::cout << "  TOTALS:TOTAL COMPARES                                 ";
  for (m=0; m < COMPARE_NUM_LEVELS; m++) {
    std::cout << std::setw(6) << npres_diff[m] << std::setw(6) << nhgt_diff[m] << std::setw(6) <<
      ntemp_diff[m] << std::setw(6) << ndd_diff[m] << std::setw(6) << nff_diff[m];
    if (floatutils::myequalf(npres_diff[m],0.)) npres_diff[m]=1.;
    if (floatutils::myequalf(nhgt_diff[m],0.)) nhgt_diff[m]=1.;
    if (floatutils::myequalf(ntemp_diff[m],0.)) ntemp_diff[m]=1.;
    if (floatutils::myequalf(ndd_diff[m],0.)) ndd_diff[m]=1.;
    if (floatutils::myequalf(nff_diff[m],0.)) nff_diff[m]=1.;
  }
  std::cout << std::endl;
  std::cout.precision(2);
  std::cout << "  TOTALS:AVERAGE DIFFERENCES                            ";
  for (m=0; m < COMPARE_NUM_LEVELS; m++)
    std::cout << std::setw(6) << pres_diff[m]/npres_diff[m] << std::setw(6) <<
      hgt_diff[m]/nhgt_diff[m] << std::setw(6) << temp_diff[m]/ntemp_diff[m] <<
      std::setw(6) << dd_diff[m]/ndd_diff[m] << std::setw(6) << ff_diff[m]/nff_diff[m];
  std::cout << std::endl;
  std::cout.precision(0);

  std::cout << "SUMMARY OF COMPARES BY DATE/TIME\n" << std::endl;
  std::cout << "                                           -------------------------"
    << "------------- NUMBER OF BAD COMPARES BY: ------------------------------"
    << "---------" << std::endl;
  std::cout << "               NON-                                       ";
  for (n=0; n < COMPARE_NUM_LEVELS; n++) {
    if (pres[n] != 0)
	std::cout << "  |-------- " << std::setw(4) << pres[n] << " MB ---------|";
    else
	std::cout << "  |----------- SFC ----------|";
  }
  std::cout << std::endl;
  std::cout << "  YYMMDDHH COMPARES COMPARES  GOOD   BAD   LAT   LON  ELEV";
  for (n=0; n < COMPARE_NUM_LEVELS; n++)
    std::cout << "  PRES   HGT  TEMP    DD    FF";
  std::cout << std::endl;
  num_lines=5;
  nncomp=ncomp=ngood=nbad=nblat=nblon=nbelev=0;
  for (n=min_date; n <= max_date; n++) {
    if (date_table.found(n,table_entry)) {
	nncomp+=table_entry.nncomp;
	ncomp+=table_entry.ncomp;
	ngood+=table_entry.ngood;
	nbad+=table_entry.nbad;
	nblat+=table_entry.nblat;
	nblon+=table_entry.nblon;
	nbelev+=table_entry.nbelev;
	std::cout << std::setw(10) << n << std::setw(9) << table_entry.nncomp << std::setw(9) <<
        table_entry.ncomp;
	if (table_entry.ncomp == 0)
	  std::cout << std::endl;
	else {
	  std::cout << std::setw(6) << table_entry.ngood << std::setw(6) << table_entry.nbad <<
          std::setw(6) << table_entry.nblat << std::setw(6) << table_entry.nblon <<
          std::setw(6) << table_entry.nbelev;
	  lcnt=0;
	  for (m=0; m < COMPARE_NUM_LEVELS; m++) {
	    if (table_entry.nbhgt[m]/static_cast<float>(table_entry.ncomp) > 0.49999 || table_entry.nbtemp[m]/static_cast<float>(table_entry.ncomp) > 0.49999 || table_entry.nbdd[m]/static_cast<float>(table_entry.ncomp) > 0.49999 || table_entry.nbff[m]/static_cast<float>(table_entry.ncomp) > 0.49999)
		lcnt++;
	    std::cout << std::setw(6) << table_entry.nbpres[m] << std::setw(6) <<
            table_entry.nbhgt[m] << std::setw(6) << table_entry.nbtemp[m] <<
            std::setw(6) << table_entry.nbdd[m] << std::setw(6) << table_entry.nbff[m];
	  }
	  if (lcnt > 1)
	    std::cout << "*";
	  std::cout << std::endl;
	}
	num_lines++;
	if (num_lines > 55) {
	  std::cout << "                                           -----------------"
          << "--------------------- NUMBER OF BAD COMPARES BY: ----------------"
          << "-----------------------" << std::endl;
	  std::cout << "               NON-                                       ";
	  for (m=0; m < COMPARE_NUM_LEVELS; m++) {
	    if (pres[m] != 0)
		std::cout << "  |-------- " << std::setw(4) << pres[m] << " MB ---------|";
	    else
		std::cout << "  |----------- SFC ----------|";
	  }
	  std::cout << std::endl;
	  std::cout << "  YYMMDDHH COMPARES COMPARES  GOOD   BAD   LAT   LON  ELEV";
	  for (m=0; m < COMPARE_NUM_LEVELS; m++)
	    std::cout << "  PRES   HGT  TEMP    DD    FF";
	  std::cout << std::endl;
	  num_lines=2;
	}
    }
  }

  std::cout << "    TOTALS" << std::setw(9) << nncomp << std::setw(9) << ncomp << std::setw(6) <<
    ngood << std::setw(6) << nbad << std::setw(6) << nblat << std::setw(6) << nblon << std::setw(6)
    << nbelev << std::endl;
}

void CompareRecord::fill(const char *input_buffer)
{
  size_t n,off;

  strutils::strget(input_buffer+1,src,2);
  strutils::strget(input_buffer+4,stn,6);
  strutils::strget(input_buffer+12,yr,2);
  strutils::strget(input_buffer+14,mo,2);
  strutils::strget(input_buffer+16,dy,2);
  strutils::strget(input_buffer+18,hr,2);
  strutils::strget(input_buffer+25,lat,6);
  strutils::strget(input_buffer+31,lon,7);
  strutils::strget(input_buffer+38,elev,5);

  off=43;
  for (n=0; n < COMPARE_NUM_LEVELS; n++) {
    strutils::strget(input_buffer+off,pres[n],5);
    strutils::strget(input_buffer+off+5,hgt[n],6);
    strutils::strget(input_buffer+off+11,temp[n],5);
    temp[n]*=0.1;
    strutils::strget(input_buffer+off+16,dd[n],5);
    strutils::strget(input_buffer+off+21,ff[n],5);
    ff[n]*=0.1;
    off+=26;
  }
}

size_t CompareRecord::getDate() const
{
  return yr*1000000+mo*10000+dy*100+hr;
}

bool operator==(const CompareRecord& a,const CompareRecord &b)
{
  int date_diff;

  if (a.stn != b.stn)
    return false;
  date_diff=dateutils::julian_day(1900+a.yr,a.mo,a.dy)-dateutils::julian_day(1900+b.yr,b.mo,b.dy);
  if (date_diff > 1 || date_diff < -1)
    return false;
  else {
    if (date_diff == 0 && abs(a.hr-b.hr) > 1)
	return false;
    else {
	if ((date_diff == 1 && (a.hr+24-b.hr) > 1) || (date_diff == -1 &&
          (b.hr+24-a.hr) > 1))
	  return false;
    }
  }

  return true;
}

bool operator<(const CompareRecord& a,const CompareRecord& b)
{
  if (a.stn < b.stn || (a.stn == b.stn && a.getDate() < b.getDate()))
    return true;
  else
    return false;
}

bool operator>(const CompareRecord& a,const CompareRecord& b)
{
  if (a.stn > b.stn || (a.stn == b.stn && a.getDate() > b.getDate()))
    return true;
  else
    return false;
}

bool isGoodCompare(const CompareRecord& a,const CompareRecord& b,const CompareTolerances& tol,CompareDifferences& comp_diff)
{
  size_t n;
  size_t num_good[COMPARE_NUM_LEVELS];
  bool is_good=true;
  static CompareDifferences::Count stn_entry,date_entry;

  if (!comp_diff.stn_table.found(a.stn,stn_entry)) {
    stn_entry.key=a.stn;
    stn_entry.nncomp=stn_entry.ncomp=0;
    stn_entry.ngood=stn_entry.nbad=0;
    stn_entry.nblat=stn_entry.nblon=stn_entry.nbelev=0;
    stn_entry.elev_diff=stn_entry.nelev_diff=0;
    for (n=0; n < COMPARE_NUM_LEVELS; n++) {
	stn_entry.nbpres[n]=stn_entry.nbhgt[n]=stn_entry.nbtemp[n]=
        stn_entry.nbdd[n]=stn_entry.nbff[n]=0;
	stn_entry.pres_diff[n]=stn_entry.hgt_diff[n]=stn_entry.temp_diff[n]=
        stn_entry.dd_diff[n]=stn_entry.ff_diff[n]=0.;
	stn_entry.npres_diff[n]=stn_entry.nhgt_diff[n]=stn_entry.ntemp_diff[n]=
        stn_entry.ndd_diff[n]=stn_entry.nff_diff[n]=0.;
    }
  }
  stn_entry.ncomp++;
  if (!comp_diff.date_table.found(a.getDate(),date_entry)) {
    date_entry.key=a.getDate();
    date_entry.nncomp=date_entry.ncomp=0;
    date_entry.ngood=date_entry.nbad=0;
    date_entry.nblat=date_entry.nblon=date_entry.nbelev=0;
    date_entry.elev_diff=date_entry.nelev_diff=0;
    for (n=0; n < COMPARE_NUM_LEVELS; n++) {
	date_entry.nbpres[n]=date_entry.nbhgt[n]=date_entry.nbtemp[n]=
        date_entry.nbdd[n]=date_entry.nbff[n]=0;
	date_entry.pres_diff[n]=date_entry.hgt_diff[n]=date_entry.temp_diff[n]=
        date_entry.dd_diff[n]=date_entry.ff_diff[n]=0.;
	date_entry.npres_diff[n]=date_entry.nhgt_diff[n]=date_entry.ntemp_diff[n]=
        date_entry.ndd_diff[n]=date_entry.nff_diff[n]=
        0.;
    }
  }
  date_entry.ncomp++;

  comp_diff.stn=a.stn;
  comp_diff.date=a.getDate();
  for (n=0; n < COMPARE_NUM_LEVELS; n++) {
    comp_diff.pres[n]=a.pres[n];
    if (comp_diff.pres[n] != b.pres[n]) comp_diff.pres[n]=0;
  }
  if (tol.lat >= 0) {
    comp_diff.lat= (a.lat > -99. && b.lat > -99.) ? (a.lat-b.lat) : 999.99;
    if (comp_diff.lat < 999. && fabs(comp_diff.lat) > tol.lat) {
	stn_entry.nblat++;
	date_entry.nblat++;
	is_good=false;
    }
  }
  else {
    std::cerr << "Error: negative latitude tolerance not allowed" << std::endl;
    exit(1);
  }
  if (tol.lon >= 0) {
    comp_diff.lon= (a.lon > -199. && b.lon > -199.) ? (a.lon-b.lon) : 999.99;
    if (comp_diff.lon < 999. && fabs(comp_diff.lon) > tol.lon) {
	stn_entry.nblon++;
	date_entry.nblon++;
	is_good=false;
    }
  }
  else {
    std::cerr << "Error: negative longitude tolerance not allowed" << std::endl;
    exit(1);
  }
  if (tol.elev >= 0) {
    comp_diff.elev= (a.elev > -999 && b.elev > -999) ? (a.elev-b.elev) : 9999;
    if (comp_diff.elev != 9999 && abs(comp_diff.elev) > tol.elev) {
	stn_entry.nbelev++;
	date_entry.nbelev++;
	is_good=false;
    }
    stn_entry.elev_diff+=comp_diff.elev;
    stn_entry.nelev_diff++;
    date_entry.elev_diff+=comp_diff.elev;
    date_entry.nelev_diff++;
  }
  else {
    std::cerr << "Error: negative elevation tolerance not allowed" << std::endl;
    exit(1);
  }
  for (n=0; n < COMPARE_NUM_LEVELS; n++) {
    num_good[n]=0;
    if (tol.hgt >= 0) {
	if (a.hgt[n] > -999 && b.hgt[n] > -999) {
	  comp_diff.hgt[n]=a.hgt[n]-b.hgt[n];
	  if (abs(comp_diff.hgt[n]) > tol.hgt) {
	    stn_entry.nbhgt[n]++;
	    date_entry.nbhgt[n]++;
	  }
	  else
	    num_good[n]++;
	  stn_entry.hgt_diff[n]+=comp_diff.hgt[n];
	  stn_entry.nhgt_diff[n]++;
	  date_entry.hgt_diff[n]+=comp_diff.hgt[n];
	  date_entry.nhgt_diff[n]++;
	}
	else {
	  comp_diff.hgt[n]=9999;
	  num_good[n]++;
	}
    }
    else {
	std::cerr << "Error: negative height tolerance not allowed" << std::endl;
	exit(1);
    }
    if (tol.temp >= 0) {
	if (a.temp[n] > -99. && b.temp[n] > -99.) {
	  comp_diff.temp[n]=a.temp[n]-b.temp[n];
	  if (fabs(comp_diff.temp[n]) > tol.temp) {
	    stn_entry.nbtemp[n]++;
	    date_entry.nbtemp[n]++;
	  }
	  else
	    num_good[n]++;
	  stn_entry.temp_diff[n]+=comp_diff.temp[n];
	  stn_entry.ntemp_diff[n]++;
	  date_entry.temp_diff[n]+=comp_diff.temp[n];
	  date_entry.ntemp_diff[n]++;
	}
	else {
	  comp_diff.temp[n]=999.9;
	  num_good[n]++;
	}
    }
    else {
	std::cerr << "Error: negative temperature tolerance not allowed" << std::endl;
	exit(1);
    }
    if (tol.dd >= 0) {
	if (a.dd[n] >= 0 && b.dd[n] >= 0) {
	  comp_diff.dd[n]=a.dd[n]-b.dd[n];
	  if (abs(comp_diff.dd[n]) >= 180)
	    comp_diff.dd[n]= (a.dd[n] > b.dd[n]) ? (360+b.dd[n]-a.dd[n]) : (360+a.dd[n]-b.dd[n]);
	  if (abs(comp_diff.dd[n]) > tol.dd) {
	    stn_entry.nbdd[n]++;
	    date_entry.nbdd[n]++;
	  }
	  else
	    num_good[n]++;
	  stn_entry.dd_diff[n]+=comp_diff.dd[n];
	  stn_entry.ndd_diff[n]++;
	  date_entry.dd_diff[n]+=comp_diff.dd[n];
	  date_entry.ndd_diff[n]++;
	}
	else {
	  comp_diff.dd[n]=9999;
	  num_good[n]++;
	}
    }
    else {
	std::cerr << "Error: negative wind direction tolerance not allowed" << std::endl;
	exit(1);
    }
    if (tol.ff >= 0) {
	if (a.ff[n] > -99. && b.ff[n] > -99.) {
	  comp_diff.ff[n]=a.ff[n]-b.ff[n];
	  if (fabs(comp_diff.ff[n]) > tol.ff) {
	    stn_entry.nbff[n]++;
	    date_entry.nbff[n]++;
	  }
	  else
	    num_good[n]++;
	  stn_entry.ff_diff[n]+=comp_diff.ff[n];
	  stn_entry.nff_diff[n]++;
	  date_entry.ff_diff[n]+=comp_diff.ff[n];
	  date_entry.nff_diff[n]++;
	}
	else {
	  comp_diff.ff[n]=999.9;
	  num_good[n]++;
	}
    }
    else {
	std::cerr << "Error: negative wind speed tolerance not allowed" << std::endl;
	exit(1);
    }
  }

  if (is_good) {
    for (n=0; n < COMPARE_NUM_LEVELS; n++) {
	if (num_good[n] < 3)
	  is_good=false;
    }
  }
  if (is_good) {
    stn_entry.ngood++;
    date_entry.ngood++;
  }
  else {
    stn_entry.nbad++;
    date_entry.nbad++;
  }
  comp_diff.stn_table.replace(stn_entry);
  if (a.stn < comp_diff.min_stn) comp_diff.min_stn=a.stn;
  if (a.stn > comp_diff.max_stn) comp_diff.max_stn=a.stn;
  comp_diff.date_table.replace(date_entry);
  if (a.getDate() < comp_diff.min_date) comp_diff.min_date=a.getDate();
  if (a.getDate() > comp_diff.max_date) comp_diff.max_date=a.getDate();

  return is_good;
}

std::ostream& operator<<(std::ostream& out_stream,const CompareDifferences& source)
{
  size_t n;

  out_stream.setf(std::ios::fixed);
  out_stream.precision(2);
  out_stream << "STN:" << std::setw(6) << std::setfill('0') << source.stn << std::setfill(' ') << " DATE:" << std::setw(8) << source.date << " DIFFS:      "
    << std::setw(6) << source.lat << std::setw(8) << source.lon << std::setw(5) << source.elev;
  out_stream.precision(1);
  for (n=0; n < COMPARE_NUM_LEVELS; n++) {
    out_stream << "       " << std::setw(5) << source.hgt[n] << std::setw(6) <<
      source.temp[n] << std::setw(5) << source.dd[n] << std::setw(6) << source.ff[n];
  }

  return out_stream;
}

std::ostream& operator<<(std::ostream& out_stream,const CompareRecord& source)
{
  size_t n;

  out_stream.setf(std::ios::fixed);
  out_stream.precision(2);
  out_stream << "STN:" << std::setfill('0') << std::setw(6) << source.stn << std::setfill(' ') << " DATE:" << source.getDate() << " SRC:" << std::setw(2) << source.src << " LOC: " << std::setw(6) << source.lat << std::setw(8) << source.lon << std::setw(5) << source.elev;
  out_stream.precision(1);
  for (n=0; n < COMPARE_NUM_LEVELS; n++) {
    out_stream << std::setw(6) << source.pres[n] << ":" << std::setw(5) << source.hgt[n]
      << std::setw(6) << source.temp[n] << std::setw(5) << source.dd[n] << std::setw(6) <<
      source.ff[n];
  }

  return out_stream;
}
