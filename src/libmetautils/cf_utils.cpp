#include <metadata.hpp>
#include <strutils.hpp>

namespace metautils {

namespace CF {

void fill_nc_time_data(std::string units_attribute_value,NcTime::TimeData& time_data,std::string user)
{
  if (std::regex_search(units_attribute_value,std::regex("since"))) {
    auto idx=units_attribute_value.find("since");
    time_data.units=strutils::to_lower(units_attribute_value.substr(0,idx));
    strutils::trim(time_data.units);
    units_attribute_value=units_attribute_value.substr(idx+5);
    while (!units_attribute_value.empty() && (units_attribute_value[0] < '0' || units_attribute_value[0] > '9')) {
	units_attribute_value=units_attribute_value.substr(1);
    }
    auto n=units_attribute_value.length()-1;
    while (n > 0 && (units_attribute_value[n] < '0' || units_attribute_value[n] > '9')) {
	--n;
    }
    ++n;
    if (n < units_attribute_value.length()) {
        units_attribute_value=units_attribute_value.substr(0,n);
    }
    strutils::trim(units_attribute_value);
    auto uparts=strutils::split(units_attribute_value);
    if (uparts.size() < 1 || uparts.size() > 3) {
	metautils::log_error("fill_nc_time_data(): unable to get reference time from units specified as: '"+units_attribute_value+"'","nc2xml",user);
    }
    auto dparts=strutils::split(uparts[0],"-");
    if (dparts.size() != 3) {
        metautils::log_error("fill_nc_time_data(): unable to get reference time from units specified as: '"+units_attribute_value+"'","nc2xml",user);
    }
    time_data.reference.set_year(std::stoi(dparts[0]));
    time_data.reference.set_month(std::stoi(dparts[1]));
    time_data.reference.set_day(std::stoi(dparts[2]));
    if (uparts.size() > 1) {
	auto tparts=strutils::split(uparts[1],":");
	switch (tparts.size()) {
	  case 1: {
	    time_data.reference.set_time(std::stoi(tparts[0])*10000);
	    break;
	  }
	  case 2: {
	    time_data.reference.set_time(std::stoi(tparts[0])*10000+std::stoi(tparts[1])*100);
	    break;
	  }
	  case 3: {
	    time_data.reference.set_time(std::stoi(tparts[0])*10000+std::stoi(tparts[1])*100+static_cast<int>(std::stof(tparts[2])));
	    break;
	  }
	}
    }
  }
  else {
    metautils::log_error("fill_nc_time_data(): unable to get CF time from time variable units","nc2xml",user);
  }
}

} // end namespace CF

} // end namespace metautils
