// FILE: isd.cpp

#include <iomanip>
#include <surface.hpp>
#include <strutils.hpp>
#include <myerror.hpp>

int InputISDObservationStream::ignore()
{
return -1;
}

int InputISDObservationStream::peek()
{
return -1;
}

int InputISDObservationStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read;
  char *buf=reinterpret_cast<char *>(const_cast<unsigned char *>(buffer));

  if (fs.is_open()) {
    curr_offset=fs.tellg();
    fs.getline(buf,buffer_length);
    if (fs.eof()) {
	return bytes_read=bfstream::eof;
    }
    else if (fs.fail()) {
	return bytes_read=bfstream::error;
    }
    else {
	bytes_read=fs.gcount()-1;
    }
  }
  else {
    bytes_read=bfstream::error;
  }
  ++num_read;
  return bytes_read;
}

void decodeASOSAWOSPresentWeather(char *buf,std::vector<std::string>& vals)
{
  vals.clear();
  auto test=std::string(&buf[2],2);
  if (test != "99") {
    vals.emplace_back(buf,1);
    vals.emplace_back(&buf[1],1);
    vals.emplace_back(test);
    vals.emplace_back(&buf[4],1);
    vals.emplace_back(&buf[5],1);
    vals.emplace_back(&buf[6],1);
    vals.emplace_back(&buf[7],1);
  }
}

void decodeAtmosphericPressureChange(char *buf,std::vector<std::string>& vals)
{
  vals.clear();
  auto test=std::string(&buf[2],3);
  if (test != "999") {
    vals.emplace_back(buf,1);
    vals.emplace_back(&buf[1],1);
    vals.emplace_back(test);
    vals.emplace_back(&buf[5],1);
    vals.emplace_back(&buf[6],4);
    vals.emplace_back(&buf[10],1);
  }
}

void decodeAutomatedPresentWeather(char *buf,std::vector<std::string>& vals)
{
  vals.clear();
  vals.emplace_back(buf,2);
  vals.emplace_back(&buf[2],1);
}

void decodeExtremeAirTemperature(char *buf,std::vector<std::string>& vals)
{
  vals.clear();
  auto test=std::string(&buf[4],5);
  if (test != "999") {
    vals.emplace_back(buf,3);
    vals.emplace_back(&buf[3],1);
    vals.emplace_back(test);
    vals.emplace_back(&buf[9],1);
  }
}

void decodeGroundSurfaceObservation(char *buf,std::vector<std::string>& vals)
{
  vals.clear();
  auto test=std::string(buf,2);
  if (test != "99") {
    vals.emplace_back(test);
    vals.emplace_back(&buf[2],1);
  }
}

void decodeGroundSurfaceMinimumTemperature(char *buf,std::vector<std::string>& vals)
{
  vals.clear();
  auto test=std::string(&buf[3],5);
  if (test != "999") {
    vals.emplace_back(buf,3);
    vals.emplace_back(test);
    vals.emplace_back(&buf[8],1);
  }
}

void decodeHourlyLiquidPrecipitation(char *buf,std::vector<std::string>& vals)
{
  vals.clear();
  auto test=std::string(&buf[2],4);
  if (test != "9999") {
    vals.emplace_back(buf,2);
    vals.emplace_back(test);
    vals.emplace_back(&buf[6],1);
    vals.emplace_back(&buf[7],1);
  }
}

void decodePastWeather(char *buf,std::vector<std::string>& vals)
{
  vals.clear();
  vals.emplace_back(buf,1);
  vals.emplace_back(&buf[1],1);
  vals.emplace_back(&buf[2],2);
  vals.emplace_back(&buf[4],1);
}

void decodePresentWeather(char *buf,std::vector<std::string>& vals)
{
  vals.clear();
  vals.emplace_back(buf,2);
  vals.emplace_back(&buf[2],1);
}

void decodeSkyCondition2(char *buf,std::vector<std::string>& vals)
{
  vals.clear();
  auto test=std::string(buf,2);
  if (test != "99") {
    vals.emplace_back(test);
    vals.emplace_back(&buf[2],2);
    vals.emplace_back(&buf[4],1);
    vals.emplace_back(&buf[5],2);
    vals.emplace_back(&buf[7],1);
    vals.emplace_back(&buf[8],2);
    vals.emplace_back(&buf[10],1);
    vals.emplace_back(&buf[11],5);
    vals.emplace_back(&buf[16],1);
    vals.emplace_back(&buf[17],2);
    vals.emplace_back(&buf[19],1);
    vals.emplace_back(&buf[20],2);
    vals.emplace_back(&buf[22],1);
  }
}

void decodeSnowAccumulation(char *buf,std::vector<std::string>& vals)
{
  vals.clear();
  auto test=std::string(&buf[2],3);
  if (test != "999") {
    vals.emplace_back(buf,2);
    vals.emplace_back(test);
    vals.emplace_back(&buf[5],1);
    vals.emplace_back(&buf[6],1);
  }
}

void decodeSnowDepth(char *buf,std::vector<std::string>& vals)
{
  vals.clear();
  auto test=std::string(buf,4);
  if (test != "9999") {
    vals.emplace_back(test);
    vals.emplace_back(&buf[4],1);
    vals.emplace_back(&buf[5],1);
    vals.emplace_back(&buf[6],6);
    vals.emplace_back(&buf[12],1);
    vals.emplace_back(&buf[13],1);
  }
}

ISDObservation::ISDObservation() : report_type_(),qc_proc(),ceiling_determination_code_(),windobs_type_code(),addl_data_codes(),addl_coded_table()
{
  AdditionalCodedData acd;

  acd.key="AA1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=8;
  acd.data->addl_data_handler=decodeHourlyLiquidPrecipitation;
  addl_coded_table.insert(acd);
  acd.key="AA2";
  addl_coded_table.insert(acd);
  acd.key="AA3";
  addl_coded_table.insert(acd);
  acd.key="AA4";
  addl_coded_table.insert(acd);
  acd.key="AB1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=7;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="AC1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=3;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="AD1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=19;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="AE1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=12;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="AG1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=4;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="AH1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=15;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="AH2";
  addl_coded_table.insert(acd);
  acd.key="AH3";
  addl_coded_table.insert(acd);
  acd.key="AH4";
  addl_coded_table.insert(acd);
  acd.key="AH5";
  addl_coded_table.insert(acd);
  acd.key="AH6";
  addl_coded_table.insert(acd);
  acd.key="AI1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=15;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="AI2";
  addl_coded_table.insert(acd);
  acd.key="AI3";
  addl_coded_table.insert(acd);
  acd.key="AI4";
  addl_coded_table.insert(acd);
  acd.key="AI5";
  addl_coded_table.insert(acd);
  acd.key="AI6";
  addl_coded_table.insert(acd);
  acd.key="AJ1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=14;
  acd.data->addl_data_handler=decodeSnowDepth;
  addl_coded_table.insert(acd);
  acd.key="AK1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=12;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="AL1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=7;
  acd.data->addl_data_handler=decodeSnowAccumulation;
  addl_coded_table.insert(acd);
  acd.key="AL2";
  addl_coded_table.insert(acd);
  acd.key="AL3";
  addl_coded_table.insert(acd);
  acd.key="AL4";
  addl_coded_table.insert(acd);
  acd.key="AM1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=18;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="AN1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=9;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="AO1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=8;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="AO2";
  addl_coded_table.insert(acd);
  acd.key="AO3";
  addl_coded_table.insert(acd);
  acd.key="AO4";
  addl_coded_table.insert(acd);
  acd.key="AP1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=6;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="AP2";
  addl_coded_table.insert(acd);
  acd.key="AP3";
  addl_coded_table.insert(acd);
  acd.key="AP4";
  addl_coded_table.insert(acd);
  acd.key="AU1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=8;
  acd.data->addl_data_handler=decodeASOSAWOSPresentWeather;
  addl_coded_table.insert(acd);
  acd.key="AU2";
  addl_coded_table.insert(acd);
  acd.key="AU3";
  addl_coded_table.insert(acd);
  acd.key="AU4";
  addl_coded_table.insert(acd);
  acd.key="AU5";
  addl_coded_table.insert(acd);
  acd.key="AU6";
  addl_coded_table.insert(acd);
  acd.key="AU7";
  addl_coded_table.insert(acd);
  acd.key="AU8";
  addl_coded_table.insert(acd);
  acd.key="AU9";
  addl_coded_table.insert(acd);
  acd.key="AW1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=3;
  acd.data->addl_data_handler=decodeAutomatedPresentWeather;
  addl_coded_table.insert(acd);
  acd.key="AW2";
  addl_coded_table.insert(acd);
  acd.key="AW3";
  addl_coded_table.insert(acd);
  acd.key="AW4";
  addl_coded_table.insert(acd);
  acd.key="AW5";
  addl_coded_table.insert(acd);
  acd.key="AX1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=6;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="AX2";
  addl_coded_table.insert(acd);
  acd.key="AX3";
  addl_coded_table.insert(acd);
  acd.key="AX4";
  addl_coded_table.insert(acd);
  acd.key="AX5";
  addl_coded_table.insert(acd);
  acd.key="AX6";
  addl_coded_table.insert(acd);
  acd.key="AY1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=5;
  acd.data->addl_data_handler=decodePastWeather;
  addl_coded_table.insert(acd);
  acd.key="AY2";
  addl_coded_table.insert(acd);
  acd.key="AZ1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=5;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="AZ2";
  addl_coded_table.insert(acd);
  acd.key="ED1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=8;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="GA1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=13;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="GA2";
  addl_coded_table.insert(acd);
  acd.key="GA3";
  addl_coded_table.insert(acd);
  acd.key="GA4";
  addl_coded_table.insert(acd);
  acd.key="GA5";
  addl_coded_table.insert(acd);
  acd.key="GA6";
  addl_coded_table.insert(acd);
  acd.key="GD1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=12;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="GD2";
  addl_coded_table.insert(acd);
  acd.key="GD3";
  addl_coded_table.insert(acd);
  acd.key="GD4";
  addl_coded_table.insert(acd);
  acd.key="GD5";
  addl_coded_table.insert(acd);
  acd.key="GD6";
  addl_coded_table.insert(acd);
  acd.key="GF1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=23;
  acd.data->addl_data_handler=decodeSkyCondition2;
  addl_coded_table.insert(acd);
  acd.key="GG1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=15;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="GG2";
  addl_coded_table.insert(acd);
  acd.key="GG3";
  addl_coded_table.insert(acd);
  acd.key="GG4";
  addl_coded_table.insert(acd);
  acd.key="GG5";
  addl_coded_table.insert(acd);
  acd.key="GG6";
  addl_coded_table.insert(acd);
  acd.key="GH1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=28;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="GJ1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=5;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="GK1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=4;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="GL1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=6;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="GM1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=30;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="GN1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=28;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="GO1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=19;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="GP1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=31;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="GQ1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=14;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="GR1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=14;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="HL1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=4;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="IA1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=3;
  acd.data->addl_data_handler=decodeGroundSurfaceObservation;
  addl_coded_table.insert(acd);
  acd.key="IA2";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=9;
  acd.data->addl_data_handler=decodeGroundSurfaceMinimumTemperature;
  addl_coded_table.insert(acd);
  acd.key="IC1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=25;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="KA1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=10;
  acd.data->addl_data_handler=decodeExtremeAirTemperature;
  addl_coded_table.insert(acd);
  acd.key="KA2";
  addl_coded_table.insert(acd);
  acd.key="MA1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=12;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="MD1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=11;
  acd.data->addl_data_handler=decodeAtmosphericPressureChange;
  addl_coded_table.insert(acd);
  acd.key="ME1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=6;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="MG1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=12;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="MV1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=3;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="MV2";
  addl_coded_table.insert(acd);
  acd.key="MV3";
  addl_coded_table.insert(acd);
  acd.key="MV4";
  addl_coded_table.insert(acd);
  acd.key="MV5";
  addl_coded_table.insert(acd);
  acd.key="MV6";
  addl_coded_table.insert(acd);
  acd.key="MV7";
  addl_coded_table.insert(acd);
  acd.key="MV8";
  addl_coded_table.insert(acd);
  acd.key="MW1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=3;
  acd.data->addl_data_handler=decodePresentWeather;
  addl_coded_table.insert(acd);
  acd.key="MW2";
  addl_coded_table.insert(acd);
  acd.key="MW3";
  addl_coded_table.insert(acd);
  acd.key="MW4";
  addl_coded_table.insert(acd);
  acd.key="MW5";
  addl_coded_table.insert(acd);
  acd.key="MW6";
  addl_coded_table.insert(acd);
  acd.key="MW7";
  addl_coded_table.insert(acd);
  acd.key="MW8";
  addl_coded_table.insert(acd);
  acd.key="OA1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=8;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="OA2";
  addl_coded_table.insert(acd);
  acd.key="OA3";
  addl_coded_table.insert(acd);
  acd.key="OC1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=5;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="OE1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=16;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="OE2";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=16;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="OE3";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=16;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="SA1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=5;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="ST1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=17;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="UA1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=10;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="UG1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=9;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="UG2";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=9;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="WA1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=6;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="WD1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=20;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
  acd.key="WG1";
  acd.data.reset(new AdditionalCodedData::Data);
  acd.data->len=11;
  acd.data->addl_data_handler=nullptr;
  addl_coded_table.insert(acd);
}

void ISDObservation::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  char *buf=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  int len,dum,off;
  short yr,mo,dy,time,n;
  std::string var_ident;
  AdditionalCodedData acd;

//  for (n=0; n < 4; isd_data.eprecip[n++].period=99);
  strutils::strget(buf,len,4);
  len+=105;
  location_.ID=std::string(&buf[4],6)+"-"+std::string(&buf[10],5)+"-"+std::string(&buf[51],5);
  strutils::strget(&buf[15],yr,4);
  strutils::strget(&buf[19],mo,2);
  strutils::strget(&buf[21],dy,2);
  strutils::strget(&buf[23],time,4);
  date_time_.set(yr,mo,dy,time*100);
  strutils::strget(&buf[28],dum,6);
  location_.latitude=dum/1000.;
  strutils::strget(&buf[34],dum,7);
  location_.longitude=dum/1000.;
  report_type_=std::string(&buf[41],5);
  strutils::strget(&buf[46],location_.elevation,5);
  qc_proc=std::string(&buf[56],4);
// mandatory section
  strutils::strget(&buf[60],dum,3);
  report_.data.dd=dum;
  report_.flags.dd=buf[63];
  windobs_type_code=buf[64];
  strutils::strget(&buf[65],dum,4);
  report_.data.ff=dum/10.;
  report_.flags.ff=buf[69];
  strutils::strget(&buf[70],dum,5);
  addl_data.data.cloud_base_hgt=dum;
  ceiling_determination_code_=buf[76];
  strutils::strget(&buf[78],dum,6);
  addl_data.data.vis=dum;
  strutils::strget(&buf[87],dum,5);
  report_.data.tdry=dum/10.;
  report_.flags.tdry=buf[92];
  strutils::strget(&buf[93],dum,5);
  report_.data.tdew=dum/10.;
  report_.flags.tdew=buf[98];
  strutils::strget(&buf[99],dum,5);
  report_.data.slp=dum/10.;
  report_.flags.slp=buf[104];
  addl_data_codes.clear();
  if (len > 105) {
    off=105;
    var_ident.assign(&buf[off],3);
    off+=3;
    while (off < len) {
	if (var_ident == "ADD") {
	  while (off < len) {
	    var_ident.assign(&buf[off],3);
	    off+=3;
	    if (var_ident == "REM" || var_ident == "EQD" || var_ident == "QNN") {
		break;
	    }
	    addl_data_codes.push_back(var_ident);
	    if (addl_coded_table.found(var_ident,acd)) {
		if (acd.data->addl_data_handler != nullptr) {
		  acd.data->addl_data_handler(&buf[off],acd.data->values);
		}
		acd.data->off=off;
		off+=acd.data->len;
	    }
	    else {
		buf[60]='\0';
		myerror="Error: additional data identifier '"+var_ident+"' not recognized: "+buf;
		exit(1);
	    }
	  }
	}
	else if (var_ident == "REM") {
	  var_ident.assign(&buf[off],3);
	  off+=3;
	  while (off < len && var_ident != "ADD" && var_ident != "EQD" && var_ident != "QNN") {
	    strutils::strget(&buf[off],n,3);
	    off+=3;
	    off+=n;
	    if (off < len) {
		var_ident.assign(&buf[off],3);
		off+=3;
	    }
	  }
	}
	else if (var_ident == "EQD") {
	  while (off < len) {
	    var_ident.assign(&buf[off],3);
	    off+=3;
	    if (var_ident == "ADD" || var_ident == "REM" || var_ident == "QNN") {
		break;
	    }
	  }
	}
	else if (var_ident == "QNN") {
off=len;
	}
    }
  }
}

void ISDObservation::print(std::ostream& outs) const
{
  print_header(outs,true);
}

void ISDObservation::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(2);
  std::list<std::string> *codes;

  if (verbose)
    outs << "ID: " << location_.ID << "  Date: " << date_time_.to_string() << "Z  Location: " << std::setw(6) << location_.latitude << std::setw(8) << location_.longitude << std::setw(6) << location_.elevation << "  QC: " << qc_proc << std::endl;
  else {
  }
  outs << "  MANDATORY DATA:" << std::endl;
  outs.precision(1);
  outs << "    Air Temp: " << std::setw(6) << report_.data.tdry << " " << static_cast<char>(report_.flags.tdry) << "  Dewpt: " << std::setw(6) << report_.data.tdew << " " << static_cast<char>(report_.flags.tdew) << "  DD: " << std::setw(3) << static_cast<int>(report_.data.dd) << " " << static_cast<char>(report_.flags.dd) << "  FF: " << std::setw(5) << report_.data.ff << " " << static_cast<char>(report_.flags.ff) << "  SLP: " << std::setw(6) << report_.data.slp << " " << static_cast<char>(report_.flags.slp) << "  Ceil Hgt: " << std::setw(5) << static_cast<int>(addl_data.data.cloud_base_hgt) << " " << ceiling_determination_code_ << "  Vsby: " << std::setw(6) << static_cast<int>(addl_data.data.vis) << std::endl;
  if (addl_data_codes.size() > 0) {
    outs << "  ADDITIONAL DATA:" << std::endl;
    codes=static_cast<std::list<std::string> *>(const_cast<std::list<std::string> *>(&addl_data_codes));
    for (const auto& code : *codes) {
/*
	if (strutils::has_beginning(code,"AA")) {
	  auto idx=static_cast<int>(code[2])-49;
	  outs << "    " << std::setw(2) << isd_data.eprecip[idx].period << "H Precip: " << std::setw(4) << isd_data.eprecip[idx].amount << "mm  Flags: " << isd_data.eprecip[idx].condition_code << "/" << isd_data.eprecip[idx].quality_code << std::endl;
	}
	else if (code == "AJ1") {
	  outs << "    Snow Depth: " << std::setw(4) << static_cast<int>(addl_data.data.snowd) << "  SWE: " << std::setw(7) << isd_data.snow_water_eq << std::endl;
	}
	else if (strutils::has_beginning(code,"AL")) {
	  auto idx=static_cast<int>(code[2])-49;
	  outs << "    " << std::setw(2) << isd_data.esnow[idx].period << "H Snow Accum: " << std::setw(3) << isd_data.esnow[idx].depth << "cm  Flags: " << isd_data.esnow[idx].condition_code << "/" << isd_data.esnow[idx].quality_code << std::endl;
	}
*/
    }
    outs << std::endl;
  }
}

ISDObservation::AdditionalCodedData& ISDObservation::additional_coded_data(std::string addl_data_code) const
{
  static ISDObservation::AdditionalCodedData coded,empty;
  auto *t=const_cast<my::map<AdditionalCodedData> *>(&addl_coded_table);
  if (t->found(addl_data_code,coded)) {
    return coded;
  }
  else {
    return empty;
  }
}
