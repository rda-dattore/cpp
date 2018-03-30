#include <iostream>
#include <complex>
#include <utils.hpp>

namespace metcalcs {

double compute_relative_humidity(double temperature,double dew_point_temperature)
{
  double vapor_pressure,saturation_vapor_pressure;

  if (dew_point_temperature > temperature) {
    std::cerr << "**compute_relative_humidity(): error -- dew point temperature " << dew_point_temperature << " greater than temperature " << temperature << std::endl;
    vapor_pressure=-.999;
    saturation_vapor_pressure=1.;
  }
  else {
    auto a=7.5;
    auto b=237.7;
    auto c=6.11;
    auto x=a*dew_point_temperature/(b+dew_point_temperature);
    vapor_pressure=c*pow(10.,x);
    x=a*temperature/(b+temperature);
    saturation_vapor_pressure=c*pow(10.,x);
  }
  return (vapor_pressure/saturation_vapor_pressure*100.);
}

double compute_dew_point_temperature(double dry_bulb_temperature,double wet_bulb_temperature,double station_pressure)
{
  double mixing_ratio=1.,last_mixing_ratio=0.;

  dry_bulb_temperature=(dry_bulb_temperature-32.)*5./9.;
  wet_bulb_temperature=(wet_bulb_temperature-32.)*5./9.;
  station_pressure*=33.8637526;
  auto saturation_vapor_pressure=6.112*pow(2.7182818,17.67*wet_bulb_temperature/(243.5+wet_bulb_temperature));
  auto saturation_mixing_ratio=0.62197*saturation_vapor_pressure/(station_pressure-saturation_vapor_pressure);
  while (fabs(mixing_ratio-last_mixing_ratio) > 0.001) {
    last_mixing_ratio=mixing_ratio;
    mixing_ratio=saturation_mixing_ratio+((1006.3+((last_mixing_ratio+saturation_mixing_ratio)/2.)*1850.)/2462880.)*(wet_bulb_temperature-dry_bulb_temperature);
  }
  auto vapor_pressure=mixing_ratio*station_pressure/(0.62197+mixing_ratio);
  auto X=log(vapor_pressure/6.112);
  return 243.5*X/(17.67-X)*9./5.+32.;
}

double compute_standard_height_from_pressure(double pressure)
{
/*
** this routine computes height from input pressure based on U.S.
**   standard atmosphere, 1976
*/
  double hbase[10]={0.,1.1e4,2.0e4,3.2e4,4.7e4,5.1e4,7.1e4,84852.0,84852.0,84852.0};
  double grad[10]={-0.0065,0.,0.001,0.0028,0.,-0.0028,-0.002,0.,0.,0.};
  double abs[10]={288.15,216.65,216.65,228.65,270.65,270.65,214.65,186.95,0.,0.};
  double pb[10]={1013.25,0.22632064e3,0.54748887e2,0.86801870e1,0.11090631e1,0.66938874,0.39564205e-1,0.37338360e-2,0.,0.};

  if (pressure > pb[0]) {
    return -99.;
  }
  auto n=0;
  while (n < 8 && pressure < pb[n]) {
    ++n;
  }
  if (n == 8) {
    return -99.;
  }
  --n;
  auto g=980.665;
  auto r=83143200./28.9644;
//  isothermal layer
  if (floatutils::myequalf(grad[n],0.)) {
    return (hbase[n]+.01*r*abs[n]/g*(log(pb[n]/pressure)));
  }
//  temp gradient not zero
  else {
    return (abs[n]/(-grad[n])*(1.-pow((pressure/pb[n]),(-grad[n]*r/(100.*g))))+hbase[n]);
  }
}

double compute_height_from_pressure_and_temperature(double pressure,double temperature)
{
  return (log(pressure/1013.15)*8.31432*(temperature+273.15))/-0.28404373326;
}

void compute_wind_direction_and_speed_from_u_and_v(double u,double v,int& wind_direction,double& wind_speed)
{
  if (u <= -999. || v <= -999.) {
    wind_direction=999;
    wind_speed=-999.;
  }
  else {
    auto hypot=sqrt(u*u+v*v);
    wind_speed=hypot;
    if (floatutils::myequalf(wind_speed,0.)) {
	wind_direction=0;
    }
    else {
	auto div=fabs(v)/hypot;
	if (div > 1.) {
	  if (div < 1.00001) {
	    div=1.;
	  }
	  else {
	    std::cerr << "**compute_wind_direction_and_speed_from_u_and_v(): error -- bad quotient to function acos()" << std::endl;
	    exit(-1);
	  }
	}
	wind_direction=(acos(div)*180./3.141592654)+0.5;
	if (u >=0) {
	  wind_direction= (v >= 0) ? wind_direction+180 : 360-wind_direction;
	}
	else if (u <= 0 && v >= 0) {
	  wind_direction=180-wind_direction;
	}
    }
  }
}

double rh(double t,double td)
{
  double e,es,c=6.11,a=7.5,b=237.7;

  if (td > t) {
    std::cerr << "**rh: error -- dewpoint " << td << " greater than temperature " << t << std::endl;
    e=-.999;
    es=1.;
  }
  else {
    auto x=a*td/(b+td);
    e=c*pow(10.,x);
    x=a*t/(b+t);
    es=c*pow(10.,x);
  }
  return (e/es*100.);
}

double stdp2z(double p)
{
/*
** this routine computes height from input pressure based on U.S.
**   standard atmosphere, 1976
*/
  double hbase[10]={0.,1.1e4,2.0e4,3.2e4,4.7e4,5.1e4,7.1e4,84852.0,84852.0,84852.0};
  double grad[10]={-0.0065,0.,0.001,0.0028,0.,-0.0028,-0.002,0.,0.,0.};
  double abs[10]={288.15,216.65,216.65,228.65,270.65,270.65,214.65,186.95,0.,0.};
  double pb[10]={1013.25,0.22632064e3,0.54748887e2,0.86801870e1,0.11090631e1,0.66938874,0.39564205e-1,0.37338360e-2,0.,0.};
  double g=980.665,r=83143200./28.9644,z;

  if (p > pb[0]) {
    return -99.;
  }
  auto n=0;
  while (n < 8 && p < pb[n]) {
    ++n;
  }
  if (n == 8) {
    return -99.;
  }
  --n;
//  isothermal layer
  if (floatutils::myequalf(grad[n],0.)) {
    z=hbase[n]+.01*r*abs[n]/g *(log(pb[n]/p));
  }
//  temp gradient not zero
  else {
    z=abs[n]/(-grad[n])*(1.-pow((p/pb[n]),(-grad[n]*r/(100.*g))))+hbase[n];
  }
  return z;
}

double tdew(double tdry,double twet,double stnp)
{
  double Ew,MRw,E,X,tdew;
  double MR=1.,MRx,last_MR=0.;
  const double e=2.7182818;

  tdry=(tdry-32.)*5./9.;
  twet=(twet-32.)*5./9.;
  stnp*=33.8637526;
  Ew=6.112*pow(e,17.67*twet/(243.5+twet));
  MRw=0.62197*Ew/(stnp-Ew);
  while (fabs(MR-last_MR) > 0.001) {
    last_MR=MR;
    MRx=(last_MR+MRw)/2.;
    MR=MRw+((1006.3+MRx*1850.)/2462880.)*(twet-tdry);
  }
  E=MR*stnp/(0.62197+MR);
  X=log(E/6.112);
  tdew=243.5*X/(17.67-X)*9./5.+32.;
  return tdew;
}

void uv2wind(double u,double v,int& dd,double& ff)
{
  if (u <= -999. || v <= -999.) {
    dd=500;
    ff=250.;
  }
  else {
    const double pi=3.141592654,deg=180./pi;
    double hypot,div;

    hypot=sqrt(u*u+v*v);
    ff=hypot;
    if (floatutils::myequalf(ff,0.)) {
	dd=0;
    }
    else {
	div=fabs(v)/hypot;
	if (div > 1.) {
	  if (div < 1.00001) {
	    div=1.;
	  }
	  else {
	    std::cerr << "uv2wind: bad quotient to function acos()" << std::endl;
	    exit(-1);
	  }
	}
	dd=acos(div)*deg+0.5;
	if (u >=0) {
	  dd= (v >= 0) ? dd+180 : 360-dd;
	}
	else if (u <= 0 && v >= 0) {
	  dd=180-dd;
	}
    }
  }
}

} // end namespace metcalcs
