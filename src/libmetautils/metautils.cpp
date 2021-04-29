#include <metadata.hpp>
#include <strutils.hpp>
#include <datetime.hpp>

namespace metautils {

void obs_per(std::string observationTypeValue,size_t numObs,DateTime start,DateTime end,double& obsper,std::string& unit)
{
  size_t num_days[]={0,31,28,31,30,31,30,31,31,30,31,30,31};
  const float TOLERANCE=0.15;
  size_t idx;
  int nyrs,climo_len,nper;
  std::string sdum;

  if (strutils::contains(observationTypeValue,"climatology")) {
    if ( (idx=observationTypeValue.find("year")-1) != std::string::npos) {
	while (idx > 0 && observationTypeValue[idx] != '_')
	  idx--;
    }
    sdum=observationTypeValue.substr(idx+1);
    sdum=sdum.substr(0,sdum.find("-year"));
    climo_len=std::stoi(sdum);
    nyrs=end.year()-start.year();
    if (nyrs >= climo_len) {
	if (climo_len == 30) {
	  nper=((nyrs % climo_len)/10)+1;
	}
	else {
	  nper=nyrs/climo_len;
	}
	obsper=numObs/static_cast<float>(nper);
	if (static_cast<int>(numObs) == nper) {
	  unit="year";
	}
	else if (numObs/nper == 4) {
	  unit="season";
	}
	else if (numObs/nper == 12) {
	  unit="month";
	}
	else if (nper > 1 && start.month() != end.month()) {
	  nper=(nper-1)*12+12-start.month()+1;
	  if (static_cast<int>(numObs) == nper) {
	    unit="month";
	  }
	  else {
	    unit="";
	  }
	}
	else {
	  unit="";
	}
    }
    else {
	if ((end.month()-start.month()) == static_cast<int>(numObs)) {
	  unit="month";
	}
	else {
	  unit="";
	}
    }
    obsper=obsper-climo_len;
  }
  else {
    if (dateutils::is_leap_year(end.year())) {
	num_days[2]=29;
    }
    obsper=numObs/static_cast<double>(end.seconds_since(start));
    if ((obsper+TOLERANCE) > 1.) {
	unit="second";
    }
    else {
	obsper*=60.;
	if ((obsper+TOLERANCE) > 1.) {
	  unit="minute";
	}
	else {
	  obsper*=60;
	  if ((obsper+TOLERANCE) > 1.) {
	    unit="hour";
	  }
	  else {
	    obsper*=24;
	    if ((obsper+TOLERANCE) > 1.) {
		unit="day";
	    }
	    else {
		obsper*=7;
		if ((obsper+TOLERANCE) > 1.) {
		  unit="week";
		}
		else {
		  obsper*=(num_days[end.month()]/7.);
		  if ((obsper+TOLERANCE) > 1.) {
		    unit="month";
		  }
		  else {
		    obsper*=12;
		    if ((obsper+TOLERANCE) > 1.) {
			unit="year";
		    }
		    else {
			unit="";
		    }
		  }
		}
	    }
	  }
	}
    }
    num_days[2]=28;
  }
}

std::string web_home() {
  return "/glade/collections/rda/data/ds" + args.dsnum;
}

std::string relative_web_filename(std::string url)
{
  strutils::replace_all(url,"https://rda.ucar.edu","");
  strutils::replace_all(url,"http://rda.ucar.edu","");
  strutils::replace_all(url,"http://dss.ucar.edu","");
  if (std::regex_search(url,std::regex("^/dsszone"))) {
    strutils::replace_all(url,"/dsszone",directives.data_root_alias);
  }
  return strutils::substitute(url,directives.data_root_alias+"/ds"+args.dsnum+"/","");
}

std::string clean_id(std::string id)
{
  strutils::trim(id);
  for (size_t n=0; n < id.length(); ++n) {
    if (static_cast<int>(id[n]) < 32 || static_cast<int>(id[n]) > 127) {
	if (n > 0) {
	  id=id.substr(0,n)+"/"+id.substr(n+1);
	}
	else {
	  id="/"+id.substr(1);
	}
    }
  }
  strutils::replace_all(id,"\"","'");
  strutils::replace_all(id,"&","&amp;");
  strutils::replace_all(id,">","&gt;");
  strutils::replace_all(id,"<","&lt;");
  return strutils::to_upper(id);
}

} // end namespace metautils
