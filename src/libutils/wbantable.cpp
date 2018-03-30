#include <wbantable.hpp>
#include <strutils.hpp>

bool WBANTable::fill(const char *filename)
{
  FILE *fp;
  if ( (fp=fopen(filename,"r")) == NULL) {
    return false;
  }
  const size_t BUF_LEN=256;
  char buffer[BUF_LEN];
  WMOEntry wmo_entry;
  WBANEntry wban_entry;
  size_t last_wmo=0,last_wban=0;
  while (fgets(buffer,BUF_LEN,fp) != NULL) {
    size_t wmo;
    strutils::strget(&buffer[1],wmo,5);
    if (last_wmo == 0) {
	last_wmo=wmo;
    }
    size_t wban;
    strutils::strget(&buffer[7],wban,5);
    if (last_wban == 0) {
	last_wban=wban;
    }
    if (wban != last_wban) {
	wban_entry.key=last_wban;
	wban_entry.list.reset(new std::list<StationEntry>);
	wban_table.insert(wban_entry);
	last_wban=wban;
    }
    if (wmo != last_wmo) {
	WMOEntry check;
	if (wmo_table.found(last_wmo,check)) {
	  wmo_table.replace(wmo_entry);
	}
	else {
	  wmo_entry.key=last_wmo;
	  wmo_entry.list.reset(new std::list<StationEntry>);
	  wmo_table.insert(wmo_entry);
	}
	last_wmo=wmo;
    }

    if (wmo_entry.list->size() == 0) {
	wmo_table.found(wmo,wmo_entry);
    }
    StationEntry wban_list;
    wban_list.wmo_number=wmo;
    StationEntry wmo_list;
    wmo_list.wban_number=wban;
    if (std::string(&buffer[15],6) == "     0") {
	wban_list.start.set(0,0,0,0);
	wmo_list.start.set(0,0,0,0);
    }
    else {
	short yr,mo,dy;
	strutils::strget(&buffer[15],yr,2);
	strutils::strget(&buffer[17],mo,2);
	strutils::strget(&buffer[19],dy,2);
	wban_list.start.set(1900+yr,mo,dy,0);
	wmo_list.start.set(1900+yr,mo,dy,0);
    }
    if (std::string(&buffer[24],6) == "     0") {
	wban_list.end.set(9999,12,31,2359);
	wmo_list.end.set(9999,12,31,2359);
    }
    else {
	short yr,mo,dy;
	strutils::strget(&buffer[24],yr,2);
	strutils::strget(&buffer[26],mo,2);
	strutils::strget(&buffer[28],dy,2);
	wban_list.end.set(1900+yr,mo,dy,2359);
	wmo_list.end.set(1900+yr,mo,dy,2359);
    }
    wban_entry.list->emplace_back(wban_list);
    wmo_entry.list->emplace_back(wmo_list);
  }

  return true;
}

size_t WBANTable::getWMONumber(size_t wban_number,DateTime date_time)
{
  WBANEntry wban_entry;

  if (wban_table.found(wban_number,wban_entry)) {
    for (auto& item : *wban_entry.list) {
	if (date_time >= item.start && date_time <= item.end) {
	  return item.wmo_number;
	}
    }
    return 0;
  }

  return 0;
}

size_t WBANTable::getWBANNumber(size_t wmo_number,DateTime date_time)
{
  WMOEntry wmo_entry;

  if (wmo_table.found(wmo_number,wmo_entry)) {
    for (auto& item : *wmo_entry.list) {
	if (date_time >= item.start && date_time <= item.end) {
	  return item.wban_number;
	}
    }
  }

  return 0;
}

bool operator!=(const WBANTable::StationEntry& a,const WBANTable::StationEntry& b)
{
  if (a.wban_number != b.wban_number) {
    return true;
  }
  if (a.wmo_number != b.wmo_number) {
    return true;
  }
  if (a.start != b.start) {
    return true;
  }
  if (a.end != b.end) {
    return true;
  }
  return false;
}
