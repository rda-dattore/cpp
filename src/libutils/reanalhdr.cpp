#include <iomanip>
#include <complex>
#include <reanalhdr.hpp>
#include <bits.hpp>

const short ReanalysisHeader::unknown_id=0,ReanalysisHeader::iwmo_id=1,
  ReanalysisHeader::iwban_id=2,ReanalysisHeader::icoop_id=3,
  ReanalysisHeader::iwmo6_id=4,ReanalysisHeader::iwmoext_id=5,
  ReanalysisHeader::iother_id=7,ReanalysisHeader::acall_id=8,
  ReanalysisHeader::awmo_id=9,ReanalysisHeader::awban_id=10,
  ReanalysisHeader::acoop_id=11,ReanalysisHeader::aname_id=12,
  ReanalysisHeader::aother_id=15;
const char *ReanalysisHeader::gateids[]={"","      ","     7","   4YB","  CGDN",
  "  DBBH","  DBFR","  DSCZ","  DZFR","  EREA","  EREB","  EREH","  EREI",
  "  ERES","  EREU","  FBEM","  FIPD","  FNBG","  FNOY","  GPBA","  GTUC",
  "  GUDU","  MVAN","  NPCR","  PAYM","  PWQO","  PXSA","  UBLF","  UERE",
  "  UHGZ","  UHQS","  UHSQ","  UMFW","  UPUI","  UQIH","  UZGH","  WEWP",
  "  WTEP","  WTER","  XCYT"," KC135"," NC130"," NOAA2"," NOAA3"," WC135",
  " 99999"};

void ReanalysisHeader::fill(const unsigned char *buffer)
{
  int ival;
  short yr,mo,dy,time;

  bits::get(buffer,fill_bits,12,4);
  bits::get(buffer,h_type,16,4);
  switch (h_type) {
    case 0:
	length=3;
	break;
    case 1:
    case 8:
    case 9:
	length=8;
	break;
    case 2:
	length=12;
	break;
    case 3:
	length=18;
	break;
    case 4:
    case 11:
    case 12:
	length=24;
	break;
    case 10:
	length=16;
	break;
    default:
	std::cerr << "Error: unknown header type " << h_type << std::endl;
	exit(1);
  }
  bits::get(buffer,num_bytes,20,4);
  length=length+num_bytes;
  bits::get(buffer,fmt,24,10);
  bits::get(buffer,d_type,34,8);
  bits::get(buffer,src,42,9);
  bits::get(buffer,summ,51,6);
  bits::get(buffer,dt_type,57,4);
  bits::get(buffer,yr,61,9);
  yr+=1700;
  bits::get(buffer,mo,70,4);
  bits::get(buffer,dy,74,5);
  bits::get(buffer,time,79,12);
  time=(static_cast<int>(time/100.))*100.+lroundf((time%100)*0.6);
  date_time.set(yr,mo,dy,time*100);
  bits::get(buffer,l_type,91,3);
  bits::get(buffer,ival,94,16);
  lon=(ival-18001)/100.;
  bits::get(buffer,ival,110,15);
  lat=(ival-9001)/100.;
  bits::get(buffer,elev,125,15);
  elev-=1001;
  bits::get(buffer,p_type,140,4);
  if (p_type < 8) {
    bits::get(buffer,stn,144,48);
  }
  else {
    char c[6];
    bits::get(buffer,c,144,8,0,6);
    cid.assign(c,6);
  }
}

void ReanalysisHeader::set(const DATSAVAircraftReport& source)
{
  fill_bits=0;
  h_type=4;
  num_bytes=0;
  fmt=12;
  d_type=4;
  src=5;
  summ=0;
  dt_type=1;
  date_time=source.date_time();
  l_type=2;
  setLocation(source.location().latitude,-source.location().longitude,source.location().altitude);
  p_type=12;
  cid=source.location().ID;
}

void ReanalysisHeader::set(const NavySpotAircraftReport& source)
{
  fill_bits=0;
  h_type=4;
  num_bytes=0;
  fmt=24;
  d_type=4;
  src=38;
  summ=0;
  dt_type=1;
  date_time=source.date_time();
  l_type=2;
  setLocation(source.location().latitude,source.location().longitude,lroundf(source.location().altitude*0.3048));
  p_type=12;
  cid=source.location().ID;
}

void ReanalysisHeader::set(const NewZealandAircraftReport& source)
{
  fill_bits=0;
  h_type=4;
  num_bytes=0;
  fmt=5;
  d_type=4;
  src=9;
  summ=0;
  dt_type=1;
  date_time=source.date_time();
  l_type=2;
  setLocation(source.location().latitude,source.location().longitude,0);
  p_type=12;
  cid=source.location().ID;
}

void ReanalysisHeader::set(const Tsraob& source)
{
  static size_t s_code[]={  0,101,102,103,104,105,106,107,108,109,110, 39,111,
   13,  0, 25,112,121,122,123,113, 40,124,125,126,114,115,116,  0, 47, 48, 35,
   49, 50, 51, 52, 53, 32, 34, 55, 56, 41, 42, 43, 44, 45, 57,  5,  0,  0,  0,
    0,  0,  0, 54, 38, 38,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0};
  static long long ruship_ids[]={0x2020202020200000LL,0x2020202020200000LL,
    0x202020c302f30000LL,0x202001001e100000LL,0x20200100a6d70000LL,
    0x2020182d5cf10000LL,0x2020a7260f1d0000LL,0x2020a7260f1b0000LL,
    0x2020a726081d0000LL,0x2020a726191d0000LL,0x2020a7268f1d0000LL,
    0x2020a726841d0000LL,0x20202d3b08a30000LL,0x20202d3b1ea30000LL,
    0x2020b82da6500000LL,0x202068008bf90000LL,0x2020d1005c0b0000LL,
    0x2020e337e3200000LL,0x202041444a520000LL,0x2020414d41590000LL,
    0x2020433720200000LL,0x2020433743200000LL,0x20204543e7e70000LL,
    0x2020455045e20000LL,0x2020455045e30000LL,0x2020455045410000LL,
    0x2020455045430000LL,0x2020455045e90000LL,0x2020455045540000LL,
    0x2020455045580000LL,0x2020455045f50000LL,0x2020455245410000LL,
    0x2020455245420000LL,0x2020455245430000LL,0x2020455245480000LL,
    0x2020455245490000LL,0x20204552454a0000LL,0x2020455245530000LL,
    0x2020455245540000LL,0x2020455245550000LL,0x2020455245560000LL,
    0x2020455235480000LL,0x20207d0000000000LL,0x2020f04f50f90000LL,
    0x20205352544d0000LL,0x2020535341580000LL,0x2020553e19aa0000LL,
    0x2020554141580000LL,0x202055424c460000LL,0x202055424e5a0000LL,
    0x202055434e520000LL,0x20205544544a0000LL,0x2020554549560000LL,
    0x2020554851530000LL,0x2020554a464f0000LL,0x2020554a4c590000LL,
    0x2020554a59470000LL,0x2020554d41590000LL,0x2020554d46570000LL,
    0x2020554e41430000LL,0x2020554e47560000LL,0x202055504a410000LL,
    0x2020555055490000LL,0x20205550554a0000LL,0x202055514a480000LL,
    0x2020555347450000LL,0x2020555550420000LL,0x202055564d4a0000LL,
    0x2020555745430000LL,0x2020555a47480000LL,0x2020590000000000LL,
    0x2020f54141f80000LL,0x2020f545414b0000LL,0x2020f54de6420000LL,
    0x2020f54d41f90000LL,0x2020f558f1430000LL,0x2020f5fae7580000LL,
    0x2020554857410000LL,0x20204c4544450000LL,0x2020202020200000LL,
    0x2020802d5c4a0000LL,0x20205a375a200000LL,0x2020435255500000LL,
    0x2020554f59540000LL,0x20204144574c0000LL,0x2020555149480000LL,
    0x2020554251500000LL,0x202020554d410000LL,0x2020554158580000LL,
    0x2020455347470000LL,0x2020204552450000LL,0x2020464142450000LL};
  size_t n;

  fill_bits=0;
  h_type=4;
  num_bytes=0;
  fmt=3;
  src=s_code[source.flags().source];
// if "Other NCDC data", check the platform type
//   wmo is TD90
//   wban is US Control from NCDC
  if (src == 25 && p_type == ReanalysisHeader::iwban_id)
    src=28;
  if (src == 0)
    std::cerr << "Warning: no source code for TSR source code " << source.flags().source << std::endl;
  if (source.flags().source == 39 && source.location().ID > "120000")
    src=30;
  switch (source.flags().format) {
    case 1:
    case 3:
    case 4:
    case 9:
    case 12:
	switch (source.flags().source) {
	  case 46:
	    d_type=2;
	    break;
	  case 47:
	    if (source.location().ID <= "010000" || (source.location().ID > "990000" && source.location().ID < "990240"))
		d_type=8;
	    else
		d_type=2;
	    break;
	  default:
	    if (source.location().ID <= "001000" || (source.location().ID > "099000" && source.location().ID < "099024"))
		d_type=8;
	    else
		d_type=2;
	}
	break;
    case 2:
    case 5:
    case 10:
    case 13:
    case 22:
	switch (source.flags().source) {
	  case 47:
	    if (source.location().ID <= "010000" || (source.location().ID > "990000" && source.location().ID < "990240"))
		d_type=9;
	    else
		d_type=3;
	    break;
	  default:
	    if (source.location().ID <= "001000" || (source.location().ID > "099000" && source.location().ID < "099024"))
		d_type=9;
	    else
		d_type=3;
	}
	break;
    case 6:
	d_type=21;
	break;
    default:
	std::cerr << "Error: unable to set data type for tsr format " <<
        source.flags().format << std::endl;
	exit(1);
  }
  if (source.flags().source < 1 || source.flags().source > 60) {
    std::cerr << "Error: unable to set source code for tsr source " <<
      source.flags().source << std::endl;
    exit(1);
  }
// GATE raobs
  else if (source.flags().source == 31) {
    if (source.location().ID <= "000045") {
	cid=gateids[std::stoi(source.location().ID)];
	if (source.location().ID == "000040" || source.location().ID == "000041" || source.location().ID == "000044") {
	  d_type=5;
	}
    }
  }
  else if (source.flags().source == 39 && source.location().ID >= "120000" && source.location().ID < "130000") {
    n=(std::stoi(source.location().ID)-120000) % 1000;
    if (n <= 91) {
	cid=reinterpret_cast<char *>(&ruship_ids[n]);
    }
    else {
	cid="      ";
    }
  }
  summ=0;
  switch (source.flags().source) {
    case 46:
	dt_type=1;
	break;
    default:
	if ( (source.date_time().time() % 100) != 0)
	  dt_type=1;
	else
	  dt_type=2;
  }
  date_time=source.date_time();
// set location type externally
// set location externally
// set platform id type externally
  stn=std::stoll(source.location().ID);
  if (source.flags().source == 47) {
    stn/=10;
  }
}

void ReanalysisHeader::setLocation(float latitude,float longitude,int elevation,
  short location_type)
{
  if (location_type > 0) {
    l_type=location_type;
  }
  lat=latitude;
  lon=longitude;
  elev=elevation;
}

void ReanalysisHeader::copyToBuffer(unsigned char *output_buffer,const size_t buffer_length) const
{
  int ival;
  short yr=date_time.year()-1700,mo=date_time.month(),dy=date_time.day(),time;

  bits::set(output_buffer,fill_bits,12,4);
  bits::set(output_buffer,h_type,16,4);
  bits::set(output_buffer,num_bytes,20,4);
  bits::set(output_buffer,fmt,24,10);
  bits::set(output_buffer,d_type,34,8);
  bits::set(output_buffer,src,42,9);
  bits::set(output_buffer,summ,51,6);
  bits::set(output_buffer,dt_type,57,4);
  bits::set(output_buffer,yr,61,9);
  bits::set(output_buffer,mo,70,4);
  bits::set(output_buffer,dy,74,5);
  time=(static_cast<int>(date_time.time()/10000.))*100+lroundf((date_time.time()%10000)/0.6);
  bits::set(output_buffer,time,79,12);
  bits::set(output_buffer,l_type,91,3);
  ival=lroundf(lon*100.)+18001;
  bits::set(output_buffer,ival,94,16);
  ival=lroundf(lat*100.)+9001;
  bits::set(output_buffer,ival,110,15);
  if (elev != -999 && elev != 99999) {
    bits::set(output_buffer,elev+1001,125,15);
  }
  else {
    bits::set(output_buffer,0,125,15);
  }
  bits::set(output_buffer,p_type,140,4);
  if (p_type < 8) {
    bits::set(output_buffer,stn,144,48);
  }
  else {
    bits::set(output_buffer,cid.c_str(),144,8,0,6);
  }
}

std::ostream& operator<<(std::ostream& out_stream,const ReanalysisHeader& source)
{
  out_stream.setf(std::ios::fixed);
  out_stream.precision(2);

  out_stream << "  FLAGS - Header Type: " << std::setw(2) << source.h_type << "  Format: " << std::setw(2) << source.fmt << "  Data Type: " << std::setw(2) << source.d_type << "  Source: " << std::setw(2) << source.src << "  Date/Time Type: " << std::setw(2) << source.dt_type << "  Location Type: " << std::setw(2) << source.l_type << "  ID Type: " << std::setw(2) << source.p_type << std::endl;
  if (source.p_type < 8) {
    if (source.stn < 100000) {
	out_stream << "  Station:  " << std::setw(5) << std::setfill('0') << source.stn << std::setfill(' ');
    }
    else {
	out_stream << "  Station: " << std::setw(6) << source.stn;
    }
  }
  else {
    out_stream << "  Station: " << source.cid;
  }
  out_stream << "  Date: " << source.date_time.to_string() << "Z  Location: " << std::setw(6) << source.lat << " " << std::setw(7) << source.lon << " " << std::setw(5) << source.elev;
  return out_stream;
}
