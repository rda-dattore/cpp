// FILE: wbantable.h

#ifndef WBANTABLE_H
#define   WBANTABLE_H

#include <list>
#include <mymap.hpp>
#include <datetime.hpp>

class WBANTable
{
public:
  struct StationEntry {
    StationEntry() : wban_number(0),wmo_number(0),start(),end() {}

    size_t wban_number,wmo_number;
    DateTime start,end;
  };
  WBANTable() : wban_table(),wmo_table() {}
  WBANTable(const char *filename) : WBANTable() { fill(filename); }
  bool fill(const char *filename);
  size_t getWBANNumber(size_t wmo_number,DateTime date_time);
  size_t getWMONumber(size_t wban_number,DateTime date_time);
  friend bool operator!=(const StationEntry& a,const StationEntry& b);

private:
  struct WBANEntry {
    WBANEntry() : key(),list() {}

    size_t key;
    std::shared_ptr<std::list<StationEntry>> list;
  };
  struct WMOEntry {
    WMOEntry() : key(),list() {}

    size_t key;
    std::shared_ptr<std::list<StationEntry>> list;
  };
  my::map<WBANEntry> wban_table;
  my::map<WMOEntry> wmo_table;
};

#endif
