#include <iomanip>
#include <surface.hpp>
#include <library.hpp>
#include <tempfile.hpp>
#include <strutils.hpp>
#include <utils.hpp>

DSSStationLibrary CPCSummaryObservation::library;

int InputCPCSummaryObservationStream::ignore()
{
return -1;
}

int InputCPCSummaryObservationStream::peek()
{
return -1;
}

int InputCPCSummaryObservationStream::read(unsigned char *buffer,size_t buffer_length)
{
  char *b=reinterpret_cast<char *>(const_cast<unsigned char *>(buffer));
  int num_bytes;
  if (icosstream != nullptr) {
    num_bytes=icosstream->read(buffer,buffer_length);
    if (num_bytes > 0) {
	++num_read;
    }
  }
  else if (fs.is_open()) {
    fs.getline(b,buffer_length);
    if (fs.eof()) {
	num_bytes=bfstream::eof;
    }
    else {
	num_bytes=std::string(b).length();
	if (b[num_bytes-1] == 0xa) {
	  b[--num_bytes]='\0';
	}
	++num_read;
    }
  }
  else {
    std::cerr << "Error: no open InputCPCSummaryObservationStream" << std::endl;
    num_bytes=-1;
  }
  return num_bytes;
}

void CPCSummaryObservation::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  char *buffer=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  short yr,mo,dy;
  std::string library_ID,sdum;
  DateTime library_date_time;
  DSSStationLibrary::LibraryEntry le;
  TempDir *work_dir;

  if (library.getNumberOfIDs() == 0) {
    work_dir=new TempDir;
    if (!work_dir->create("/tmp")) {
	std::cerr << "Error creating temporary directory" << std::endl;
	exit(1);
    }
    if (!library.fill(unixutils::remote_web_file("http://rda.ucar.edu/datasets/ds902.0/sf.7601_0012.adp.40",work_dir->name()))) {
	std::cerr << "Error opening library file sf.7601_0012.adp.40" << std::endl;
	exit(1);
    }
    delete work_dir;
  }
  if (buffer[0] == ' ') {
    strutils::strget(&buffer[1],yr,4);
    strutils::strget(&buffer[5],mo,2);
// new formats
    if (buffer[17] == '.') {
// daily
	strutils::strget(&buffer[7],dy,2);
	date_time_.set(yr,mo,dy,0);
	library_date_time=date_time_;
	location_.ID.assign(&buffer[9],5);
	strutils::strget(&buffer[9],addl_data.data.tmax,5);
	strutils::strget(&buffer[14],addl_data.data.tmin,5);
	strutils::strget(&buffer[19],addl_data.data.pcp_amt,6);
	strutils::strget(&buffer[25],cpc_dly_summ.eprcp,6);
	strutils::strget(&buffer[31],cpc_dly_summ.vp,6);
	strutils::strget(&buffer[37],cpc_dly_summ.potev,6);
	strutils::strget(&buffer[43],cpc_dly_summ.vpd,6);
	strutils::strget(&buffer[49],cpc_dly_summ.slp06,6);
	strutils::strget(&buffer[55],cpc_dly_summ.slp12,6);
	strutils::strget(&buffer[61],cpc_dly_summ.slp18,6);
	strutils::strget(&buffer[67],cpc_dly_summ.slp00,6);
	strutils::strget(&buffer[73],cpc_dly_summ.aptmx,6);
	strutils::strget(&buffer[79],cpc_dly_summ.chillm,6);
	strutils::strget(&buffer[85],cpc_dly_summ.rad,5);
	strutils::strget(&buffer[106],cpc_dly_summ.mxrh,3);
	strutils::strget(&buffer[109],cpc_dly_summ.mnrh,3);
    }
    else {
// monthly
	dy=0;
	date_time_.set(yr,mo,dy,0);
	library_date_time.set(yr,mo,1,0);
	location_.ID.assign(&buffer[7],5);
	strutils::strget(&buffer[12],cpc_mly_summ.tmean,5);
	strutils::strget(&buffer[17],cpc_mly_summ.tmax,5);
	strutils::strget(&buffer[22],cpc_mly_summ.tmin,5);
	strutils::strget(&buffer[27],cpc_mly_summ.hmax,5);
	strutils::strget(&buffer[32],cpc_mly_summ.rminl,5);
	strutils::strget(&buffer[37],cpc_mly_summ.ctmean,5);
	strutils::strget(&buffer[42],cpc_mly_summ.aptmax,5);
	strutils::strget(&buffer[47],cpc_mly_summ.wcmin,5);
	strutils::strget(&buffer[52],cpc_mly_summ.hmaxapt,5);
	strutils::strget(&buffer[57],cpc_mly_summ.wclminl,5);
	strutils::strget(&buffer[62],cpc_mly_summ.rpcp,7);
	strutils::strget(&buffer[69],cpc_mly_summ.epcp,7);
	strutils::strget(&buffer[76],cpc_mly_summ.cpcp,7);
	strutils::strget(&buffer[83],cpc_mly_summ.epcpmax,5);
	strutils::strget(&buffer[88],cpc_mly_summ.ahs,5);
	strutils::strget(&buffer[93],cpc_mly_summ.avp,5);
	strutils::strget(&buffer[98],cpc_mly_summ.apet,5);
	strutils::strget(&buffer[103],cpc_mly_summ.avpd,5);
	strutils::strget(&buffer[114],cpc_mly_summ.trad,5);
	strutils::strget(&buffer[119],cpc_mly_summ.amaxrh,5);
	strutils::strget(&buffer[124],cpc_mly_summ.aminrh,5);
	strutils::strget(&buffer[129],cpc_mly_summ.ihdd,5);
	strutils::strget(&buffer[134],cpc_mly_summ.icdd,5);
	strutils::strget(&buffer[139],cpc_mly_summ.igdd,5);
    }
    if (strutils::has_beginning(location_.ID,"99")) {
	strutils::replace_all(location_.ID,"99","");
	library_ID="K"+location_.ID+"  ";
    }
    else {
	library_ID=location_.ID+"0";
    }
    le=library.getEntryFromHistory(library_ID,library_date_time);
    location_.latitude=le.lat;
    location_.longitude=le.lon;
    location_.elevation=le.elev;
  }
  else {
// AB formats
    if (buffer[6] == ' ') {
// daily AB 3i2,1x,i5
	strutils::strget(buffer,yr,2);
	strutils::strget(&buffer[2],mo,2);
	strutils::strget(&buffer[4],dy,2);
	date_time_.set(1900+yr,mo,dy,0);
	location_.ID.assign(&buffer[7],5);
	if (location_.ID == "99999") {
	  location_.ID.assign(&buffer[13],4);
	  strutils::trim(location_.ID);
	}
	else {
	  strutils::replace_all(location_.ID," ","0");
	}
	strutils::strget(&buffer[17],location_.latitude,6);
	location_.latitude/=100.;
	strutils::strget(&buffer[23],location_.longitude,6);
	location_.longitude/=100.;
// convert west longitudes
	if (location_.longitude > 180.) {
	  location_.longitude=360.-location_.longitude;
	}
	else {
	  location_.longitude=-location_.longitude;
	}
	strutils::strget(&buffer[29],location_.elevation,5);
	if (buffer[40] == '9') {
	  addl_data.data.tmax=-99.9;
	}
	else {
	  strutils::strget(&buffer[34],addl_data.data.tmax,5);
	  addl_data.data.tmax/=10.;
	}
	if (buffer[47] == '9') {
	  addl_data.data.tmin=-99.9;
	}
	else {
	  strutils::strget(&buffer[41],addl_data.data.tmin,5);
	  addl_data.data.tmin/=10.;
	}
	if (buffer[54] == '8' || buffer[54] == '9') {
	  addl_data.data.pcp_amt=-99.9;
	}
	else {
	  strutils::strget(&buffer[48],addl_data.data.pcp_amt,5);
	  addl_data.data.pcp_amt/=10.;
	}
    }
    else if (buffer[4] == ' ') {
// monthly AB i4,1x,i5
    }
    else {
// snow AB i5,a4,i6
    }
  }
}

void CPCSummaryObservation::print(std::ostream& outs) const
{
  print_header(outs,true);
}

void CPCSummaryObservation::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(2);
  outs << "ID: " << location_.ID << "  DATE: " << date_time_.to_string("%Y-%m-%d") << "  LAT: " << std::setw(5) << location_.latitude << "  LON: " << std::setw(6) << location_.longitude << "  ELEV: " << std::setw(5) << location_.elevation << std::endl;
}
