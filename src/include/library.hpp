// FILE: library.h
//

#ifndef LIBRARY_H
#define   LIBRARY_H

#include <stdlib.h>
#include <list>
#include <datetime.hpp>
#include <mymap.hpp>

class DSSStationLibrary
{
public:
  struct LibraryEntry {
    LibraryEntry() : start(),end(),lat(0.),lon(0.),elev(0) {}

    DateTime start,end;
    float lat,lon;
    int elev;
  };
  struct LibraryHistory {
    LibraryHistory() : key(),historyList() {}

    std::string key;
    std::shared_ptr<std::list<LibraryEntry>> historyList;
  };
  DSSStationLibrary() : historyTable() {}
  DSSStationLibrary(std::string library_file) : DSSStationLibrary() { fill(library_file); }
  bool fill(std::string library_file);
  LibraryEntry getEntryFromHistory(std::string ID,DateTime date_time);
  size_t getNumberOfIDs() const { return historyTable.size(); }

private:
  my::map<LibraryHistory> historyTable;
};

/*
class LibEntry
{
public:
  LibEntry();
  void fill(const char *buf);
  void recordHit() { nhits++; ntries++; }
  void recordMiss(int miss_flg);
  void addEntry(size_t station,float latitude,float longitude,int elevation);
  void setStation(size_t station) { stn=station; }
  bool isTooFar(float latitude,float longitude,float tolerance,float& computed_distance);
  bool isBadElev(int elevation,int tolerance,int& elevation_difference);
  size_t getEntryType() const { return type; }
  size_t getEntryNumber() const { return entry_num; }
  size_t getNumEntries() const { return num_entries; }
  size_t getStation() const { return stn; }
  long long getStartDateTime() const { return start_date_time; }
  long long getEndDateTime() const { return end_date_time; }
  float getLatitude() const { return lat; }
  float getLongitude() const { return lon; }
  int getElevation() const { return elev; }
  size_t getHitCount() const { return nhits; }
  void getMissCount(size_t& num_before,size_t& num_after,size_t& num_notfound) const { num_before=nprev; num_after=npost; num_notfound=nnf; }
  size_t getTryCount() const { return ntries; }
  bool isHit(size_t entry_type,size_t station,long long date_time,int& miss_type) const;

private:
  long long start_date_time,end_date_time;
  size_t type,stn,entry_num,num_entries;
  float lat,lon;
  int elev;
  size_t nhits,nprev,npost,nnf,ntries;
  bool too_far,bad_elev;
};
*/

#endif
