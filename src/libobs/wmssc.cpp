#include <iomanip>
#include <surface.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>

int InputWMSSCObservationStream::ignore()
{
return -1;
}

int InputWMSSCObservationStream::peek()
{
return -1;
}

int InputWMSSCObservationStream::read(unsigned char *buffer,size_t buffer_length)
{
  int num_bytes;
  unsigned char *temp;

  if (irs != NULL) {
// binary rptout version
    num_bytes=irs->read(buffer,buffer_length);
    if (num_bytes < 0)
      return num_bytes;
    if (irs->flag() == 0) {
      temp=new unsigned char[num_bytes];
      bits::get(buffer,temp,16,8,0,num_bytes-2);
      bits::set(buffer,temp,20,8,0,num_bytes-2);
      bits::set(buffer,0x3f,16,6);
      num_bytes++;
      delete[] temp;
    }
  }
  else {
// ASCII version
num_bytes=-1;
  }

  num_read++;
  return num_bytes;
}

void WMSSCObservation::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  int dum,off;
  short yr,mo;
  char namea[30];

  bits::get(stream_buffer,wmssc.format,16,6);
  bits::get(stream_buffer,wmssc.data_source,22,2);
  bits::get(stream_buffer,wmssc.ship_indicator,24,1);
  bits::get(stream_buffer,wmssc.id_source,25,2);
  bits::get(stream_buffer,dum,27,21);
  location_.ID=strutils::ftos(dum,6,0,'0');
  bits::get(stream_buffer,yr,48,12);
  bits::get(stream_buffer,mo,60,4);
  date_time_.set(yr,mo,1,0);
  bits::get(stream_buffer,dum,64,11);
  location_.latitude=(dum-1000)/10.;
  bits::get(stream_buffer,dum,75,12);
  location_.longitude=(dum-2000)/10.;
  if (location_.longitude > -199.) {
    location_.longitude=-location_.longitude;
  }
  bits::get(stream_buffer,location_.elevation,87,14);
  location_.elevation-=1000;
  bits::get(stream_buffer,namea,101,6,0,30);
  charconversions::dpc_to_ascii(namea,namea,30);
  wmssc.name.assign(namea,30);
  bits::get(stream_buffer,wmssc.addl_data_indicator,331,1);
  if (!fill_header_only) {
    off=332;
    for (size_t n=0; n < 12; ++n) {
      bits::get(stream_buffer,monthly_means[n].report.flags.slp,off,1);
      bits::get(stream_buffer,dum,off+1,15);
      if (dum == 20000) {
        monthly_means[n].report.data.slp=-999.9;
      }
      else {
        monthly_means[n].report.data.slp=dum/10.;
      }
      bits::get(stream_buffer,monthly_means[n].slp_indicator2,off+16,2);
      bits::get(stream_buffer,dum,off+18,15);
      if (dum == 20000) {
        monthly_means[n].report.data.stnp=-999.9;
      }
      else {
        monthly_means[n].report.data.stnp=dum/10.;
      }
      bits::get(stream_buffer,monthly_means[n].z,off+33,13);
      if (monthly_means[n].z == 5000) {
        monthly_means[n].z=-9999;
      }
      bits::get(stream_buffer,dum,off+46,11);
      dum-=1000;
      if (dum == 990) {
        monthly_means[n].report.data.tdry=-99.9;
      }
      else {
        monthly_means[n].report.data.tdry=dum/10.;
      }
      bits::get(stream_buffer,dum,off+57,18);
      if (dum == 200000) {
        monthly_means[n].addl_data.data.pcp_amt=-999.9;
      }
      else if (dum == 150000) {
        monthly_means[n].addl_data.data.pcp_amt=888.8;
      }
      else {
        monthly_means[n].addl_data.data.pcp_amt=dum/10.;
      }
      off+=75;
    }
    bits::get(stream_buffer,report_.flags.slp,off,1);
    bits::get(stream_buffer,dum,off+1,15);
    report_.data.slp=(dum-20000)/10.;
    if (dum == 20000) {
      report_.data.slp=-999.9;
    }
    else {
      report_.data.slp=dum/10.;
    }
    bits::get(stream_buffer,annual_slp_indicator2,off+16,2);
    bits::get(stream_buffer,dum,off+18,15);
    report_.data.stnp=(dum-20000)/10.;
    if (dum == 20000) {
      report_.data.stnp=-999.9;
    }
    else {
      report_.data.stnp=dum/10.;
    }
    bits::get(stream_buffer,annual_z,off+33,13);
    if (annual_z == 5000) {
      annual_z=-9999;
    }
    bits::get(stream_buffer,dum,off+46,11);
    dum-=1000;
    if (dum == 990) {
      report_.data.tdry=-99.9;
    }
    else {
      report_.data.tdry=dum/10.;
    }
    bits::get(stream_buffer,dum,off+57,18);
    if (dum == 200000) {
      addl_data.data.pcp_amt=-999.9;
    }
    else if (dum == 150000) {
      addl_data.data.pcp_amt=888.8;
    }
    else {
      addl_data.data.pcp_amt=dum/10.;
    }
    off+=75;
    if (wmssc.addl_data_indicator == 1) {
      if (yr < 1961) {
      }
      else {
        for (size_t n=0; n < 12; ++n) {
          bits::get(stream_buffer,dum,off,11);
          dum-=1000;
          if (dum == 990)
            monthly_means[n].wmssc_addl_data.t_dep=-99.9;
          else
            monthly_means[n].wmssc_addl_data.t_dep=dum/10.;
          bits::get(stream_buffer,monthly_means[n].wmssc_addl_data.mois_flag,off+11,1);
          switch (monthly_means[n].wmssc_addl_data.mois_flag) {
            case 0:
              bits::get(stream_buffer,dum,off+12,11);
              if (dum == 127)
                monthly_means[n].wmssc_addl_data.mois=-99;
              else
                monthly_means[n].wmssc_addl_data.mois=dum;
              bits::get(stream_buffer,dum,off+23,11);
              dum-=100;
              if (dum == 200)
                monthly_means[n].wmssc_addl_data.mois_dep=-99;
              else
                monthly_means[n].wmssc_addl_data.mois_dep=dum;
              break;
            case 1:
              bits::get(stream_buffer,dum,off+12,11);
              if (dum == 1000)
                monthly_means[n].wmssc_addl_data.mois=-99.9;
              else
                monthly_means[n].wmssc_addl_data.mois=dum/10.;
              bits::get(stream_buffer,dum,off+23,11);
              dum-=1000;
              if (dum == 1000)
                monthly_means[n].wmssc_addl_data.mois_dep=-99.9;
              else
                monthly_means[n].wmssc_addl_data.mois_dep=dum/10.;
              break;
          }
          bits::get(stream_buffer,monthly_means[n].wmssc_addl_data.n_pcp_1mm,off+34,6);
          if (monthly_means[n].wmssc_addl_data.n_pcp_1mm == 63)
            monthly_means[n].wmssc_addl_data.n_pcp_1mm=-9;
          bits::get(stream_buffer,monthly_means[n].wmssc_addl_data.pcp_dep,off+40,12);
          monthly_means[n].wmssc_addl_data.pcp_dep-=2000;
          if (monthly_means[n].wmssc_addl_data.pcp_dep == 2000)
            monthly_means[n].wmssc_addl_data.pcp_dep=-999;
          bits::get(stream_buffer,monthly_means[n].wmssc_addl_data.quint_pcp,off+52,3);
          if (monthly_means[n].wmssc_addl_data.quint_pcp == 7)
            monthly_means[n].wmssc_addl_data.quint_pcp=9;
          bits::get(stream_buffer,monthly_means[n].wmssc_addl_data.nobs,off+55,6);
          if (monthly_means[n].wmssc_addl_data.nobs == 63)
            monthly_means[n].wmssc_addl_data.nobs=-99;
          bits::get(stream_buffer,monthly_means[n].wmssc_addl_data.hrs_sun,off+61,10);
          if (monthly_means[n].wmssc_addl_data.hrs_sun == 999)
            monthly_means[n].wmssc_addl_data.hrs_sun=-999;
          bits::get(stream_buffer,monthly_means[n].wmssc_addl_data.pct_avg_sun,off+71,10);
          if (monthly_means[n].wmssc_addl_data.pct_avg_sun == 1000)
            monthly_means[n].wmssc_addl_data.pct_avg_sun=-999;
          bits::get(stream_buffer,dum,off+81,11);
          dum-=1000;
          if (dum == 990)
            monthly_means[n].wmssc_addl_data.sst=-99.9;
          else
            monthly_means[n].wmssc_addl_data.sst=dum/10.;
          bits::get(stream_buffer,dum,off+92,11);
          dum-=1000;
          if (dum == 990)
            monthly_means[n].wmssc_addl_data.sst_dep=-99.9;
          else
            monthly_means[n].wmssc_addl_data.sst_dep=dum/10.;
          off+=103;
        }
        bits::get(stream_buffer,dum,off,11);
        dum-=1000;
        if (dum == 990)
          annual_wmssc_addl_data.t_dep=-99.9;
        else
          annual_wmssc_addl_data.t_dep=dum/10.;
        bits::get(stream_buffer,annual_wmssc_addl_data.mois_flag,off+11,1);
          switch (annual_wmssc_addl_data.mois_flag) {
          case 0:
            bits::get(stream_buffer,dum,off+12,11);
            if (dum == 127)
              annual_wmssc_addl_data.mois=-99;
            else
              annual_wmssc_addl_data.mois=dum;
            bits::get(stream_buffer,dum,off+23,11);
            dum-=100;
            if (dum == 200)
              annual_wmssc_addl_data.mois_dep=-99;
            else
              annual_wmssc_addl_data.mois_dep=dum;
            break;
          case 1:
            bits::get(stream_buffer,dum,off+12,11);
            if (dum == 1000)
              annual_wmssc_addl_data.mois=-99.9;
            else
              annual_wmssc_addl_data.mois=dum/10.;
            bits::get(stream_buffer,dum,off+23,11);
            dum-=1000;
            if (dum == 1000)
              annual_wmssc_addl_data.mois_dep=-99.9;
            else
              annual_wmssc_addl_data.mois_dep=dum/10.;
            break;
        }
        bits::get(stream_buffer,annual_wmssc_addl_data.n_pcp_1mm,off+34,6);
        if (annual_wmssc_addl_data.n_pcp_1mm == 63)
          annual_wmssc_addl_data.n_pcp_1mm=-9;
        bits::get(stream_buffer,annual_wmssc_addl_data.pcp_dep,off+40,12);
        annual_wmssc_addl_data.pcp_dep-=2000;
        if (annual_wmssc_addl_data.pcp_dep == 2000)
          annual_wmssc_addl_data.pcp_dep=-999;
        bits::get(stream_buffer,annual_wmssc_addl_data.quint_pcp,off+52,3);
        if (annual_wmssc_addl_data.quint_pcp == 7)
          annual_wmssc_addl_data.quint_pcp=9;
        bits::get(stream_buffer,annual_wmssc_addl_data.nobs,off+55,6);
        if (annual_wmssc_addl_data.nobs == 63)
          annual_wmssc_addl_data.nobs=-99;
        bits::get(stream_buffer,annual_wmssc_addl_data.hrs_sun,off+61,10);
        if (annual_wmssc_addl_data.hrs_sun == 999)
          annual_wmssc_addl_data.hrs_sun=-999;
        bits::get(stream_buffer,annual_wmssc_addl_data.pct_avg_sun,off+71,10);
        if (annual_wmssc_addl_data.pct_avg_sun == 1000)
          annual_wmssc_addl_data.pct_avg_sun=-999;
        bits::get(stream_buffer,dum,off+81,11);
        dum-=1000;
        if (dum == 990)
          annual_wmssc_addl_data.sst=-99.9;
        else
          annual_wmssc_addl_data.sst=dum/10.;
        bits::get(stream_buffer,dum,off+92,11);
        dum-=1000;
        if (dum == 990)
          annual_wmssc_addl_data.sst_dep=-99.9;
        else
          annual_wmssc_addl_data.sst_dep=dum/10.;
        off+=103;
      }
    }
  }
}

SurfaceObservation::SurfaceAdditionalData WMSSCObservation::monthly_additional_data(size_t month_number) const
{
  if (month_number < 1 || month_number > 12) {
    SurfaceObservation::SurfaceAdditionalData empty;
    return empty;
  }
  else {
    return monthly_means[month_number-1].addl_data;
  }
}

SurfaceObservation::SurfaceReport WMSSCObservation::monthly_report(size_t month_number) const
{
  if (month_number < 1 || month_number > 12) {
    SurfaceObservation::SurfaceReport empty;
    return empty;
  }
  else {
    return monthly_means[month_number-1].report;
  }
}

WMSSCObservation::WMSSCAdditionalData WMSSCObservation::monthly_WMSSC_additional_data(size_t month_number) const
{
  if (month_number < 1 || month_number > 12) {
    WMSSCObservation::WMSSCAdditionalData empty;
    return empty;
  }
  else {
    return monthly_means[month_number-1].wmssc_addl_data;
  }
}

void WMSSCObservation::print(std::ostream& outs) const
{
  print_header(outs,true);
  outs << " STANDARD PARAMETERS:" << std::endl;
  for (size_t n=0; n < 12; ++n) {
    outs << "   MONTH: " << std::setw(2) << (n+1);
    switch (static_cast<int>(monthly_means[n].slp_indicator2)) {
      case 0:
      {
        outs << "  SLP: " << std::setw(6) << monthly_means[n].report.data.slp;
        break;
      }
      case 1:
      {
        outs << "  P: " << std::setw(6) << monthly_means[n].report.data.slp << " at Z: " << std::setw(5) << monthly_means[n].z;
        break;
      }
      case 2:
      {
        outs << "  Z: " << std::setw(5) << monthly_means[n].z << " at P: " << std::setw(6) << monthly_means[n].report.data.slp;
        break;
      }
    }
    outs << "  I: " << static_cast<int>(monthly_means[n].report.flags.slp) << "/" << static_cast<int>(monthly_means[n].slp_indicator2) << "  STNP: " << std::setw(6) << monthly_means[n].report.data.stnp << "  T: " << std::setw(5) << monthly_means[n].report.data.tdry << "  PCP: " << std::setw(6) << monthly_means[n].addl_data.data.pcp_amt << std::endl;
  }
  outs << "   ANNUAL:  ";
  switch (static_cast<int>(annual_slp_indicator2)) {
    case 0:
      outs << "  SLP: " << std::setw(6) << report_.data.slp;
      break;
    case 1:
      outs << "  P: " << std::setw(6) << report_.data.slp << " at Z: " << std::setw(5) << annual_z;
      break;
    case 2:
      outs << "  Z: " << std::setw(5) << annual_z << " at P: " << std::setw(6) << report_.data.slp;
      break;
  }
  outs << "  I: " << static_cast<int>(report_.flags.slp) << "/" << static_cast<int>(annual_slp_indicator2) << "  STNP: " << std::setw(6) << report_.data.stnp << "  T: " << std::setw(5) << report_.data.tdry << "  PCP: " << std::setw(6) << addl_data.data.pcp_amt << std::endl;
  if (static_cast<int>(wmssc.addl_data_indicator) == 1) {
    outs << " ADDITIONAL DATA:" << std::endl;
    if (date_time_.year() < 1961) {
    }
    else {
      for (size_t n=0; n < 12; ++n) {
        outs << "   MONTH: " << std::setw(2) << (n+1) << "  T_DEP: " << std::setw(5) << monthly_means[n].wmssc_addl_data.t_dep << "  MOIS: " << std::setw(5) << monthly_means[n].wmssc_addl_data.mois << " I: " << static_cast<int>(monthly_means[n].wmssc_addl_data.mois_flag) << "  MOIS_DEP: " << std::setw(5) << monthly_means[n].wmssc_addl_data.mois_dep << "  #PCP>1MM: " << std::setw(2) << monthly_means[n].wmssc_addl_data.n_pcp_1mm << "  PCP_DEP: " << std::setw(4) << monthly_means[n].wmssc_addl_data.pcp_dep << "  QUINT: " << monthly_means[n].wmssc_addl_data.quint_pcp << "  #OBS: " << std::setw(2) << monthly_means[n].wmssc_addl_data.nobs << "  SUN: " << std::setw(4) << monthly_means[n].wmssc_addl_data.hrs_sun << "  %AVG: " << std::setw(3) << monthly_means[n].wmssc_addl_data.pct_avg_sun;
        if (monthly_means[n].wmssc_addl_data.sst > -99. || monthly_means[n].wmssc_addl_data.sst_dep > -99.)
          outs << "  SST: " << std::setw(5) << monthly_means[n].wmssc_addl_data.sst << "  SST_DEP: " << std::setw(5) << monthly_means[n].wmssc_addl_data.sst_dep;
        outs << std::endl;
      }
      outs << "   ANNUAL:  " << std::endl;
      outs << "  T_DEP: " << std::setw(5) << annual_wmssc_addl_data.t_dep << "  MOIS: " << std::setw(5) << annual_wmssc_addl_data.mois << " I: " << static_cast<int>(annual_wmssc_addl_data.mois_flag) << "  MOIS_DEP: " << std::setw(5) << annual_wmssc_addl_data.mois_dep << "  #PCP>1MM: " << std::setw(2) << annual_wmssc_addl_data.n_pcp_1mm << "  PCP_DEP: " << std::setw(4) << annual_wmssc_addl_data.pcp_dep << "  QUINT: " << annual_wmssc_addl_data.quint_pcp << "  #OBS: " << std::setw(2) << annual_wmssc_addl_data.nobs << "  SUN: " << std::setw(4) << annual_wmssc_addl_data.hrs_sun << "  %AVG: " << std::setw(3) << annual_wmssc_addl_data.pct_avg_sun;
      if (annual_wmssc_addl_data.sst > -99. || annual_wmssc_addl_data.sst_dep > -99.)
        outs << "  SST: " << std::setw(5) << annual_wmssc_addl_data.sst << "  SST_DEP: " << std::setw(5) << annual_wmssc_addl_data.sst_dep;
      outs << std::endl;
    }
  }
}

void WMSSCObservation::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(1);
  outs << "WMO#: " << location_.ID << "  SHIP: " << static_cast<int>(wmssc.ship_indicator) << "  SRC CODES: " << static_cast<int>(wmssc.data_source) << "/" << static_cast<int>(wmssc.id_source) << "  NAME: " << wmssc.name << "  DATE: " << date_time_.to_string("%Y-%m") << "  LAT: " << std::setw(5) << location_.latitude << "  LON: " << std::setw(6) << location_.longitude << "  ELEV: " << std::setw(5) << location_.elevation << "  ADI: " << static_cast<int>(wmssc.addl_data_indicator) << std::endl;
}
