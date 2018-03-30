// FILE: adpsfc.cpp

#include <iomanip>
#include <surface.hpp>
#include <strutils.hpp>

void ADPSurfaceReport::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  char *buffer=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  short yr,mo,dy,hund;
  int units,idum;
  std::string dum;
  int rpt_len,off,next;

  strutils::strget(buffer,units,2);
  strutils::strget(&buffer[2],hund,2);
  strutils::strget(&buffer[4],yr,2);
  strutils::strget(&buffer[6],mo,2);
  strutils::strget(&buffer[8],dy,2);
  if (mo != 0 && dy != 0) {
    if (yr < 70) {
	yr+=2000;
    }
    else {
	yr+=1900;
    }
  }
  synoptic_date_time_.set(yr,mo,dy,units*10000+lround(hund*60./100.)*100);
  location_.ID.assign(&buffer[20],6);
  strutils::trim(location_.ID);
  while (location_.ID.length() < 6) {
    location_.ID=" "+location_.ID;
  }
  strutils::strget(&buffer[26],units,2);
  strutils::strget(&buffer[28],hund,2);
  if (synoptic_date_time_.time() == 0 && units > 18) {
    date_time_=synoptic_date_time_.hours_subtracted(24);
    date_time_.set_time(units*10000+lround(hund*60./100.)*100);
  }
  else {
    date_time_.set(yr,mo,dy,units*10000+lround(hund*60./100.)*100);
  }
  strutils::strget(&buffer[30],units,2);
  strutils::strget(&buffer[32],hund,2);
  if (synoptic_date_time_.time() == 0 && units > 18) {
    receipt_date_time_=synoptic_date_time_.hours_subtracted(24);
    receipt_date_time_.set_time(units*10000+lround(hund*60./100.)*100);
  }
  else if (synoptic_date_time_.time() == 1800 && units < 15) {
    receipt_date_time_=synoptic_date_time_.hours_added(24);
    receipt_date_time_.set_time(units*10000+lround(hund*60./100.)*100);
  }
  else {
    receipt_date_time_.set(yr,mo,dy,units*10000+lround(hund*60./100.)*100);
  }
  strutils::strget(&buffer[37],report_type_,3);
  strutils::strget(&buffer[10],units,5);
  if (units != 99999 && units <= 9000 and units >= -9000) {
    location_.latitude=units/100.;
  }
  else {
    location_.latitude=-99.9;
  }
  strutils::strget(&buffer[15],units,5);
  if (units != 99999 && units >= 0 && units <= 36000) {
    location_.longitude= (units == 0) ? 0. : 360.-(units/100.);
  }
  else {
    location_.longitude=-999.9;
  }
  strutils::strget(&buffer[40],location_.elevation,5);
  if (fill_header_only) {
    return;
  }
  report_.data.slp=9999.9;
  report_.data.stnp=9999.9;
  report_.data.dd=999.;
  report_.data.ff=999.;
  report_.data.tdry=999.9;
  report_.data.tdew=999.9;
  addl_data.data.tmax=999.9;
  addl_data.data.tmin=999.9;
  addl_data.data.pcp_amt=99.99;
  addl_data.data.snowd=999.;
  addl_data.data.vis=999.;
  adp_addl_data.present_wx_code=999;
  adp_addl_data.past_wx_code=99;
  addl_data.data.tot_cloud=99.;
  adp_addl_data.low_cloud_type=99;
  adp_addl_data.lowmid_cloud_amount=99;
  addl_data.data.cloud_base_hgt=99.;
  adp_addl_data.middle_cloud_type=99;
  adp_addl_data.high_cloud_type=99;
  addl_data.data.pres_tend=99.9;
  adp_addl_data.precipitation_amount_24=99.99;
  adp_addl_data.precipitation_time=9;
  strutils::strget(&buffer[47],rpt_len,3);
  rpt_len*=10;
  off=50;
  while (off < rpt_len) {
    dum.assign(&buffer[off],2);
    if (strutils::is_numeric(dum)) {
	if (dum != "00" && dum != "09" && dum != "99") {
// category 51
	  if (dum == "51") {
	    strutils::strget(&buffer[off+10],idum,5);
	    report_.data.slp=idum/10.;
	    strutils::strget(&buffer[off+15],idum,5);
	    report_.data.stnp=idum/10.;
	    strutils::strget(&buffer[off+20],idum,3);
	    report_.data.dd=idum;
	    strutils::strget(&buffer[off+23],idum,3);
	    report_.data.ff=idum;
	    strutils::strget(&buffer[off+26],idum,4);
	    report_.data.tdry=idum/10.;
	    if (report_.data.tdry < 999.) {
		strutils::strget(&buffer[off+30],idum,3);
		report_.data.tdew=report_.data.tdry-idum/10.;
	    }
	    else {
		report_.data.tdew=999.9;
	    }
	    strutils::strget(&buffer[off+33],addl_data.data.tmax,4);
	    addl_data.data.tmax/=10.;
	    strutils::strget(&buffer[off+37],addl_data.data.tmin,4);
	    addl_data.data.tmin/=10.;
	    strutils::strget(&buffer[off+41],report_.flags.slp,1);
	    strutils::strget(&buffer[off+42],report_.flags.stnp,1);
	    strutils::strget(&buffer[off+43],report_.flags.dd,1);
	    report_.flags.ff=report_.flags.dd;
	    strutils::strget(&buffer[off+44],report_.flags.tdry,1);
	    strutils::strget(&buffer[off+46],addl_data.data.vis,3);
	    strutils::strget(&buffer[off+49],adp_addl_data.present_wx_code,3);
	    strutils::strget(&buffer[off+52],adp_addl_data.past_wx_code,2);
	    strutils::strget(&buffer[off+54],addl_data.data.tot_cloud,2);
	    strutils::strget(&buffer[off+56],adp_addl_data.lowmid_cloud_amount,2);
	    strutils::strget(&buffer[off+58],adp_addl_data.low_cloud_type,2);
	    strutils::strget(&buffer[off+60],addl_data.data.cloud_base_hgt,2);
	    strutils::strget(&buffer[off+62],adp_addl_data.middle_cloud_type,2);
	    strutils::strget(&buffer[off+64],adp_addl_data.high_cloud_type,2);
	    addl_data.data.pres_tend=buffer[off+66];
	    strutils::strget(&buffer[off+67],addl_data.data.pres_tend,3);
	    addl_data.data.pres_tend/=10.;
	  }
	  else if (dum == "52") {
	    strutils::strget(&buffer[off+10],addl_data.data.pcp_amt,4);
	    addl_data.data.pcp_amt/=100.;
	    strutils::strget(&buffer[off+14],addl_data.data.snowd,3);
	    strutils::strget(&buffer[off+17],adp_addl_data.precipitation_amount_24,4);
	    adp_addl_data.precipitation_amount_24/=100.;
	    adp_addl_data.precipitation_time=buffer[off+21];
	  }
	}
	else {
	  break;
	}
    }
    else {
	break;
    }
    dum.assign(&buffer[off+2],3);
    if (strutils::is_numeric(dum)) {
	next=std::stoi(dum);
    }
    else {
	break;
    }
    off=next*10;
  }
}

void ADPSurfaceReport::print(std::ostream& outs) const
{
  print_header(outs,true);
}

void ADPSurfaceReport::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(2);

  if (verbose)
    outs << "ID: " << location_.ID << "  Date: " << date_time_.to_string() << "Z  Location: " << std::setw(6) << location_.latitude << std::setw(8) << location_.longitude << std::setw(6) << location_.elevation << std::endl;
  else {
  }
}
