#include <fstream>
#include <library.hpp>
#include <strutils.hpp>

bool DSSStationLibrary::fill(std::string library_file)
{
  std::ifstream ifs;
  char line[256];
  LibraryHistory lh;
  LibraryEntry le;
  long long date;

  ifs.open(library_file.c_str());
  if (!ifs.is_open()) {
    return false;
  }
  ifs.getline(line,256);
  while (!ifs.eof()) {
    lh.key.assign(line,6);
    if (!historyTable.found(lh.key,lh)) {
	lh.historyList.reset(new std::list<LibraryEntry>);
	historyTable.insert(lh);
    }
    strutils::strget(&line[16],date,10);
    le.start.set(date*10000);
    strutils::strget(&line[27],date,10);
    le.end.set(date*10000);
    strutils::strget(&line[37],le.lat,6);
    strutils::strget(&line[44],le.lon,6);
    if (le.lon > 180.) {
	le.lon-=360.;
    }
    strutils::strget(&line[50],le.elev,8);
    lh.historyList->push_back(le);
    ifs.getline(line,256);
  }
  ifs.close();
  return true;
}

DSSStationLibrary::LibraryEntry DSSStationLibrary::getEntryFromHistory(std::string ID,DateTime date_time)
{
  LibraryHistory lh;
  LibraryEntry le;

  lh.key=ID;
  if (historyTable.found(lh.key,lh)) {
    for (auto& item : *lh.historyList) {
	le=item;
	if (date_time < le.start || (date_time >= le.start && date_time <= le.end)) {
	  break;
	}
    }
  }
  else {
    le.start.set(static_cast<long long>(0));
    le.end=le.start;
    le.lat=-99.99;
    le.lon=-999.99;
    le.elev=-999;
  }
  return le;
}

/*
LibEntry::LibEntry()
{
  type=stn=elev=0;
  start_date=end_date=0;
  lat=99.99;
  lon=999.99;
  nhits=nprev=npost=nnf=ntries=0;
  too_far=bad_elev=false;
}

void LibEntry::fill(const char *buf)
{
  strutils::strget(buf,stn,6);
  strutils::strget(buf+6,entry_num,4);
  strutils::strget(buf+10,num_entries,5);
  strutils::strget(buf+15,type,1);
  strutils::strget(buf+16,start_date,10);
  strutils::strget(buf+27,end_date,10);
  strutils::strget(buf+37,lat,6);
  strutils::strget(buf+44,lon,6);
  if (lon > 180.) lon-=360.;
  strutils::strget(buf+50,elev,8);
}

void LibEntry::recordMiss(int miss_flg)
{
  if (miss_flg < 0)
    nprev++;
  else if (miss_flg == 0)
    nnf++;
  else
    npost++;
  ntries++;
}

void LibEntry::addEntry(size_t station,float latitude,float longitude,int elevation)
{
  stn=station;
  lat=latitude;
  lon=longitude;
  elev=elevation;
}

bool LibEntry::isHit(size_t entry_type,size_t station,long long date,int& miss_type) const
{
  miss_type=0;
  if (start_date == 0 && (end_date == 0 || end_date == 23))
    return false;
  if (entry_type != 0 && entry_type != type)
    return false;
  if (station != stn)
    return false;
  if (date < start_date) {
    miss_type=-1;
    return false;
  }
  if (date > end_date) {
    miss_type=1;
    return false;
  }

  return true;
}

bool LibEntry::isTooFar(float latitude,float longitude,float tolerance,float& computed_distance)
{
  tolerance+=0.01;

  computed_distance=-1.;
  if ((lat == 0. && lon == 0.) || too_far)
    return false;

  if ((longitude >= 0. && lon >= 0.) || (longitude < 0. && lon < 0.))
    computed_distance=gc_distance(latitude,longitude,lat,lon);
  else {
    if (longitude < 0.)
	computed_distance=gc_distance(latitude,360.+longitude,lat,lon);
    else
	computed_distance=gc_distance(latitude,longitude,lat,360.+lon);
  }
  if (computed_distance <= tolerance)
    return false;

  too_far=true;
  return true;
}

bool LibEntry::isBadElev(int elevation,int tolerance,int& elevation_difference)
{
  elevation_difference=-1;
  if (elev == 99999 || bad_elev)
    return false;

  elevation_difference=abs(elevation-elev);
  if (elevation_difference <= tolerance)
    return false;

  bad_elev=true;
  return true;
}
*/
