#include <iomanip>
#include <grid.hpp>
#include <gridutils.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>
#include <myerror.hpp>
#ifdef __WITH_JASPER
#include <jasper/jasper.h>
#endif

using floatutils::myequalf;
using std::runtime_error;
using std::string;
using std::to_string;

#ifndef __cosutils

void GRIB2Message::unpack_ids(const unsigned char *input_buffer)
{
  off_t off=curr_off*8;
  GRIB2Grid *g2=reinterpret_cast<GRIB2Grid *>(grids.back().get());
  m_offsets.ids=curr_off;
  bits::get(input_buffer,m_lengths.ids,off,32);
  short sec_num;
  bits::get(input_buffer,sec_num,off+32,8);
  if (sec_num != 1) {
    mywarning="may not be unpacking the Identification Section";
  }
  bits::get(input_buffer,g2->grid.src,off+40,16);
  bits::get(input_buffer,g2->grib.sub_center,off+56,16);
  bits::get(input_buffer,g2->grib.table,off+72,8);
  bits::get(input_buffer,g2->grib2.local_table,off+80,8);
  bits::get(input_buffer,g2->grib2.time_sig,off+88,8);
  short yr,mo,dy,hr,min,sec;
  bits::get(input_buffer,yr,off+96,16);
  bits::get(input_buffer,mo,off+112,8);
  bits::get(input_buffer,dy,off+120,8);
  bits::get(input_buffer,hr,off+128,8);
  bits::get(input_buffer,min,off+136,8);
  bits::get(input_buffer,sec,off+144,8);
  g2->m_reference_date_time.set(yr,mo,dy,hr*10000+min*100+sec);
  bits::get(input_buffer,g2->grib2.prod_status,off+152,8);
  bits::get(input_buffer,g2->grib2.data_type,off+160,8);
  curr_off+=m_lengths.ids;
}

void GRIB2Message::pack_ids(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  short hr,min,sec;
  GRIB2Grid *g2;

  g2=reinterpret_cast<GRIB2Grid *>(grid);
bits::set(output_buffer,21,off,32);
  bits::set(output_buffer,1,off+32,8);
  bits::set(output_buffer,g2->grid.src,off+40,16);
  bits::set(output_buffer,g2->grib.sub_center,off+56,16);
  bits::set(output_buffer,g2->grib.table,off+72,8);
  bits::set(output_buffer,g2->grib2.local_table,off+80,8);
  bits::set(output_buffer,g2->grib2.time_sig,off+88,8);
  bits::set(output_buffer,g2->m_reference_date_time.year(),off+96,16);
  bits::set(output_buffer,g2->m_reference_date_time.month(),off+112,8);
  bits::set(output_buffer,g2->m_reference_date_time.day(),off+120,8);
  hr=g2->m_reference_date_time.time()/10000;
  min=(g2->m_reference_date_time.time()/100 % 100);
  sec=(g2->m_reference_date_time.time() % 100);
  bits::set(output_buffer,hr,off+128,8);
  bits::set(output_buffer,min,off+136,8);
  bits::set(output_buffer,sec,off+144,8);
  bits::set(output_buffer,g2->grib2.prod_status,off+152,8);
  bits::set(output_buffer,g2->grib2.data_type,off+160,8);
offset+=21;
}

void GRIB2Message::unpack_lus(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;

  bits::get(stream_buffer,lus_len,off,32);
  curr_off+=lus_len;
}

void GRIB2Message::pack_lus(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
}

void GRIB2Message::unpack_pds(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  short sec_num;
  unsigned char sign;
  short scale;
  int dum,noff,n,num_ranges;
  short yr,mo,dy,hr,min,sec;
  DateTime ddum;
  GRIB2Grid::StatisticalProcessRange stat_process_range;
  GRIB2Grid *g2;

  m_offsets.pds=curr_off;
  g2=reinterpret_cast<GRIB2Grid *>(grids.back().get());
  bits::get(stream_buffer,m_lengths.pds,off,32);
  bits::get(stream_buffer,sec_num,off+32,8);
  if (sec_num != 4) {
    mywarning="may not be unpacking the Product Description Section";
  }
  bits::get(stream_buffer,g2->grib2.num_coord_vals,off+40,16);
  g2->grid.fcst_time=0;
  g2->ensdata.fcst_type="";
  g2->grib2.stat_process_ranges.clear();
// template number
  bits::get(stream_buffer,g2->grib2.product_type,off+56,16);
  switch (g2->grib2.product_type) {
    case 0:
    case 1:
    case 2:
    case 8:
    case 11:
    case 12:
    case 15:
    case 61: {
      bits::get(stream_buffer,g2->grib2.param_cat,off+72,8);
      bits::get(stream_buffer,g2->grid.param,off+80,8);
      bits::get(stream_buffer,g2->grib.process,off+88,8);
      bits::get(stream_buffer,g2->grib2.backgen_process,off+96,8);
      bits::get(stream_buffer,g2->grib2.fcstgen_process,off+104,8);
      bits::get(stream_buffer,g2->grib.time_unit,off+136,8);
      bits::get(stream_buffer,g2->grid.fcst_time,off+144,32);
      bits::get(stream_buffer,g2->grid.level1_type,off+176,8);
      bits::get(stream_buffer,sign,off+192,1);
      bits::get(stream_buffer,dum,off+193,31);
      if (dum != 0x7fffffff || sign != 1) {
        if (sign == 1) {
          dum=-dum;
        }
        bits::get(stream_buffer,sign,off+184,1);
        bits::get(stream_buffer,scale,off+185,7);
        if (sign == 1) {
          scale=-scale;
        }
        g2->grid.level1=dum/pow(10.,scale);
        if (g2->grid.level1_type == 100 || g2->grid.level1_type == 108) {
          g2->grid.level1/=100.;
        }
        else if (g2->grid.level1_type == 109) {
          g2->grid.level1*=1000000000.;
        }
      }
      else {
        g2->grid.level1=Grid::MISSING_VALUE;
      }
      bits::get(stream_buffer,g2->grid.level2_type,off+224,8);
      if (g2->grid.level2_type != 255) {
        bits::get(stream_buffer,sign,off+240,1);
        bits::get(stream_buffer,dum,off+241,31);
        if (dum != 0x7fffffff || sign != 1) {
          if (sign == 1) {
            dum=-dum;
          }
          bits::get(stream_buffer,sign,off+232,1);
          bits::get(stream_buffer,scale,off+233,7);
          if (sign == 1) {
            scale=-scale;
          }
          g2->grid.level2=dum/pow(10.,scale);
          if (g2->grid.level2_type == 100 || g2->grid.level2_type == 108) {
            g2->grid.level2/=100.;
          }
          else if (g2->grid.level2_type == 109) {
            g2->grid.level2*=1000000000.;
          }
        }
        else {
          g2->grid.level2=Grid::MISSING_VALUE;
        }
      }
      else {
        g2->grid.level2=Grid::MISSING_VALUE;
      }
      switch (g2->grib2.product_type) {
        case 1:
        case 11:
        case 61: {
          bits::get(stream_buffer,dum,off+272,8);
          switch (dum) {
            case 0: {
              g2->ensdata.fcst_type="HRCTL";
              break;
            }
            case 1: {
              g2->ensdata.fcst_type="LRCTL";
              break;
            }
            case 2: {
              g2->ensdata.fcst_type="NEG";
              break;
            }
            case 3: {
              g2->ensdata.fcst_type="POS";
              break;
            }
            case 255: {
              g2->ensdata.fcst_type="UNK";
              break;
            }
            default: {
              myerror="no ensemble type mapping for "+to_string(dum);
              exit(1);
            }
          }
          bits::get(stream_buffer,dum,off+280,8);
          g2->ensdata.id=to_string(dum);
          bits::get(stream_buffer,g2->ensdata.total_size,off+288,8);
          switch (g2->grib2.product_type) {
            case 61: {
              bits::get(stream_buffer,yr,off+296,16);
              bits::get(stream_buffer,mo,off+312,8);
              bits::get(stream_buffer,dy,off+320,8);
              bits::get(stream_buffer,hr,off+328,8);
              bits::get(stream_buffer,min,off+336,8);
              bits::get(stream_buffer,sec,off+344,8);
              g2->grib2.modelv_date.set(yr,mo,dy,hr*10000+min*100+sec);
              off+=56;
            }
            case 11: {
              bits::get(stream_buffer,yr,off+296,16);
              bits::get(stream_buffer,mo,off+312,8);
              bits::get(stream_buffer,dy,off+320,8);
              bits::get(stream_buffer,hr,off+328,8);
              bits::get(stream_buffer,min,off+336,8);
              bits::get(stream_buffer,sec,off+344,8);
              g2->grib2.stat_period_end.set(yr,mo,dy,hr*10000+min*100+sec);
bits::get(stream_buffer,dum,off+352,8);
if (dum > 1) {
myerror="error processing multiple ("+to_string(dum)+") statistical processes for product type "+to_string(g2->grib2.product_type);
exit(1);
}
              bits::get(stream_buffer,g2->grib2.stat_process_nmissing,off+360,32);
              bits::get(stream_buffer,stat_process_range.type,off+392,8);
              bits::get(stream_buffer,stat_process_range.time_increment_type,off+400,8);
              bits::get(stream_buffer,stat_process_range.period_length.unit,off+408,8);
              bits::get(stream_buffer,stat_process_range.period_length.value,off+416,32);
              bits::get(stream_buffer,stat_process_range.period_time_increment.unit,off+448,8);
              bits::get(stream_buffer,stat_process_range.period_time_increment.value,off+456,32);
              g2->grib2.stat_process_ranges.emplace_back(stat_process_range);
              break;
            }
          }
          break;
        }
        case 2:
        case 12: {
          bits::get(stream_buffer,dum,off+272,8);
          switch (dum) {
            case 0: {
              g2->ensdata.fcst_type="UNWTD_MEAN";
              break;
            }
            case 1: {
              g2->ensdata.fcst_type="WTD_MEAN";
              break;
            }
            case 2: {
              g2->ensdata.fcst_type="STDDEV";
              break;
            }
            case 3: {
              g2->ensdata.fcst_type="STDDEV_NORMED";
              break;
            }
            case 4: {
              g2->ensdata.fcst_type="SPREAD";
              break;
            }
            case 5: {
              g2->ensdata.fcst_type="LRG_ANOM_INDEX";
              break;
            }
            case 6: {
              g2->ensdata.fcst_type="UNWTD_MEAN";
              break;
            }
            default: {
              myerror="no ensemble type mapping for "+to_string(dum);
              exit(1);
            }
          }
          g2->ensdata.id="ALL";
          bits::get(stream_buffer,g2->ensdata.total_size,off+280,8);
          switch (g2->grib2.product_type) {
            case 12: {
              bits::get(stream_buffer,yr,off+288,16);
              bits::get(stream_buffer,mo,off+304,8);
              bits::get(stream_buffer,dy,off+312,8);
              bits::get(stream_buffer,hr,off+320,8);
              bits::get(stream_buffer,min,off+328,8);
              bits::get(stream_buffer,sec,off+336,8);
              g2->grib2.stat_period_end.set(yr,mo,dy,hr*10000+min*100+sec);
bits::get(stream_buffer,dum,off+344,8);
if (dum > 1) {
myerror="error processing multiple ("+to_string(dum)+") statistical processes for product type "+to_string(g2->grib2.product_type);
exit(1);
}
              bits::get(stream_buffer,g2->grib2.stat_process_nmissing,off+352,32);
              bits::get(stream_buffer,stat_process_range.type,off+384,8);
              bits::get(stream_buffer,stat_process_range.time_increment_type,off+392,8);
              bits::get(stream_buffer,stat_process_range.period_length.unit,off+400,8);
              bits::get(stream_buffer,stat_process_range.period_length.value,off+408,32);
              bits::get(stream_buffer,stat_process_range.period_time_increment.unit,off+440,8);
              bits::get(stream_buffer,stat_process_range.period_time_increment.value,off+448,32);
              g2->grib2.stat_process_ranges.emplace_back(stat_process_range);
              break;
            }
          }
          break;
        }
        case 8: {
          bits::get(stream_buffer,yr,off+272,16);
          bits::get(stream_buffer,mo,off+288,8);
          bits::get(stream_buffer,dy,off+296,8);
          bits::get(stream_buffer,hr,off+304,8);
          bits::get(stream_buffer,min,off+312,8);
          bits::get(stream_buffer,sec,off+320,8);
          g2->grib2.stat_period_end.set(yr,mo,dy,hr*10000+min*100+sec);
          bits::get(stream_buffer,num_ranges,off+328,8);
// patch
          if (num_ranges == 0) {
            num_ranges=(m_lengths.pds-46)/12;
          }
          bits::get(stream_buffer,g2->grib2.stat_process_nmissing,off+336,32);
/*
if (num_ranges > 1) {
myerror="error processing multiple ("+to_string(num_ranges)+") statistical processes for product type "+to_string(g2->grib2.product_type);
exit(1);
}
*/
          noff=368;
          for (n=0; n < num_ranges; ++n) {
            bits::get(stream_buffer,stat_process_range.type,off+noff,8);
            bits::get(stream_buffer,stat_process_range.time_increment_type,off+noff+8,8);
            bits::get(stream_buffer,stat_process_range.period_length.unit,off+noff+16,8);
            bits::get(stream_buffer,stat_process_range.period_length.value,off+noff+24,32);
            bits::get(stream_buffer,stat_process_range.period_time_increment.unit,off+noff+56,8);
            bits::get(stream_buffer,stat_process_range.period_time_increment.value,off+noff+64,32);
            g2->grib2.stat_process_ranges.emplace_back(stat_process_range);
            noff+=96;
          }
          break;
        }
        case 15: {
          bits::get(stream_buffer,g2->grib2.spatial_process.stat_process,off+272,8);
          bits::get(stream_buffer,g2->grib2.spatial_process.type,off+280,8);
          bits::get(stream_buffer,g2->grib2.spatial_process.num_points,off+288,8);
          break;
        }
      }
      break;
    }
    default: {
      myerror="product template "+to_string(g2->grib2.product_type)+" not recognized";
      exit(1);
    }
  }
  switch (g2->grib.time_unit) {
case 0: {
break;
}
    case 1: {
      break;
    }
    case 3: {
      ddum=g2->m_reference_date_time.months_added(g2->grid.fcst_time);
      g2->grid.fcst_time=ddum.hours_since(g2->m_reference_date_time);
      break;
    }
    case 11: {
      g2->grid.fcst_time*=6;
      break;
    }
    case 12: {
      g2->grid.fcst_time*=12;
      break;
    }
    default: {
      myerror="unable to adjust valid time for time units "+to_string(g2->grib.time_unit);
      exit(1);
    }
  }
/*
  if (g2->grid.src == 7 && g2->grib2.stat_process_ranges.size() > 1 && g2->grib2.stat_process_ranges[0].type == 255 && g2->grib2.stat_process_ranges[1].type != 255) {
    g2->grib2.stat_process_ranges[0].type=g2->grib2.stat_process_ranges[1].type;
  }
*/
/*
  if (g2->grib2.stat_process_ranges.size() > 0) {
    if (((g2->grib2.stat_process_ranges[0].type >= 0 && g2->grib2.stat_process_ranges[0].type <= 4) || g2->grib2.stat_process_ranges[0].type == 8 || (g2->grib2.stat_process_ranges[0].type == 255 && g2->grib2.stat_period_end.year() > 0))) {
//if (((g2->grib2.stat_process_ranges[0].type >= 0 && g2->grib2.stat_process_ranges[0].type <= 4) || g2->grib2.stat_process_ranges[0].type == 8)) {
      g2->m_valid_date_time=g2->m_reference_date_time.hours_added(g2->grid.fcst_time);
    }
    else if (g2->grib2.stat_process_ranges[0].type > 191) {
      g2->m_valid_date_time=g2->m_reference_date_time.hours_added(g2->grid.fcst_time);
      lastValidDateTime=g2->m_valid_date_time;
      grib_utils::GRIB2_product_description(g2,g2->m_valid_date_time,lastValidDateTime);
      if (lastValidDateTime != g2->m_valid_date_time) {
        g2->grib2.stat_period_end=lastValidDateTime;
      }
    }
  }
  else {
    g2->m_valid_date_time=g2->m_reference_date_time.hours_added(g2->grid.fcst_time);
  }
*/
if (g2->grib.time_unit == 0) {
g2->m_forecast_date_time=g2->m_valid_date_time=g2->m_reference_date_time.minutes_added(g2->grid.fcst_time);
g2->grid.fcst_time*=100;
}
else {
  g2->m_forecast_date_time=g2->m_valid_date_time=g2->m_reference_date_time.hours_added(g2->grid.fcst_time);
  g2->grid.fcst_time*=10000;
}
  std::tie(g2->grib.prod_descr, g2->m_forecast_date_time, g2->
      m_valid_date_time) = grib_utils::grib2_product(g2);
  curr_off += m_lengths.pds;
}

void GRIB2Message::pack_pds(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  size_t fcst_time;
  short hr,min,sec,scale;
  long long ldum;
  double lval;
  unsigned char sign;
  int noff,n;
  GRIB2Grid *g2;

  g2=reinterpret_cast<GRIB2Grid *>(grid);
  bits::set(output_buffer,4,off+32,8);
bits::set(output_buffer,0,off+40,16);
  fcst_time=g2->grid.fcst_time/10000;
// template number
  bits::set(output_buffer,g2->grib2.product_type,off+56,16);
  switch (g2->grib2.product_type) {
    case 0:
    case 1:
    case 2:
    case 8:
    case 11:
    case 12: {
      bits::set(output_buffer,g2->grib2.param_cat,off+72,8);
      bits::set(output_buffer,g2->grid.param,off+80,8);
      bits::set(output_buffer,g2->grib.process,off+88,8);
      bits::set(output_buffer,g2->grib2.backgen_process,off+96,8);
      bits::set(output_buffer,g2->grib2.fcstgen_process,off+104,8);
      for (auto n=offset+14; n < offset+17; output_buffer[n++]=0);
      bits::set(output_buffer,g2->grib.time_unit,off+136,8);
      bits::set(output_buffer,fcst_time,off+144,32);
      bits::set(output_buffer,g2->grid.level1_type,off+176,8);
      if (g2->grid.level1 == Grid::MISSING_VALUE) {
        bits::set(output_buffer,0xff,off+184,8);
        bits::set(output_buffer,static_cast<size_t>(0xffffffff),off+192,32);
      }
      else {
        lval=g2->grid.level1;
        if (lval < 0.)
          sign=1;
        else
          sign=0;
        if (g2->grid.level1_type == 100 || g2->grid.level1_type == 108)
          lval*=100.;
        else if (g2->grid.level1_type == 109)
          lval/=1000000000.;
        scale=0;
        if (myequalf(lval,static_cast<long long>(lval),0.00000000001)) {
          ldum=lval;
          while (ldum > 0 && (ldum % 10) == 0) {
            ++scale;
            ldum/=10;
          }
          bits::set(output_buffer,ldum,off+193,31);
        }
        else {
          lval*=10.;
          --scale;
          while (!myequalf(lval,llround(lval),0.00000000001)) {
            --scale;
            lval*=10.;
          }
          bits::set(output_buffer,llround(lval),off+193,31);
        }
        if (scale > 0) {
          bits::set(output_buffer,1,off+184,1);
        }
        else {
          bits::set(output_buffer,0,off+184,1);
          scale=-scale;
        }
        bits::set(output_buffer,scale,off+185,7);
        bits::set(output_buffer,sign,off+192,1);
      }
      bits::set(output_buffer,g2->grid.level2_type,off+224,8);
      if (g2->grid.level2 == Grid::MISSING_VALUE) {
        bits::set(output_buffer,0xff,off+232,8);
        bits::set(output_buffer,static_cast<size_t>(0xffffffff),off+240,32);
      }
      else {
        lval=g2->grid.level2;
        if (lval < 0.) {
          sign=1;
        }
        else {
          sign=0;
        }
        if (g2->grid.level2_type == 100 || g2->grid.level2_type == 108) {
          lval*=100.;
        }
        else if (g2->grid.level2_type == 109) {
          lval/=1000000000.;
        }
        scale=0;
        if (myequalf(lval,static_cast<long long>(lval),0.00000000001)) {
          ldum=lval;
          while (ldum > 0 && (ldum % 10) == 0) {
            ++scale;
            ldum/=10;
          }
          bits::set(output_buffer,ldum,off+241,31);
        }
        else {
          lval*=10.;
          --scale;
          while (!myequalf(lval,llround(lval),0.00000000001)) {
            --scale;
            lval*=10.;
          }
          bits::set(output_buffer,llround(lval),off+241,31);
        }
        if (scale > 0)
          bits::set(output_buffer,1,off+232,1);
        else {
          bits::set(output_buffer,0,off+232,1);
          scale=-scale;
        }
        bits::set(output_buffer,scale,off+233,7);
        bits::set(output_buffer,sign,off+240,1);
      }
      switch (g2->grib2.product_type) {
        case 0: {
          bits::set(output_buffer,34,off,32);
          offset+=34;
          break;
        }
        case 1:
        case 11: {
std::cerr << "unable to finish packing PDS for product type " << g2->grib2.product_type << std::endl;
          break;
        }
        case 2:
        case 12: {
std::cerr << "unable to finish packing PDS for product type " << g2->grib2.product_type << std::endl;
          break;
        }
        case 8: {
          bits::set(output_buffer,g2->grib2.stat_period_end.year(),off+272,16);
          bits::set(output_buffer,g2->grib2.stat_period_end.month(),off+288,8);
          bits::set(output_buffer,g2->grib2.stat_period_end.day(),off+296,8);
          hr=g2->grib2.stat_period_end.time()/10000;
          min=(g2->grib2.stat_period_end.time()/100 % 100);
          sec=(g2->grib2.stat_period_end.time() % 100);
          bits::set(output_buffer,hr,off+304,8);
          bits::set(output_buffer,min,off+312,8);
          bits::set(output_buffer,sec,off+320,8);
          bits::set(output_buffer,g2->grib2.stat_process_ranges.size(),off+328,8);
          bits::set(output_buffer,g2->grib2.stat_process_nmissing,off+336,32);
          noff=368;
          for (n=0; n < static_cast<int>(g2->grib2.stat_process_ranges.size()); ++n) {
            bits::set(output_buffer,g2->grib2.stat_process_ranges[n].type,off+noff,8);
            bits::set(output_buffer,g2->grib2.stat_process_ranges[n].time_increment_type,off+noff+8,8);
            bits::set(output_buffer,g2->grib2.stat_process_ranges[n].period_length.unit,off+noff+16,8);
            bits::set(output_buffer,g2->grib2.stat_process_ranges[n].period_length.value,off+noff+24,32);
            bits::set(output_buffer,g2->grib2.stat_process_ranges[n].period_time_increment.unit,off+noff+56,8);
            bits::set(output_buffer,g2->grib2.stat_process_ranges[n].period_time_increment.value,off+noff+64,32);
            noff+=96;
          }
          bits::set(output_buffer,noff/8,off,32);
          offset+=noff/8;
          break;
        }
      }
      break;
    }
  }
}

void GRIB2Message::unpack_gds(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  short sec_num;
  int dum;
  unsigned char sign;
  size_t sdum;
  GRIB2Grid *g2;

  m_offsets.gds=curr_off;
  g2=reinterpret_cast<GRIB2Grid *>(grids.back().get());
  bits::get(stream_buffer,m_lengths.gds,off,32);
  bits::get(stream_buffer,sec_num,off+32,8);
  if (sec_num != 3) {
    mywarning="may not be unpacking the Grid Description Section";
  }
  bits::get(stream_buffer,dum,off+40,8);
if (dum != 0) {
myerror="grid has no template";
exit(1);
}
  bits::get(stream_buffer,g2->dim.size,off+48,32);
  bits::get(stream_buffer,g2->grib2.lp_width,off+80,8);
  if (g2->grib2.lp_width != 0)
    bits::get(stream_buffer,g2->grib2.lpi,off+88,8);
  else
    g2->grib2.lpi=0;
// template number
  bits::get(stream_buffer,g2->grid.grid_type,off+96,16);
  switch (g2->grid.grid_type) {
    case 0: {
      g2->def.type=Grid::Type::latitudeLongitude;
      break;
    }
    case 10: {
      g2->def.type=Grid::Type::mercator;
      break;
    }
    case 30: {
      g2->def.type=Grid::Type::lambertConformal;
      break;
    }
    case 40: {
      g2->def.type=Grid::Type::gaussianLatitudeLongitude;
      break;
    }
    default: {
      myerror="no unpacking for grid type "+to_string(g2->grid.grid_type);
      exit(1);
    }
  }
  switch (g2->grid.grid_type) {
    case 0:
    case 40: {
      bits::get(stream_buffer,g2->grib2.earth_shape,off+112,8);
      bits::get(stream_buffer,g2->dim.x,off+256,16);
      bits::get(stream_buffer,g2->dim.y,off+288,16);
      bits::get(stream_buffer,sdum,off+369,31);
      g2->def.slatitude=sdum/1000000.;
      bits::get(stream_buffer,sign,off+368,1);
      if (sign == 1) {
        g2->def.slatitude=-(g2->def.slatitude);
      }
      bits::get(stream_buffer,sdum,off+401,31);
      g2->def.slongitude=sdum/1000000.;
      bits::get(stream_buffer,sign,off+400,1);
      if (sign == 1) {
        g2->def.slongitude=-(g2->def.slongitude);
      }
      bits::get(stream_buffer,g2->grib.rescomp,off+432,8);
      bits::get(stream_buffer,sdum,off+441,31);
      g2->def.elatitude=sdum/1000000.;
      bits::get(stream_buffer,sign,off+440,1);
      if (sign == 1) {
        g2->def.elatitude=-(g2->def.elatitude);
      }
      bits::get(stream_buffer,sdum,off+473,31);
      g2->def.elongitude=sdum/1000000.;
      bits::get(stream_buffer,sign,off+472,1);
      if (sign == 1) {
        g2->def.elongitude=-(g2->def.elongitude);
      }
      bits::get(stream_buffer,sdum,off+504,32);
      g2->def.loincrement=sdum/1000000.;
      bits::get(stream_buffer,g2->def.num_circles,off+536,32);
      bits::get(stream_buffer,g2->grib.scan_mode,off+568,8);
      if (g2->grid.grid_type == 0) {
        g2->def.laincrement=g2->def.num_circles/1000000.;
      }
      off=576;
      break;
    }
    case 10: {
      bits::get(stream_buffer,g2->grib2.earth_shape,off+112,8);
      bits::get(stream_buffer,g2->dim.x,off+256,16);
      bits::get(stream_buffer,g2->dim.y,off+288,16);
      bits::get(stream_buffer,sdum,off+305,31);
      g2->def.slatitude=sdum/1000000.;
      bits::get(stream_buffer,sign,off+304,1);
      if (sign == 1) {
        g2->def.slatitude=-(g2->def.slatitude);
      }
      bits::get(stream_buffer,sdum,off+337,31);
      g2->def.slongitude=sdum/1000000.;
      bits::get(stream_buffer,sign,off+336,1);
      if (sign == 1) {
        g2->def.slongitude=-(g2->def.slongitude);
      }
      bits::get(stream_buffer,g2->grib.rescomp,off+368,8);
      bits::get(stream_buffer,sdum,off+377,31);
      g2->def.stdparallel1=sdum/1000000.;
      bits::get(stream_buffer,sign,off+376,1);
      if (sign == 1) {
        g2->def.stdparallel1=-(g2->def.stdparallel1);
      }
      bits::get(stream_buffer,sdum,off+409,31);
      g2->def.elatitude=sdum/1000000.;
      bits::get(stream_buffer,sign,off+408,1);
      if (sign == 1) {
        g2->def.elatitude=-(g2->def.elatitude);
      }
      bits::get(stream_buffer,sdum,off+441,31);
      g2->def.elongitude=sdum/1000000.;
      bits::get(stream_buffer,sign,off+440,1);
      if (sign == 1) {
        g2->def.elongitude=-(g2->def.elongitude);
      }
      bits::get(stream_buffer,g2->grib.scan_mode,off+472,8);
      bits::get(stream_buffer,sdum,off+512,32);
      g2->def.dx=sdum/1000000.;
      bits::get(stream_buffer,sdum,off+544,32);
      g2->def.dy=sdum/1000000.;
      break;
    }
    case 30: {
      bits::get(stream_buffer,g2->grib2.earth_shape,off+112,8);
      bits::get(stream_buffer,g2->dim.x,off+256,16);
      bits::get(stream_buffer,g2->dim.y,off+288,16);
      bits::get(stream_buffer,sdum,off+305,31);
      g2->def.slatitude=sdum/1000000.;
      bits::get(stream_buffer,sign,off+304,1);
      if (sign == 1) {
        g2->def.slatitude=-(g2->def.slatitude);
      }
      bits::get(stream_buffer,sdum,off+337,31);
      g2->def.slongitude=sdum/1000000.;
      bits::get(stream_buffer,sign,off+336,1);
      if (sign == 1) {
        g2->def.slongitude=-(g2->def.slongitude);
      }
      bits::get(stream_buffer,g2->grib.rescomp,off+368,8);
      bits::get(stream_buffer,sdum,off+377,31);
      g2->def.llatitude=sdum/1000000.;
      bits::get(stream_buffer,sign,off+376,1);
      if (sign == 1) {
        g2->def.llatitude=-(g2->def.llatitude);
      }
      bits::get(stream_buffer,sdum,off+409,31);
      g2->def.olongitude=sdum/1000000.;
      bits::get(stream_buffer,sign,off+408,1);
      if (sign == 1) {
        g2->def.olongitude=-(g2->def.olongitude);
      }
      bits::get(stream_buffer,sdum,off+440,32);
      g2->def.dx=sdum/1000000.;
      bits::get(stream_buffer,sdum,off+472,32);
      g2->def.dy=sdum/1000000.;
      bits::get(stream_buffer,g2->def.projection_flag,off+504,8);
      bits::get(stream_buffer,g2->grib.scan_mode,off+512,8);
      bits::get(stream_buffer,sdum,off+521,31);
      g2->def.stdparallel1=sdum/1000000.;
      bits::get(stream_buffer,sign,off+520,1);
      if (sign == 1) {
        g2->def.stdparallel1=-(g2->def.stdparallel1);
      }
      bits::get(stream_buffer,sdum,off+553,31);
      g2->def.stdparallel2=sdum/1000000.;
      bits::get(stream_buffer,sign,off+552,1);
      if (sign == 1) {
        g2->def.stdparallel2=-(g2->def.stdparallel2);
      }
      bits::get(stream_buffer,sdum,off+585,31);
      g2->grib.sp_lat=sdum/1000000.;
      bits::get(stream_buffer,sign,off+584,1);
      if (sign == 1) {
        g2->grib.sp_lat=-(g2->grib.sp_lat);
      }
      bits::get(stream_buffer,sdum,off+617,31);
      g2->grib.sp_lon=sdum/1000000.;
      bits::get(stream_buffer,sign,off+616,1);
      if (sign == 1) {
        g2->grib.sp_lon=-(g2->grib.sp_lon);
      }
      break;
    }
    default: {
      myerror="grid template "+to_string(dum)+" not recognized";
      exit(1);
    }
  }
// reduced grids
  if (g2->dim.x < 0) {
    if (g2->plist != nullptr)
      delete[] g2->plist;
    g2->plist=new int[g2->dim.y];
    bits::get(stream_buffer,g2->plist,off,g2->grib2.lp_width*8,0,g2->dim.y);
    g2->def.loincrement=(g2->def.elongitude-g2->def.slongitude)/(g2->plist[g2->dim.y/2]-1);
  }
  g2->def=gridutils::fix_grid_definition(g2->def,g2->dim);
  curr_off+=m_lengths.gds;
}

void GRIB2Message::pack_gds(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  long long sdum;
  GRIB2Grid *g2;

  g2=reinterpret_cast<GRIB2Grid *>(grid);
  bits::set(output_buffer,3,off+32,8);
bits::set(output_buffer,0,off+40,8);
  bits::set(output_buffer,g2->dim.size,off+48,32);
  bits::set(output_buffer,g2->grib2.lp_width,off+80,8);
  bits::set(output_buffer,g2->grib2.lpi,off+88,8);
// template number
  bits::set(output_buffer,g2->grid.grid_type,off+96,16);
  switch (g2->grid.grid_type) {
    case 0:
    case 40: {
      bits::set(output_buffer,g2->grib2.earth_shape,off+112,8);
      for (auto n=offset+15; n < offset+30; output_buffer[n++]=0);
      bits::set(output_buffer,0,off+240,16);
      bits::set(output_buffer,g2->dim.x,off+256,16);
      bits::set(output_buffer,0,off+272,16);
      bits::set(output_buffer,g2->dim.y,off+288,16);
bits::set(output_buffer,0,off+304,32);
bits::set(output_buffer,0xffffffff,off+336,32);
      sdum=g2->def.slatitude*1000000.;
      if (sdum < 0)
        sdum=-sdum;
      bits::set(output_buffer,sdum,off+369,31);
      if (g2->def.slatitude < 0.)
        bits::set(output_buffer,1,off+368,1);
      else
        bits::set(output_buffer,0,off+368,1);
      sdum=g2->def.slongitude*1000000.;
      if (sdum < 0)
        sdum+=360000000;
      bits::set(output_buffer,sdum,off+400,32);
      bits::set(output_buffer,g2->grib.rescomp,off+432,8);
      sdum=g2->def.elatitude*1000000.;
      if (sdum < 0)
        sdum=-sdum;
      bits::set(output_buffer,sdum,off+441,31);
      if (g2->def.elatitude < 0.)
        bits::set(output_buffer,1,off+440,1);
      else
        bits::set(output_buffer,0,off+440,1);
      sdum=g2->def.elongitude*1000000.;
      if (sdum < 0)
        sdum=-sdum;
      bits::set(output_buffer,sdum,off+473,31);
      if (g2->def.elongitude < 0.)
        bits::set(output_buffer,1,off+472,1);
      else
        bits::set(output_buffer,0,off+472,1);
//      sdum=(long long)(g2->def.loincrement*1000000.);
sdum=llround(g2->def.loincrement*1000000.);
      bits::set(output_buffer,sdum,off+504,32);
      if (g2->grid.grid_type == 0)
//        sdum=(long long)(g2->def.laincrement*1000000.);
sdum=llround(g2->def.laincrement*1000000.);
      else
        sdum=g2->def.num_circles;
      bits::set(output_buffer,sdum,off+536,32);
      bits::set(output_buffer,g2->grib.scan_mode,off+568,8);
      bits::set(output_buffer,72,off,32);
      offset+=72;
      break;
    }
    default: {
      myerror="unable to pack GDS for template "+to_string(g2->grid.grid_type);
      exit(1);
    }
  }
}

void GRIB2Message::unpack_drs(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  short sec_num;
  union {
    float dum;
    int idum;
  };
  GRIB2Grid *g2;
  int num_packed;

  m_offsets.drs=curr_off;
  g2=reinterpret_cast<GRIB2Grid *>(grids.back().get());
  bits::get(stream_buffer,m_lengths.drs,off,32);
  bits::get(stream_buffer,sec_num,off+32,8);
  if (sec_num != 5) {
    mywarning="may not be unpacking the Data Representation Section";
  }
  bits::get(stream_buffer,num_packed,off+40,32);
  g2->grid.num_missing=g2->dim.size-num_packed;
// template number
  bits::get(stream_buffer,g2->grib2.data_rep,off+72,16);
  switch (g2->grib2.data_rep) {
    case 0:
    case 2:
    case 3:
    case 40:
    case 40000: {
      bits::get(stream_buffer,idum,off+88,32);
      bits::get(stream_buffer,g2->grib.E,off+120,16);
      if (g2->grib.E > 0x8000)
        g2->grib.E=0x8000-g2->grib.E;
      bits::get(stream_buffer,g2->grib.D,off+136,16);
      if (g2->grib.D > 0x8000)
        g2->grib.D=0x8000-g2->grib.D;
      g2->stats.min_val=dum/pow(10.,g2->grib.D);
      bits::get(stream_buffer,g2->grib.pack_width,off+152,8);
      bits::get(stream_buffer,g2->grib2.orig_val_type,off+160,8);
      if (g2->grib2.data_rep == 2 || g2->grib2.data_rep == 3) {
        bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.split_method,off+168,8);
        bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.miss_val_mgmt,off+176,8);
        if (g2->grib2.orig_val_type == 0) {
          bits::get(stream_buffer,idum,off+184,32);
          g2->grib2.complex_pack.grid_point.primary_miss_sub=dum;
          bits::get(stream_buffer,idum,off+216,32);
          g2->grib2.complex_pack.grid_point.secondary_miss_sub=dum;
        }
        else if (g2->grib2.orig_val_type == 1) {
          bits::get(stream_buffer,idum,off+184,32);
          g2->grib2.complex_pack.grid_point.primary_miss_sub=idum;
          bits::get(stream_buffer,idum,off+216,32);
          g2->grib2.complex_pack.grid_point.secondary_miss_sub=idum;
        }
        else {
          myerror="unable to decode missing value substitutes for original value type of "+to_string(g2->grib2.orig_val_type);
          exit(1);
        }
        bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.num_groups,off+248,32);
        bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.width_ref,off+280,8);
        bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.width_pack_width,off+288,8);
        bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.length_ref,off+296,32);
        bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.length_incr,off+328,8);
        bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.last_length,off+336,32);
        bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.length_pack_width,off+368,8);
        if (g2->grib2.data_rep == 3) {
          bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.spatial_diff.order,off+376,8);
          bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.spatial_diff.order_vals_width,off+384,8);
        }
      }
      break;
    }
    default: {
      myerror="data template "+to_string(g2->grib2.data_rep)+" is not understood";
      exit(1);
    }
  }
  curr_off+=m_lengths.drs;
}

void GRIB2Message::pack_drs(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  union {
    float dum;
    int idum;
  };
  GRIB2Grid *g2=reinterpret_cast<GRIB2Grid *>(grid);

  bits::set(output_buffer,5,off+32,8);
  bits::set(output_buffer,g2->dim.size-(g2->grid.num_missing),off+40,32);
// template number
  bits::set(output_buffer,g2->grib2.data_rep,off+72,16);
  switch (g2->grib2.data_rep) {
    case 0:
    case 2:
    case 3:
    case 40:
    case 40000: {
      dum=lroundf(g2->stats.min_val*pow(10.,g2->grib.D));
      bits::set(output_buffer,idum,off+88,32);
      idum=g2->grib.E;
      if (idum < 0)
        idum=0x8000-idum;
      bits::set(output_buffer,idum,off+120,16);
      idum=g2->grib.D;
      if (idum < 0)
        idum=0x8000-idum;
      bits::set(output_buffer,idum,off+136,16);
      bits::set(output_buffer,g2->grib.pack_width,off+152,8);
      bits::set(output_buffer,g2->grib2.orig_val_type,off+160,8);
      switch (g2->grib2.data_rep) {
        case 0: {
          bits::set(output_buffer,21,off,32);
          offset+=21;
          break;
        }
        case 2:
        case 3: {
          bits::set(output_buffer,g2->grib2.complex_pack.grid_point.split_method,off+168,8);
          bits::set(output_buffer,g2->grib2.complex_pack.grid_point.miss_val_mgmt,off+176,8);
          bits::set(output_buffer,23,off,32);
          offset+=23;
          break;
        }
        case 40: {
          bits::set(output_buffer,0,off+168,8);
          bits::set(output_buffer,0xff,off+176,8);
          bits::set(output_buffer,23,off,32);
          offset+=23;
          break;
        }
      }
      break;
    }
    default: {
      myerror="unable to pack data template "+to_string(g2->grib2.data_rep);
      exit(1);
    }
  }
}

void GRIB2Message::unpack_ds(const unsigned char *stream_buffer,bool fill_header_only)
{
  struct {
    short sign;
    int omin;
    long long miss_val,group_miss_val;
    int max_length;
  } groups;

  auto off=curr_off*8;
  m_offsets.ds=curr_off;
  bits::get(stream_buffer,m_lengths.ds,off,32);
  short sec_num;
  bits::get(stream_buffer,sec_num,off+32,8);
  if (sec_num != 7) {
    mywarning="may not be unpacking the Data Section";
  }
  if (!fill_header_only) {
    GRIB2Grid *g2=reinterpret_cast<GRIB2Grid *>(grids.back().get());
    g2->stats.max_val=-Grid::MISSING_VALUE;
    double D=pow(10.,g2->grib.D);
    double E=pow(2.,g2->grib.E);
    switch (g2->grib2.data_rep) {
      case 0: {
        g2->galloc();
        off+=40;
        size_t x=0;
        auto avg_cnt=0;
        for (int n=0; n < g2->dim.y; ++n) {
          for (int m=0; m < g2->dim.x; ++m) {
            if (!g2->bitmap.applies || g2->bitmap.map[x] == 1) {
              size_t pval=0;
              bits::get(stream_buffer,pval,off,g2->grib.pack_width);
              g2->m_gridpoints[n][m]=g2->stats.min_val+pval*E/D;
              if (g2->m_gridpoints[n][m] > g2->stats.max_val) {
                g2->stats.max_val=g2->m_gridpoints[n][m];
              }
              g2->stats.avg_val+=g2->m_gridpoints[n][m];
              ++avg_cnt;
              off+=g2->grib.pack_width;
            }
            else {
              g2->m_gridpoints[n][m]=Grid::MISSING_VALUE;
            }
            ++x;
          }
        }
        g2->stats.avg_val/=static_cast<double>(avg_cnt);
        g2->grid.filled=true;
        break;
      }
      case 2: {
        if (g2->grib.scan_mode != 0) {
          myerror="unable to decode ddef2 for scan mode "+to_string(g2->grib.scan_mode);
          exit(1);
        }
        g2->galloc();
        if (g2->grib2.complex_pack.grid_point.miss_val_mgmt > 0) {
          groups.miss_val=pow(2.,g2->grib.pack_width)-1;
        }
        else {
          groups.miss_val=Grid::MISSING_VALUE;
        }
        off+=40;
        g2->grib2.complex_pack.grid_point.ref_vals.resize(g2->grib2.complex_pack.grid_point.num_groups);
        if (g2->grib.pack_width > 0) {
          bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.ref_vals.data(),off,g2->grib.pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
        }
        else {
          for (int n=0; n < g2->grib2.complex_pack.grid_point.num_groups; ++n) {
            g2->grib2.complex_pack.grid_point.ref_vals[n]=0;
          }
        }
        off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib.pack_width;
        auto pad=(off % 8);
        if (pad > 0) {
          off+=8-pad;
        }
        g2->grib2.complex_pack.grid_point.widths.resize(g2->grib2.complex_pack.grid_point.num_groups);
        if (g2->grib2.complex_pack.grid_point.width_pack_width > 0) {
          bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.widths.data(),off,g2->grib2.complex_pack.grid_point.width_pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
          for (int n=0; n < g2->grib2.complex_pack.grid_point.num_groups; ++n) {
            g2->grib2.complex_pack.grid_point.widths[n]+=g2->grib2.complex_pack.grid_point.width_ref;
          }
        }
        else {
          for (int n=0; n < g2->grib2.complex_pack.grid_point.num_groups; ++n) {
            g2->grib2.complex_pack.grid_point.widths[n]=g2->grib2.complex_pack.grid_point.width_ref;
          }
        }
        off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib2.complex_pack.grid_point.width_pack_width;
        pad=(off % 8);
        if (pad > 0) {
          off+=8-pad;
        }
        g2->grib2.complex_pack.grid_point.lengths.resize(g2->grib2.complex_pack.grid_point.num_groups);
        bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.lengths.data(),off,g2->grib2.complex_pack.grid_point.length_pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
        off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib2.complex_pack.grid_point.length_pack_width;
        pad=(off % 8);
        if (pad > 0) {
          off+=8-pad;
        }
        groups.max_length=0;
        for (int n=0; n < g2->grib2.complex_pack.grid_point.num_groups; ++n) {
          g2->grib2.complex_pack.grid_point.lengths[n]=g2->grib2.complex_pack.grid_point.length_ref+g2->grib2.complex_pack.grid_point.lengths[n]*g2->grib2.complex_pack.grid_point.length_incr;
          if (g2->grib2.complex_pack.grid_point.lengths[n] > groups.max_length) {
            groups.max_length=g2->grib2.complex_pack.grid_point.lengths[n];
          }
        }
        g2->grib2.complex_pack.grid_point.pvals.resize(groups.max_length);
        size_t gridpoint_index=0;
        auto avg_cnt=0;
        for (int n=0; n < g2->grib2.complex_pack.grid_point.num_groups; ++n) {
          if (g2->grib2.complex_pack.grid_point.widths[n] > 0) {
            if (g2->grib2.complex_pack.grid_point.miss_val_mgmt > 0) {
              groups.group_miss_val=pow(2.,g2->grib2.complex_pack.grid_point.widths[n])-1;
            }
            else {
              groups.group_miss_val=Grid::MISSING_VALUE;
            }
            bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.pvals.data(),off,g2->grib2.complex_pack.grid_point.widths[n],0,g2->grib2.complex_pack.grid_point.lengths[n]);
            off+=g2->grib2.complex_pack.grid_point.widths[n]*g2->grib2.complex_pack.grid_point.lengths[n];
            for (int m=0; m < g2->grib2.complex_pack.grid_point.lengths[n]; ++m) {
              auto j=gridpoint_index/g2->dim.x;
              auto i=(gridpoint_index % g2->dim.x);
              if (g2->grib2.complex_pack.grid_point.pvals[m] == groups.group_miss_val || (g2->bitmap.applies && g2->bitmap.map[gridpoint_index] == 0)) {
                g2->m_gridpoints[j][i]=Grid::MISSING_VALUE;
              }
              else {
                g2->m_gridpoints[j][i]=g2->stats.min_val+(g2->grib2.complex_pack.grid_point.pvals[m]+g2->grib2.complex_pack.grid_point.ref_vals[n])*E/D;
                if (g2->m_gridpoints[j][i] > g2->stats.max_val) {
                  g2->stats.max_val=g2->m_gridpoints[j][i];
                }
                g2->stats.avg_val+=g2->m_gridpoints[j][i];
                ++avg_cnt;
              }
              ++gridpoint_index;
            }
          }
          else {
// constant group
            for (int m=0; m < g2->grib2.complex_pack.grid_point.lengths[n]; ++m) {
              auto j=gridpoint_index/g2->dim.x;
              auto i=(gridpoint_index % g2->dim.x);
              if (g2->grib2.complex_pack.grid_point.ref_vals[n] == groups.miss_val || (g2->bitmap.applies && g2->bitmap.map[gridpoint_index] == 0)) {
                g2->m_gridpoints[j][i]=Grid::MISSING_VALUE;
              }
              else {
                g2->m_gridpoints[j][i]=g2->stats.min_val+g2->grib2.complex_pack.grid_point.ref_vals[n]*E/D;
                if (g2->m_gridpoints[j][i] > g2->stats.max_val) {
                  g2->stats.max_val=g2->m_gridpoints[j][i];
                }
                g2->stats.avg_val+=g2->m_gridpoints[j][i];
                ++avg_cnt;
              }
              ++gridpoint_index;
            }
          }
        }
        for (; gridpoint_index < g2->dim.size; ++gridpoint_index) {
          auto j=gridpoint_index/g2->dim.x;
          auto i=(gridpoint_index % g2->dim.x);
          g2->m_gridpoints[j][i]=Grid::MISSING_VALUE;
        }
        g2->stats.avg_val/=static_cast<double>(avg_cnt);
        g2->grid.filled=true;
        break;
      }
      case 3: {
        if (g2->grib.scan_mode != 0) {
          myerror="unable to decode ddef3 for scan mode "+to_string(g2->grib.scan_mode);
          exit(1);
        }
        g2->galloc();
        if (g2->grib2.complex_pack.grid_point.num_groups > 0) {
          if (g2->grib2.complex_pack.grid_point.miss_val_mgmt > 0) {
            groups.miss_val=pow(2.,g2->grib.pack_width)-1;
          }
          else {
            groups.miss_val=Grid::MISSING_VALUE;
          }
          off+=40;
          g2->grib2.complex_pack.grid_point.spatial_diff.first_vals.resize(g2->grib2.complex_pack.grid_point.spatial_diff.order);
          for (int n=0; n < g2->grib2.complex_pack.grid_point.spatial_diff.order; ++n) {
            bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.spatial_diff.first_vals[n],off,g2->grib2.complex_pack.grid_point.spatial_diff.order_vals_width*8);
            off+=g2->grib2.complex_pack.grid_point.spatial_diff.order_vals_width*8;
          }
          bits::get(stream_buffer,groups.sign,off,1);
          bits::get(stream_buffer,groups.omin,off+1,g2->grib2.complex_pack.grid_point.spatial_diff.order_vals_width*8-1);
          if (groups.sign == 1) {
            groups.omin=-groups.omin;
          }
          off+=g2->grib2.complex_pack.grid_point.spatial_diff.order_vals_width*8;
          g2->grib2.complex_pack.grid_point.ref_vals.resize(g2->grib2.complex_pack.grid_point.num_groups);
          if (g2->grib.pack_width > 0) {
            bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.ref_vals.data(),off,g2->grib.pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
          }
          else {
            for (int n=0; n < g2->grib2.complex_pack.grid_point.num_groups; ++n) {
              g2->grib2.complex_pack.grid_point.ref_vals[n]=0;
            }
          }
          off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib.pack_width;
          auto pad=(off % 8);
          if (pad > 0) {
            off+=8-pad;
          }
          g2->grib2.complex_pack.grid_point.widths.resize(g2->grib2.complex_pack.grid_point.num_groups);
          if (g2->grib2.complex_pack.grid_point.width_pack_width > 0) {
            bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.widths.data(),off,g2->grib2.complex_pack.grid_point.width_pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
            for (int n=0; n < g2->grib2.complex_pack.grid_point.num_groups; ++n) {
              g2->grib2.complex_pack.grid_point.widths[n]+=g2->grib2.complex_pack.grid_point.width_ref;
            }
          }
          else {
            for (int n=0; n < g2->grib2.complex_pack.grid_point.num_groups; ++n) {
              g2->grib2.complex_pack.grid_point.widths[n]=g2->grib2.complex_pack.grid_point.width_ref;
            }
          }
          off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib2.complex_pack.grid_point.width_pack_width;
          pad=(off % 8);
          if (pad > 0) {
            off+=8-pad;
          }
          g2->grib2.complex_pack.grid_point.lengths.resize(g2->grib2.complex_pack.grid_point.num_groups);
          bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.lengths.data(),off,g2->grib2.complex_pack.grid_point.length_pack_width,0,g2->grib2.complex_pack.grid_point.num_groups);
          off+=g2->grib2.complex_pack.grid_point.num_groups*g2->grib2.complex_pack.grid_point.length_pack_width;
          pad=(off % 8);
          if (pad > 0) {
            off+=8-pad;
          }
          groups.max_length=0;
          int end=g2->grib2.complex_pack.grid_point.num_groups-1;
          for (int n=0; n < end; ++n) {
            g2->grib2.complex_pack.grid_point.lengths[n]=g2->grib2.complex_pack.grid_point.length_ref+g2->grib2.complex_pack.grid_point.lengths[n]*g2->grib2.complex_pack.grid_point.length_incr;
            if (g2->grib2.complex_pack.grid_point.lengths[n] > groups.max_length) {
              groups.max_length=g2->grib2.complex_pack.grid_point.lengths[n];
            }
          }
          g2->grib2.complex_pack.grid_point.lengths[end]=g2->grib2.complex_pack.grid_point.last_length;
          if (g2->grib2.complex_pack.grid_point.lengths[end] > groups.max_length) {
            groups.max_length=g2->grib2.complex_pack.grid_point.lengths[end];
          }
          g2->grib2.complex_pack.grid_point.pvals.resize(groups.max_length);
// unpack the field of differences
          size_t gridpoint_index=0;
          for (int n=0; n < g2->grib2.complex_pack.grid_point.num_groups; ++n) {
            if (g2->grib2.complex_pack.grid_point.widths[n] > 0) {
              if (g2->grib2.complex_pack.grid_point.miss_val_mgmt > 0) {
                groups.group_miss_val=pow(2.,g2->grib2.complex_pack.grid_point.widths[n])-1;
              }
              else {
                groups.group_miss_val=Grid::MISSING_VALUE;
              }
              bits::get(stream_buffer,g2->grib2.complex_pack.grid_point.pvals.data(),off,g2->grib2.complex_pack.grid_point.widths[n],0,g2->grib2.complex_pack.grid_point.lengths[n]);
              off+=g2->grib2.complex_pack.grid_point.widths[n]*g2->grib2.complex_pack.grid_point.lengths[n];
              for (int m=0; m < g2->grib2.complex_pack.grid_point.lengths[n]; ) {
                auto j=gridpoint_index/g2->dim.x;
                auto i=(gridpoint_index % g2->dim.x);
                if ((g2->bitmap.applies && g2->bitmap.map[gridpoint_index] == 0) || g2->grib2.complex_pack.grid_point.pvals[m] == groups.group_miss_val) {
                  g2->m_gridpoints[j][i]=Grid::MISSING_VALUE;
                  if (g2->grib2.complex_pack.grid_point.miss_val_mgmt > 0) {
                    ++m;
                  }
                }
                else {
                  g2->m_gridpoints[j][i]=g2->grib2.complex_pack.grid_point.pvals[m]+g2->grib2.complex_pack.grid_point.ref_vals[n]+groups.omin;
                  ++m;
                }
                ++gridpoint_index;
              }
            }
            else {
// constant group
              double cval = g2->grib2.complex_pack.grid_point.ref_vals[n] + groups.omin;
              for (int m=0; m < g2->grib2.complex_pack.grid_point.lengths[n]; ) {
                auto j=gridpoint_index/g2->dim.x;
                auto i=(gridpoint_index % g2->dim.x);
                if ((g2->bitmap.applies && g2->bitmap.map[gridpoint_index] == 0) || g2->grib2.complex_pack.grid_point.ref_vals[n] == groups.miss_val) {
                  g2->m_gridpoints[j][i]=Grid::MISSING_VALUE;
                  if (g2->grib2.complex_pack.grid_point.miss_val_mgmt > 0) {
                    ++m;
                  }
                }
                else {
                  g2->m_gridpoints[j][i] = cval;
                  ++m;
                }
                ++gridpoint_index;
              }
            }
          }
          for (; gridpoint_index < g2->dim.size; ++gridpoint_index) {
            auto j=gridpoint_index/g2->dim.x;
            auto i=(gridpoint_index % g2->dim.x);
            g2->m_gridpoints[j][i]=Grid::MISSING_VALUE;
          }
          for (int n=g2->grib2.complex_pack.grid_point.spatial_diff.order-1; n > 0; --n) {
            auto lastgp=g2->grib2.complex_pack.grid_point.spatial_diff.first_vals[n]-g2->grib2.complex_pack.grid_point.spatial_diff.first_vals[n-1];
            int num_not_missing=0;
            for (size_t l=0; l < g2->dim.size; ++l) {
              auto j=l/g2->dim.x;
              auto i=(l % g2->dim.x);
              if (g2->m_gridpoints[j][i] != Grid::MISSING_VALUE) {
                if (num_not_missing >= g2->grib2.complex_pack.grid_point.spatial_diff.order) {
                  g2->m_gridpoints[j][i]+=lastgp;
                  lastgp=g2->m_gridpoints[j][i];
                }
                ++num_not_missing;
              }
            }
          }
          double lastgp=0.;
          int num_not_missing=0;
          auto avg_cnt=0;
          for (size_t l=0; l < g2->dim.size; ++l) {
            auto j=l/g2->dim.x;
            auto i=(l % g2->dim.x);
            if (g2->m_gridpoints[j][i] != Grid::MISSING_VALUE) {
              if (num_not_missing < g2->grib2.complex_pack.grid_point.spatial_diff.order) {
                g2->m_gridpoints[j][i]=g2->stats.min_val+g2->grib2.complex_pack.grid_point.spatial_diff.first_vals[num_not_missing]/D*E;
                lastgp=g2->stats.min_val*D/E+g2->grib2.complex_pack.grid_point.spatial_diff.first_vals[num_not_missing];
              }
              else {
                lastgp+=g2->m_gridpoints[j][i];
                g2->m_gridpoints[j][i]=lastgp*E/D;
              }
              if (g2->m_gridpoints[j][i] > g2->stats.max_val) {
                g2->stats.max_val=g2->m_gridpoints[j][i];
              }
              g2->stats.avg_val+=g2->m_gridpoints[j][i];
              ++avg_cnt;
              ++num_not_missing;
            }
          }
          g2->stats.avg_val/=static_cast<double>(avg_cnt);
        }
        else {
          for (int n=0; n < g2->dim.y; ++n) {
            for (int m=0; m < g2->dim.x; ++m) {
              g2->m_gridpoints[n][m]=Grid::MISSING_VALUE;
            }
          }
          g2->stats.min_val=g2->stats.max_val=g2->stats.avg_val=Grid::MISSING_VALUE;
        }
        g2->grid.filled=true;
        break;
      }
#ifdef __WITH_JASPER
      case 40:
      case 40000: {
        auto len=m_lengths.ds-5;
        g2->galloc();
        decode_jpeg2000(&stream_buffer[curr_off+5],len,jas_matrix,g2,D,E);
        break;
      }
#endif
      default: {
        myerror="unable to decode data section for data representation "+to_string(g2->grib2.data_rep);
        exit(1);
      }
    }
  }
  curr_off+=m_lengths.ds;
}

void GRIB2Message::pack_ds(unsigned char *output_buffer, off_t& offset, Grid
     *grid) const {
  off_t off = offset * 8;
  bits::set(output_buffer, 7, off + 32, 8);
  GRIB2Grid *g2 = reinterpret_cast<GRIB2Grid *>(grid);
  switch (g2->grib2.data_rep) {
    case 0: {
      auto pval=new int[g2->dim.size];
      auto cnt=0, x = 0;
      for (int n = 0; n < g2->dim.y; ++n) {
        for (int m = 0; m < g2->dim.x; ++m) {
          if (!g2->bitmap.applies || g2->bitmap.map[x] == 1) {
            pval[cnt++] = static_cast<int>(lround((g2->m_gridpoints[n][m] - g2->
                stats.min_val) * pow(10., g2->grib.D)) / pow(2., g2->grib.E));
          }
          ++x;
        }
      }
      bits::set(&output_buffer[off / 8 + 5], pval, 0, g2->grib.pack_width, 0,
          cnt);
      int len=(cnt * g2->grib.pack_width + 7) / 8;
      bits::set(output_buffer, len + 5, off, 32);
      offset += len + 5;
      delete[] pval;
      break;
    }
#ifdef __WITH_JASPER
    case 40:
    case 40000: {
      int cps = (g2->grib.pack_width + 7) / 8;
      int jpclen = g2->dim.size * cps;

      // for some unknown reason, encode_jpeg2000 fails if jpclen is too small
      if (jpclen < 100000) {
        jpclen = 100000;
      }
      double d=pow(10.,g2->grib.D),e=pow(2.,g2->grib.E);
      auto bps = cps * 8;
      auto cin = new unsigned char[jpclen];
      auto x = 0, y = 0;
      for (int n = 0; n < g2->dim.y; ++n) {
        for (int m = 0; m < g2->dim.x; ++m) {
          if (!g2->bitmap.applies || g2->bitmap.map[x] == 1) {
            int jval;
            if (g2->m_gridpoints[n][m] == Grid::MISSING_VALUE) {
              jval = 0;
            } else {
              jval=lround((lround(g2->m_gridpoints[n][m] * d) - lround(g2->
                  stats.min_val * d)) / e);
            }
            bits::set(cin, jval, y * bps, bps);
            ++y;
          }
          ++x;
        }
      }
      int pwidth, pheight;
      if (g2->bitmap.applies) {
        pwidth = y;
        pheight = 1;
      }
      else {
        pwidth = g2->dim.x;
        pheight = g2->dim.y;
      }
      int len;
      if (pwidth > 0) {
        int pnbits = g2->grib.pack_width;
        int ltype = 0, ratio = 0, retry = 1;
        auto noff = off / 8 + 5;
        len = encode_jpeg2000(cin, &pwidth, &pheight, &pnbits, &ltype, &ratio,
            &retry, reinterpret_cast<char *>(&output_buffer[noff]), &jpclen);
      }
      else {
        len = 0;
      }
      delete[] cin;
      bits::set(output_buffer, len + 5, off, 32);
      offset += len + 5;
      break;
    }
#endif
    default: {
      throw runtime_error("unable to encode data section for data "
          "representation " + to_string(g2->grib2.data_rep));
    }
  }
}

off_t GRIB2Message::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length)
{
  off_t offset;
  pack_is(output_buffer,offset,grids.front().get());
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copy_to_buffer()";
    exit(1);
  }
  pack_ids(output_buffer,offset,grids.front().get());
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copy_to_buffer()";
    exit(1);
  }
  for (const auto& grid : grids) {
    pack_lus(output_buffer,offset,grid.get());
    if (offset > static_cast<off_t>(buffer_length)) {
      myerror="buffer overflow in copy_to_buffer()";
      exit(1);
    }
    if (gds_included) {
      pack_gds(output_buffer,offset,grid.get());
      if (offset > static_cast<off_t>(buffer_length)) {
        myerror="buffer overflow in copy_to_buffer()";
        exit(1);
      }
    }
    pack_pds(output_buffer,offset,grid.get());
    if (offset > static_cast<off_t>(buffer_length)) {
      myerror="buffer overflow in copy_to_buffer()";
      exit(1);
    }
    pack_drs(output_buffer,offset,grid.get());
    if (offset > static_cast<off_t>(buffer_length)) {
      myerror="buffer overflow in copy_to_buffer()";
      exit(1);
    }
    pack_bms(output_buffer,offset,grid.get());
    if (offset > static_cast<off_t>(buffer_length)) {
      myerror="buffer overflow in copy_to_buffer()";
      exit(1);
    }
    pack_ds(output_buffer,offset,grid.get());
    if (offset > static_cast<off_t>(buffer_length)) {
      myerror="buffer overflow in copy_to_buffer()";
      exit(1);
    }
  }
  pack_end(output_buffer,offset);
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copy_to_buffer()";
    exit(1);
  }
  mlength=offset;
  pack_length(output_buffer);
  return mlength;
}

void GRIB2Message::fill(const unsigned char *stream_buffer, bool
    fill_header_only) {
  if (stream_buffer == nullptr) {
    myerror = "empty file stream";
    exit(1);
  }
  clear_grids();
  unpack_is(stream_buffer);
  if (m_edition != 2) {
    myerror = "can't decode edition " + to_string(m_edition);
    exit(1);
  }
  short last_sec_num = 99;
  int test;
  bits::get(stream_buffer, test, curr_off*8, 32);
  while (test != 0x37373737) {

    // patch for bad END section
    if ( (curr_off+4) == mlength) {
      mywarning = "bad END section repaired";
      test = 0x37373737;
      break;
    }
    int len;
    bits::get(stream_buffer, len, curr_off*8, 32);
    short sec_num;
    bits::get(stream_buffer, sec_num, curr_off*8+32, 8);

    // patch for bad grid
    if (sec_num < 1 || sec_num > 7 || len > mlength) {
      mywarning = "bad grid ignored - offset: " + to_string(curr_off) + ", "
          "sec_num: " + to_string(sec_num) + ", len: " + to_string(len) + ", "
          "length: " + to_string(mlength);
      test = 0x37373737;
      break;
    } else {
      if (sec_num < last_sec_num) {
        auto g2 = new GRIB2Grid;
        if (!grids.empty()) {
          g2->quick_copy(*(reinterpret_cast<GRIB2Grid *>(grids.back().get())));
        }
        bits::get(stream_buffer, discipline, 48, 8);
        g2->grib2.discipline = discipline;
        g2->grid.filled = false;
        grids.emplace_back(g2);
      }
      last_sec_num = sec_num;
      switch (sec_num) {
        case 1: {
          unpack_ids(stream_buffer);
          break;
        }
        case 2: {
          unpack_lus(stream_buffer);
          break;
        }
        case 3: {
          unpack_gds(stream_buffer);
          break;
        }
        case 4: {
          unpack_pds(stream_buffer);
          break;
        }
        case 5: {
          unpack_drs(stream_buffer);
          break;
        }
        case 6: {
          unpack_bms(stream_buffer);
          break;
        }
        case 7: {
          unpack_ds(stream_buffer,fill_header_only);
          break;
        }
      }
    }
    bits::get(stream_buffer, test, curr_off*8, 32);
  }
}

void GRIB2Message::print_header(std::ostream& outs,bool verbose,string path_to_parameter_map) const
{
  int n=0;

  if (verbose) {
    outs << "  GRIB Ed: " << m_edition << "  Length: " << mlength << "  Number of Grids: " << grids.size() << std::endl;
    for (const auto& grid : grids) {
      outs << "  Grid #" << ++n << ":" << std::endl;
      (reinterpret_cast<GRIB2Grid *>(grid.get()))->print_header(outs,verbose,path_to_parameter_map);
    }
  }
  else {
    outs << " Ed=" << m_edition << " NGrds=" << grids.size();
    for (const auto& grid : grids) {
      outs << " Grd=" << ++n;
      (reinterpret_cast<GRIB2Grid *>(grid.get()))->print_header(outs,verbose,path_to_parameter_map);
    }
  }
}

void GRIB2Message::quick_fill(const unsigned char *stream_buffer)
{
  int test,off;
  short sec_num,last_sec_num=99;
  GRIB2Grid *g2=nullptr;
  union {
    float dum;
    int idum;
  };

  if (stream_buffer == nullptr) {
    myerror="empty file stream";
    exit(1);
  }
  clear_grids();
  m_edition=2;
  curr_off=16;
  bits::get(stream_buffer,test,curr_off*8,32);
  while (test != 0x37373737) {
    bits::get(stream_buffer,sec_num,curr_off*8+32,8);
    if (sec_num < last_sec_num) {
      g2=new GRIB2Grid;
      if (grids.size() > 0) {
        g2->quick_copy(*(reinterpret_cast<GRIB2Grid *>(grids.back().get())));
      }
      g2->grid.filled=false;
      grids.emplace_back(g2);
    }
    switch (sec_num) {
      case 1: {
        curr_off+=test;
        break;
      }
      case 2: {
        curr_off+=test;
        break;
      }
      case 3: {
        off=curr_off*8;
        bits::get(stream_buffer,g2->dim.size,off+48,32);
        bits::get(stream_buffer,g2->dim.x,off+256,16);
        bits::get(stream_buffer,g2->dim.y,off+288,16);
        switch (g2->grid.grid_type) {
          case 0:
          case 1:
          case 2:
          case 3:
          case 40:
          case 41:
          case 42:
          case 43: {
            bits::get(stream_buffer,g2->grib.scan_mode,off+568,8);
            break;
          }
          case 10: {
            bits::get(stream_buffer,g2->grib.scan_mode,off+472,8);
            break;
          }
          case 20:
          case 30:
          case 31: {
            bits::get(stream_buffer,g2->grib.scan_mode,off+512,8);
            break;
          }
          default: {
            std::cerr << "Error: quick_fill not implemented for grid type " << g2->grid.grid_type << std::endl;
            exit(1);
          }
        }
        curr_off+=test;
        break;
      }
      case 4: {
        off=curr_off*8;
        bits::get(stream_buffer,g2->grib2.param_cat,off+72,8);
        bits::get(stream_buffer,g2->grid.param,off+80,8);
        curr_off+=test;
        break;
      }
      case 5: {
        unpack_drs(stream_buffer);
        break;
      }
      case 6: {
unpack_bms(stream_buffer);
/*
        off=curr_off*8;
        int ind;
        bits::get(stream_buffer,ind,off+40,8);
        if (ind == 0) {
          bits::get(stream_buffer,m_lengths.bms,off,32);
          if ( (len=(m_lengths.bms-6)*8) > 0) {
            if (len > g2->bitmap.capacity) {
              if (g2->bitmap.map != nullptr) {
                delete[] g2->bitmap.map;
              }
              g2->bitmap.capacity=len;
              g2->bitmap.map=new unsigned char[g2->bitmap.capacity];
            }
            bits::get(stream_buffer,g2->bitmap.map,off+48,1,0,len);
          }
          g2->bitmap.applies=true;
        }
        else if (ind != 255) {
          std::cerr << "Error: bitmap not implemented for quick_fill()" << std::endl;
          exit(1);
        }
        else {
          g2->bitmap.applies=false;
        }
        curr_off+=test;
*/
        break;
      }
      case 7: {
        unpack_ds(stream_buffer,false);
        break;
      }
    }
    last_sec_num=sec_num;
    bits::get(stream_buffer,test,curr_off*8,32);
  }
}

GRIB2Grid& GRIB2Grid::operator=(const GRIB2Grid& source)
{
  GRIBGrid::operator=(static_cast<GRIBGrid>(source));
  grib2=source.grib2;
  return *this;
}

void GRIB2Grid::quick_copy(const GRIB2Grid& source) {
  if (this == &source) {
    return;
  }
  m_reference_date_time = source.m_reference_date_time;
  m_valid_date_time = source.m_valid_date_time;
  dim = source.dim;
  def = source.def;
  stats = source.stats;
  ensdata = source.ensdata;
  grid = source.grid;
  if (source.plist != nullptr) {
    if (plist != nullptr) {
      delete[] plist;
    }
    plist=new int[dim.y];
    for (auto n = 0; n < dim.y; ++n) {
      plist[n] = source.plist[n];
    }
  } else {
    if (plist != nullptr) delete[] plist;
    plist = nullptr;
  }
  if (source.bitmap.applies) {
    resize_bitmap(source.bitmap.capacity);
    for (size_t n = 0; n < dim.size; ++n) {
      bitmap.map[n] = source.bitmap.map[n];
    }
  }
  bitmap.applies = source.bitmap.applies;
  grib = source.grib;
  grib2 = source.grib2;
}

string GRIB2Grid::build_level_search_key() const {
  return to_string(grid.src) + "-" + to_string(grib.sub_center);
}

string GRIB2Grid::first_level_description(xmlutils::LevelMapper& level_mapper) const
{
  return level_mapper.description("WMO_GRIB2",to_string(grid.level1_type),build_level_search_key());
}

string GRIB2Grid::first_level_units(xmlutils::LevelMapper& level_mapper) const
{
  return level_mapper.units("WMO_GRIB2",to_string(grid.level1_type),build_level_search_key());
}

string GRIB2Grid::build_parameter_search_key() const
{
  return to_string(grid.src)+"-"+to_string(grib.sub_center)+"."+to_string(grib.table)+"-"+to_string(grib2.local_table);
}

string GRIB2Grid::parameter_cf_keyword(xmlutils::ParameterMapper& parameter_mapper) const
{
  auto level_type=to_string(grid.level1_type);
  if (grid.level2_type > 0 && grid.level2_type < 255) {
    level_type+="-"+to_string(grid.level2_type);
  }
  return parameter_mapper.cf_keyword("WMO_GRIB2",build_parameter_search_key()+":"+to_string(grib2.discipline)+"."+to_string(grib2.param_cat)+"."+to_string(grid.param),level_type);
}

string GRIB2Grid::parameter_description(xmlutils::ParameterMapper& parameter_mapper) const
{
  return parameter_mapper.description("WMO_GRIB2",build_parameter_search_key()+":"+to_string(grib2.discipline)+"."+to_string(grib2.param_cat)+"."+to_string(grid.param));
}

string GRIB2Grid::parameter_short_name(xmlutils::ParameterMapper& parameter_mapper) const
{
  return parameter_mapper.short_name("WMO_GRIB2",build_parameter_search_key()+":"+to_string(grib2.discipline)+"."+to_string(grib2.param_cat)+"."+to_string(grid.param));
}

string GRIB2Grid::parameter_units(xmlutils::ParameterMapper& parameter_mapper) const
{
  return parameter_mapper.units("WMO_GRIB2",build_parameter_search_key()+":"+to_string(grib2.discipline)+"."+to_string(grib2.param_cat)+"."+to_string(grid.param));
}

void GRIB2Grid::set_data_representation(unsigned short
    data_representation_code) {
  if (data_representation_code == 0 && data_representation_code != grib2.
      data_rep) {
    if (this->grib.D != 0 || this->grib.E >= 0) {
      this->set_scale_and_packing_width(this->grib.D, 32);
    } else {
      this->set_scale_and_packing_width(-this->grib.E, 32);
    }
  }
  grib2.data_rep = data_representation_code;
}

void GRIB2Grid::compute_mean()
{
//  double cnt;

  if (grib2.stat_process_ranges.size() > 0 && grib2.stat_process_ranges[0].type == 1) {
/*
    cnt=(grib2.stat_process_ranges[0].period_length.value+grib2.stat_process_ranges[0].period_time_increment.value)/grib2.stat_process_ranges[0].period_time_increment.value;
    this->divide_by(cnt);
*/
this->divide_by(grid.nmean);
    grib2.stat_process_ranges[0].type=0;
  }
  else {
    myerror="unable to compute the mean";
    exit(1);
  }
}

GRIB2Grid GRIB2Grid::create_subset(double south_latitude, double north_latitude,
    int latitude_stride, double west_longitude, double east_longitude, int
    longitude_stride) const {

  // check for filled grid
  if (!grid.filled) {
    throw runtime_error("can't create subset from unfilled grid");
  }
  if (grib.scan_mode != 0x0) {
    throw runtime_error("can't create subset for scanning mode " +
        to_string(grib.scan_mode));
  }
  GRIB2Grid new_grid = *this; // return value
/*
  new_grid.grid.src = 60;
  new_grid.grib.sub_center = 1;
*/
  if (latitude_stride < 1) {
    latitude_stride = 1;
  }
  if (longitude_stride < 1) {
    longitude_stride = 1;
  }
  switch (def.type) {
    case Grid::Type::latitudeLongitude:
    case Grid::Type::gaussianLatitudeLongitude: {
      auto lon = (def.elongitude - def.slongitude) / (dim.x - 1);

      // special case of -180 to +180
      if (static_cast<int>(west_longitude) == -180 && static_cast<int>(
          east_longitude) == 180) {
        east_longitude -= lon;
      }
      if (def.slongitude >= 0. && def.elongitude >= 0.) {
        if (west_longitude < 0.) {
          west_longitude += 360.;
        }
        if (east_longitude < 0.) {
          east_longitude += 360.;
        }
      }
      int start_x = -1, end_x =- 1;
      int start_y =- 1, end_y =- 1;
      for (int m = 0; m < dim.x; ++m) {
        auto index_longitude = def.slongitude + m * lon;
        if (start_x < 0 && index_longitude >= west_longitude) {
          start_x = m;
          new_grid.def.slongitude = index_longitude;
        }
        if (end_x < 0 && index_longitude > east_longitude) {
          end_x = m - 1;
          new_grid.def.elongitude = index_longitude - lon;
        }
      }
      if (end_x < 0) {
        end_x = dim.x - 1;
      }
      switch (def.type) {
        case Grid::Type::latitudeLongitude: {
          auto lat = (def.slatitude - def.elatitude) / (dim.y - 1);
          for (int n = 0; n < dim.y; ++n) {
            auto index_latitude = def.slatitude - n * lat;
            if (start_y < 0 && index_latitude <= north_latitude) {
              start_y = n;
              new_grid.def.slatitude = index_latitude;
            }
            if (end_y < 0 && index_latitude < south_latitude) {
              end_y = n - 1;
              new_grid.def.elatitude = index_latitude + lat;
            }
          }
          if (end_y < 0) {
            end_y = dim.y - 1;
          }
          break;
        }
        case Grid::Type::gaussianLatitudeLongitude: {
          my::map<Grid::GLatEntry> gaus_lats;
          if (!gridutils::filled_gaussian_latitudes(_path_to_gauslat_lists,
              gaus_lats, def.num_circles, (grib.scan_mode & 0x40) != 0x40)) {
            throw runtime_error("unable to get gaussian latitudes for " +
                to_string(def.num_circles) + " circles from '" +
                _path_to_gauslat_lists + "'");
          }
          Grid::GLatEntry glat_entry;
          gaus_lats.found(def.num_circles, glat_entry);
          size_t n = 0;
          for (; n < def.num_circles * 2; ++n) {
            if (start_y < 0 && glat_entry.lats[n] <= north_latitude) {
              start_y = n;
              new_grid.def.slatitude = glat_entry.lats[n];
            }
            if (end_y < 0 && glat_entry.lats[n] < south_latitude) {
              end_y = n - 1;
              new_grid.def.elatitude = glat_entry.lats[n - 1];
            }
          }
          if (end_y < 0) {
            end_y = n - 1;
          }
          break;
        }
        default: { }
      }
      auto crosses_greenwich=false;
      if (end_x < start_x || (end_x == start_x && west_longitude ==
          -(east_longitude))) {
        crosses_greenwich = true;
        new_grid.dim.x = end_x + (dim.x - start_x) + 1;
        new_grid.def.slongitude -= 360.;
      }
      else {
        new_grid.dim.x = end_x - start_x + 1;
      }
      new_grid.dim.y = end_y - start_y + 1;
      new_grid.dim.size = new_grid.dim.x * new_grid.dim.y;
      new_grid.stats.min_val = Grid::MISSING_VALUE;
      new_grid.stats.max_val = -Grid::MISSING_VALUE;
      auto bitmap_idx = 0;
      new_grid.grid.num_missing = 0;
      for (int n = start_y; n <= end_y; ++n) {
        auto ex = end_x;
        if (crosses_greenwich) {
          ex = dim.x - 1;
        }
        for (int m = start_x; m <= ex; ++m) {
          if (new_grid.bitmap.capacity > 0) {
            new_grid.bitmap.map[bitmap_idx] = bitmap.map[n * dim.x + m];
            if (new_grid.bitmap.map[bitmap_idx] == 0) {
              ++new_grid.grid.num_missing;
            }
            ++bitmap_idx;
          }
          new_grid.m_gridpoints[n - start_y][m - start_x] = m_gridpoints[n][m];
          if (!myequalf(m_gridpoints[n][m], Grid::MISSING_VALUE)) {
            if (m_gridpoints[n][m] < new_grid.stats.min_val) {
              new_grid.stats.min_val = m_gridpoints[n][m];
            }
            if (m_gridpoints[n][m] > new_grid.stats.max_val) {
              new_grid.stats.max_val = m_gridpoints[n][m];
            }
          }
        }
        if (crosses_greenwich) {
          for (int m = 0; m <= end_x; ++m) {
            if (new_grid.bitmap.capacity > 0) {
              new_grid.bitmap.map[bitmap_idx] = bitmap.map[n * dim.x + m];
              if (new_grid.bitmap.map[bitmap_idx] == 0) {
                ++new_grid.grid.num_missing;
              }
              ++bitmap_idx;
            }
            new_grid.m_gridpoints[n - start_y][m + dim.x - start_x] =
                m_gridpoints[n][m];
            if (!myequalf(m_gridpoints[n][m],
                Grid::MISSING_VALUE)) {
              if (m_gridpoints[n][m] < new_grid.stats.min_val) {
                new_grid.stats.min_val = m_gridpoints[n][m];
              }
              if (m_gridpoints[n][m] > new_grid.stats.max_val) {
                new_grid.stats.max_val = m_gridpoints[n][m];
              }
            }
          }
        }
      }
      break;
    }
    default: {
      throw runtime_error("GRIB2Grid::create_subset(): unable to create a "
          "subset for grid type " + to_string(static_cast<int>(def.type)));
    }
  }
  return new_grid;
}

void GRIB2Grid::v_print_header(std::ostream& outs,bool verbose,string path_to_parameter_map) const
{
  size_t m;

  outs.setf(std::ios::fixed);
  outs.precision(2);
  if (verbose) {
    outs << "  Center: " << std::setw(3) << grid.src << "-" << std::setw(2) << grib.sub_center << "  Tables: " << std::setw(3) << grib.table << "." << std::setw(2) << grib2.local_table << "  RefTime: " << m_reference_date_time.to_string() << "  TimeSig: " << grib2.time_sig << "  DRS Template: " << grib2.data_rep << "    Valid Time: ";
    if (grib2.stat_period_end.year() != 0)
      outs << m_reference_date_time.to_string() << " to " << grib2.stat_period_end.to_string();
    else
      outs << m_valid_date_time.to_string();
    outs << "  Dimensions: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  Type: " << std::setw(3) << grid.grid_type;
    switch (grid.grid_type) {
      case 0:
      case 40: {
        outs.precision(6);
        outs << "  LonRange: " << std::setw(11) << def.slongitude << " to " << std::setw(11) << def.elongitude << " by " << std::setw(8) << def.loincrement << "  LatRange: " << std::setw(10) << def.slatitude << " to " << std::setw(10) << def.elatitude;
        switch (grid.grid_type) {
          case 0: {
            outs << " by " << std::setw(8) << def.laincrement;
            break;
          }
          case 40: {
            outs << " Circles: " << std::setw(4) << def.num_circles;
            break;
          }
        }
        outs << "  ScanMode: 0x" << std::hex << grib.scan_mode << std::dec;
        outs.precision(2);
        break;
      }
    }
    outs << "  Param: " << std::setw(3) << grib2.discipline << "/" << std::setw(3) << grib2.param_cat << "/" << std::setw(3) << grid.param;
    switch (grib2.product_type) {
      case 0:
      case 1:
      case 8: {
        outs << "  Generating Process: " << grib.process << "/" << grib2.backgen_process << "/" << grib2.fcstgen_process;
//        if (!myequalf(grid.level2,Grid::MISSING_VALUE))
        if (grid.level2_type != 255) {
          outs << "  Levels: " << grid.level1_type;
          if (!myequalf(grid.level1,Grid::MISSING_VALUE)) {
            outs << "/" << grid.level1;
          }
          outs << "," << grid.level2_type;
          if (!myequalf(grid.level2,Grid::MISSING_VALUE)) {
            outs << "/" << grid.level2;
          }
        }
        else {
          outs << "  Level: " << grid.level1_type;
          if (!myequalf(grid.level1,Grid::MISSING_VALUE))
            outs << "/" << grid.level1;
        }
        switch (grib2.product_type) {
          case 1: {
            outs << "  Ensemble Type/ID/Size: " << ensdata.fcst_type << "/" << ensdata.id << "/" << ensdata.total_size;
            break;
          }
          case 8: {
            outs << "  Outermost Statistical Process: " << grib2.stat_process_ranges[0].type;
            break;
          }
        }
        break;
      }
      default: {
        outs << "  Product: " << grib2.product_type << "?";
      }
    }
    outs << "  Decimal Scale (D): " << std::setw(3) << grib.D << "  Binary Scale (E): " << std::setw(3) << grib.E << "  Pack Width: " << std::setw(2) << grib.pack_width;
    outs << std::endl;
  }
  else {
    outs << "|PDef=" << grib2.product_type << "|Ctr=" << grid.src << "-" << grib.sub_center << "|Tbls=" << grib.table << "." << grib2.local_table << "|RTime=" << grib2.time_sig << ":" << m_reference_date_time.to_string("%Y%m%d%H%MM%SS") << "|Fcst=" << grid.fcst_time << "|GDef=" << grid.grid_type << "," << dim.x << "," << dim.y << ",";
    switch (grid.grid_type) {
      case 0:
      case 40: {
        outs << def.slatitude << "," << def.slongitude << "," << def.elatitude << "," << def.elongitude << "," << def.loincrement << ",";
        if (grid.grid_type == 0) {
          outs << def.laincrement;
        }
        else {
          outs << def.num_circles;
        }
        outs << "," << grib2.earth_shape;
        break;
      }
      case 10: {
        outs << def.slatitude << "," << def.slongitude << "," << def.llatitude << "," << def.olongitude << "," << def.stdparallel1 << "," << def.dx << "," << def.dy << "," << grib2.earth_shape;
        break;
      }
      case 30: {
        outs << def.slatitude << "," << def.slongitude << "," << def.llatitude << "," << def.olongitude << ",";
        if ( (def.projection_flag&0x40) == 0) {
          if ( (def.projection_flag&0x80) == 0x80) {
            outs << "S,";
          }
          else {
            outs << "N,";
          }
        }
        outs << def.stdparallel1 << "," << def.stdparallel2 << "," << grib2.earth_shape;
        break;
      }
    }
    outs << "|VTime=" << m_forecast_date_time.to_string("%Y%m%d%H%MM%SS");
    if (m_valid_date_time != m_forecast_date_time) {
      outs << ":" << m_valid_date_time.to_string("%Y%m%d%H%MM%SS");
    }
    for (m=0; m < grib2.stat_process_ranges.size(); ++m) {
      if (grib2.stat_process_ranges[m].type >= 0) {
        outs << "|SP" << m << "=" << grib2.stat_process_ranges[m].type << "." << grib2.stat_process_ranges[m].time_increment_type << "." << grib2.stat_process_ranges[m].period_length.unit << "." << grib2.stat_process_ranges[m].period_length.value << "." << grib2.stat_process_ranges[m].period_time_increment.unit << "." << grib2.stat_process_ranges[m].period_time_increment.value;
      }
    }
    if (!ensdata.fcst_type.empty()) {
      outs << "|E=" << ensdata.fcst_type << "." << ensdata.id << "." << ensdata.total_size;
      if (grib2.modelv_date.year() != 0) {
        outs << "|Mver=" << grib2.modelv_date.to_string("%Y%m%d%H%MM%SS");
      }
    }
    if (grib2.spatial_process.type >= 0) {
      outs << "|Spatial=" << grib2.spatial_process.stat_process << "," << grib2.spatial_process.type << "," << grib2.spatial_process.num_points;
    }
    outs << "|P=" << grib2.discipline << "." << grib2.param_cat << "." << grid.param << "|L=" << grid.level1_type;
    if (!myequalf(grid.level1,Grid::MISSING_VALUE)) {
      outs << ":" << grid.level1;
    }
    if (grid.level2_type < 255) {
      outs << "," << grid.level2_type;
      if (!myequalf(grid.level2,Grid::MISSING_VALUE)) {
        outs << ":" << grid.level2;
      }
    }
    outs << "|DDef=" << grib2.data_rep;
    if (!myequalf(stats.avg_val,0.) && fabs(stats.avg_val) < 0.01) {
      outs.unsetf(std::ios::fixed);
      outs.setf(std::ios::scientific);
      outs.precision(3);
      outs << "|R=" << stats.min_val;
      if (grid.filled) {
        outs << "|Max=" << stats.max_val << "|Avg=" << stats.avg_val;
      }
      outs.unsetf(std::ios::scientific);
      outs.setf(std::ios::fixed);
      outs.precision(2);
    }
    else {
      outs << "|R=" << stats.min_val;
      if (grid.filled) {
        outs << "|Max=" << stats.max_val << "|Avg=" << stats.avg_val;
      }
    }
    outs << std::endl;
  }
}

void GRIB2Grid::set_statistical_process_ranges(const std::vector<StatisticalProcessRange>& source)
{
  grib2.stat_process_ranges.clear();
  grib2.stat_process_ranges=source;
}

void GRIB2Grid::operator+=(const GRIB2Grid& source) {
  size_t avg_cnt=0,cnt=0;
  StatisticalProcessRange srange;

/*
  grid.src = 60;
  grib.sub_center = 1;
*/
  if (grib.capacity.points == 0) {

    // using a new grid, so set from source and return
    *this = source;
    return;
  }
  if (dim != source.dim) {
    std::cerr << "Warning: unable to perform grid addition on grids with "
        "different dimensions" << std::endl;
    return;
  }

  // add to an existing grid
  resize_bitmap(dim.size);
  grid.num_missing = 0;
  stats.max_val = -Grid::MISSING_VALUE;
  stats.min_val = Grid::MISSING_VALUE;
  stats.avg_val = 0.;
  for (auto n = 0; n < dim.y; ++n) {
    for (auto m = 0; m < dim.x; ++m) {
      auto l = source.grib.scan_mode == grib.scan_mode ? n : dim.y - n - 1;
      if (!myequalf(m_gridpoints[n][m], Grid::MISSING_VALUE) && !myequalf(
          source.m_gridpoints[l][m], Grid::MISSING_VALUE)) {
        m_gridpoints[n][m] += source.m_gridpoints[l][m];
        bitmap.map[cnt++] = 1;
        if (m_gridpoints[n][m] > stats.max_val) {
          stats.max_val = m_gridpoints[n][m];
        }
        if (m_gridpoints[n][m] < stats.min_val) {
          stats.min_val = m_gridpoints[n][m];
        }
        stats.avg_val += m_gridpoints[n][m];
        ++avg_cnt;
      } else {
        m_gridpoints[n][m] = (Grid::MISSING_VALUE);
        bitmap.map[cnt++] = 0;
        ++grid.num_missing;
      }
    }
  }
  bitmap.applies = grid.num_missing > 0 ? true : false;
  if (avg_cnt > 0) {
    stats.avg_val /= static_cast<double>(avg_cnt);
  }
  ++grid.nmean;
  if (grid.nmean == 1) {
    bool can_add=false;
    if (grib2.stat_process_ranges.size() > 0) {
      if (grib2.stat_process_ranges.size() == 1) {
        srange=grib2.stat_process_ranges[0];
        grib2.stat_process_ranges.emplace_back(srange);
        can_add=true;
      }
      else {
        if (source.grid.src == 7) {
          switch (source.grib2.stat_process_ranges[0].type) {
            case 193:
            case 194: {
              if (source.grib2.stat_process_ranges.size() == 2) {
                float x=source.grib2.stat_process_ranges[0].period_length.value/static_cast<float>(dateutils::days_in_month(source.m_reference_date_time.year(),source.m_reference_date_time.month()));
                if (myequalf(x,static_cast<int>(x),0.001)) {
                  srange.type=0;
                  srange.time_increment_type=1;
                  srange.period_length.unit=2;
                  srange.period_length.value=dateutils::days_in_month(source.m_reference_date_time.year(),source.m_reference_date_time.month());
                  srange.period_time_increment.unit=1;
                  srange.period_time_increment.value=source.grib2.stat_process_ranges[0].period_time_increment.value;
                  grib2.stat_process_ranges[1]=srange;
                  can_add=true;
                }
              }
              break;
            }
            case 204: {
              if (source.grib2.stat_process_ranges.size() == 2) {
                float x=source.grib2.stat_process_ranges[0].period_length.value/static_cast<float>(dateutils::days_in_month(source.m_reference_date_time.year(),source.m_reference_date_time.month()));
                if (myequalf(x,static_cast<int>(x),0.001)) {
                  srange.type=0;
                  srange.time_increment_type=1;
                  srange.period_length.unit=2;
                  srange.period_length.value=dateutils::days_in_month(source.m_reference_date_time.year(),source.m_reference_date_time.month());
                  srange.period_time_increment.unit=1;
                  srange.period_time_increment.value=6;
                  grib2.stat_process_ranges[1]=srange;
                  srange.type=1;
                  srange.time_increment_type=2;
                  srange.period_length.unit=1;
                  srange.period_length.value=source.grib2.stat_process_ranges[0].period_time_increment.value;
                  srange.period_time_increment.unit=255;
                  srange.period_time_increment.value=0;
                  grib2.stat_process_ranges.emplace_back(srange);
                  can_add=true;
                }
              }
              break;
            }
          }
        }
      }
    }
    else {
      grib2.stat_process_ranges.resize(1);
      can_add=true;
    }
    if (!can_add) {
      myerror="unable to combine grids with more than one statistical process";
      exit(1);
    }
    srange.type=1;
    srange.time_increment_type=1;
    srange.period_length.value=0;
    if (m_reference_date_time.year() != source.m_reference_date_time.year() && m_reference_date_time.month() == source.m_reference_date_time.month() && m_reference_date_time.day() == source.m_reference_date_time.day() && m_reference_date_time.time() == source.m_reference_date_time.time()) {
      srange.period_length.unit=4;
      srange.period_time_increment.unit=4;
      srange.period_time_increment.value=source.m_reference_date_time.years_since(m_reference_date_time);
    }
    else {
      srange.period_length.unit=1;
      srange.period_time_increment.unit=1;
      srange.period_time_increment.value=source.m_reference_date_time.hours_since(m_reference_date_time);
    }
    grib2.stat_process_ranges[0]=srange;
  }
  if (m_reference_date_time.year() != source.m_reference_date_time.year() &&
      m_reference_date_time.month() == source.m_reference_date_time.month() &&
      m_reference_date_time.day() == source.m_reference_date_time.day() &&
      m_reference_date_time.time() == source.m_reference_date_time.time()) {
    grib2.stat_process_ranges[0].period_length.value = source.
        m_reference_date_time.years_since(m_reference_date_time) + 1;
    grib2.stat_period_end = source.grib2.stat_period_end;
  } else {
    grib2.stat_process_ranges[0].period_length.value = source.
        m_valid_date_time.hours_since(m_reference_date_time);
    grib2.stat_period_end = source.m_valid_date_time;
  }
  grib2.product_type = 8;
}

#ifdef __WITH_JASPER
void decode_jpeg2000(const unsigned char *jpc_bitstream,size_t jpc_bitstream_length,GRIB2Message::JasMatrix& jas_matrix,GRIB2Grid *g2,double D,double E)
{
  jas_image_t *jas_image=nullptr;
  if (jpc_bitstream_length > 0) {
// create a jas_stream_t
    auto jas_stream=jas_stream_memopen(reinterpret_cast<char *>(const_cast<unsigned char *>(jpc_bitstream)),jpc_bitstream_length);
// decode the image
    char *image_opts=nullptr;
    jas_image=jpc_decode(jas_stream,image_opts);
//jas_image=jas_image_decode(jas_stream,4,image_opts);
    jas_stream_close(jas_stream);
  }
// if image failed to decode, or it's not a grayscale, fill gridpoint array
//   with the reference value
  if (jas_image == nullptr || jas_image->numcmpts_ != 1) {
    size_t x=0;
    auto avg_cnt=0;
    for (int n=0; n < g2->dim.y; ++n) {
      for (int m=0; m < g2->dim.x; ++m) {
        if (!g2->bitmap.applies || g2->bitmap.map[x] == 1) {
            g2->m_gridpoints[n][m]=g2->stats.min_val;
            if (g2->m_gridpoints[n][m] > g2->stats.max_val) {
              g2->stats.max_val=g2->m_gridpoints[n][m];
            }
            g2->stats.avg_val+=g2->m_gridpoints[n][m];
            ++avg_cnt;
        }
        else {
            g2->m_gridpoints[n][m]=Grid::MISSING_VALUE;
        }
        ++x;
      }
    }
    g2->stats.avg_val/=static_cast<double>(avg_cnt);
    g2->grid.filled=true;
    return;
  }
// reallocate the data matrix, if needed
  auto h=jas_image_height(jas_image);
  auto w=jas_image_width(jas_image);
  if (h != jas_matrix.height || w != jas_matrix.width) {
    if (jas_matrix.data != nullptr) {
      jas_matrix_destroy(jas_matrix.data);
    }
    jas_matrix.data=jas_matrix_create(h,w);
    jas_matrix.height=h;
    jas_matrix.width=w;
  }
// read the only image component into the data matrix
  jas_image_readcmpt(jas_image,0,0,0,jas_matrix.width,jas_matrix.height,jas_matrix.data);
// fill the gridpoint array
  size_t x=0,y=0;
  auto avg_cnt=0;
  for (int n=0; n < g2->dim.y; ++n) {
    for (int m=0; m < g2->dim.x; ++m) {
      if (!g2->bitmap.applies || g2->bitmap.map[x] == 1) {
          g2->m_gridpoints[n][m]=g2->stats.min_val+jas_matrix.data->data_[y++]*E/D;
          if (g2->m_gridpoints[n][m] > g2->stats.max_val) {
            g2->stats.max_val=g2->m_gridpoints[n][m];
          }
          g2->stats.avg_val+=g2->m_gridpoints[n][m];
          ++avg_cnt;
      }
      else {
          g2->m_gridpoints[n][m]=Grid::MISSING_VALUE;
      }
      ++x;
    }
  }
// clean up
  jas_image_destroy(jas_image);
  g2->stats.avg_val/=static_cast<double>(avg_cnt);
  g2->grid.filled=true;
}

int encode_jpeg2000(unsigned char *cin,int *pwidth,int *pheight,int *pnbits,
                 int *ltype, int *ratio, int *retry, char *outjpc, 
                 int *jpclen)
/*$$$  SUBPROGRAM DOCUMENTATION BLOCK
*                .      .    .                                       .
* SUBPROGRAM:    encode_jpeg2000      Encodes JPEG2000 code stream
*   PRGMMR: Gilbert          ORG: W/NP11     DATE: 2002-12-02
*
* ABSTRACT: This Function encodes a grayscale image into a JPEG2000 code stream
*   specified in the JPEG2000 Part-1 standard (i.e., ISO/IEC 15444-1) 
*   using JasPer Software version 1.500.4 (or 1.700.2 ) written by the 
*   University of British Columbia, Image Power Inc, and others.
*   JasPer is available at http://www.ece.uvic.ca/~mdadams/jasper/.
*
* PROGRAM HISTORY LOG:
* 2002-12-02  Gilbert
* 2004-07-20  GIlbert - Added retry argument/option to allow option of
*                       increasing the maximum number of guard bits to the
*                       JPEG2000 algorithm.
*
* USAGE:    int encode_jpeg2000(unsigned char *cin,g2int *pwidth,g2int *pheight,
*                            g2int *pnbits, g2int *ltype, g2int *ratio, 
*                            g2int *retry, char *outjpc, g2int *jpclen)
*
*   INPUT ARGUMENTS:
*      cin   - Packed matrix of Grayscale image values to encode.
*    pwidth  - Pointer to width of image
*    pheight - Pointer to height of image
*    pnbits  - Pointer to depth (in bits) of image.  i.e number of bits
*              used to hold each data value
*    ltype   - Pointer to indicator of lossless or lossy compression
*              = 1, for lossy compression
*              != 1, for lossless compression
*    ratio   - Pointer to target compression ratio.  (ratio:1)
*              Used only when *ltype == 1.
*    retry   - Pointer to option type.
*              1 = try increasing number of guard bits
*              otherwise, no additional options
*    jpclen  - Number of bytes allocated for new JPEG2000 code stream in
*              outjpc.
*
*   INPUT ARGUMENTS:
*     outjpc - Output encoded JPEG2000 code stream
*
*   RETURN VALUES :
*        > 0 = Length in bytes of encoded JPEG2000 code stream
*         -3 = Error decode jpeg2000 code stream.
*         -5 = decoded image had multiple color components.
*              Only grayscale is expected.
*
* REMARKS:
*
*      Requires JasPer Software version 1.500.4 or 1.700.2
*
* ATTRIBUTES:
*   LANGUAGE: C
*   MACHINE:  IBM SP
*
*$$$*/
{
    int ier,rwcnt;
    jas_image_t image;
    jas_stream_t *jpcstream,*istream;
    jas_image_cmpt_t cmpt,*pcmpt;
    const int MAXOPTSSIZE=1024;
    char opts[MAXOPTSSIZE];
    int width,height,nbits;

    width=*pwidth;
    height=*pheight;
    nbits=*pnbits;
// Set lossy compression options, if requested.
    if ( *ltype != 1 ) {
       opts[0]='\0';
    }
    else {
       snprintf(opts,MAXOPTSSIZE,"mode=real\nrate=%f",1.0/(*ratio));
    }
    if ( *retry == 1 ) {             // option to increase number of guard bits
       strcat(opts,"\nnumgbits=4");
    }
    //printf("SAGopts: %s\n",opts);
// Initialize the JasPer image structure describing the grayscale image to
// encode into the JPEG2000 code stream.
    image.tlx_=0;
    image.tly_=0;
    image.brx_=static_cast<jas_image_coord_t>(width);
    image.bry_=static_cast<jas_image_coord_t>(height);
    image.numcmpts_=1;
    image.maxcmpts_=1;
    image.clrspc_=JAS_CLRSPC_SGRAY;
    image.cmprof_=0; 
    image.inmem_=1;
    cmpt.tlx_=0;
    cmpt.tly_=0;
    cmpt.hstep_=1;
    cmpt.vstep_=1;
    cmpt.width_=static_cast<jas_image_coord_t>(width);
    cmpt.height_=static_cast<jas_image_coord_t>(height);
    cmpt.type_=JAS_IMAGE_CT_COLOR(JAS_CLRSPC_CHANIND_GRAY_Y);
    cmpt.prec_=nbits;
    cmpt.sgnd_=0;
    cmpt.cps_=(nbits+7)/8;
    pcmpt=&cmpt;
    image.cmpts_=&pcmpt;
// Open a JasPer stream containing the input grayscale values
    istream=jas_stream_memopen(reinterpret_cast<char *>(cin),height*width*cmpt.cps_);
    cmpt.stream_=istream;
// Open an output stream that will contain the encoded jpeg2000 code stream.
    jpcstream=jas_stream_memopen(outjpc,static_cast<int>(*jpclen));
// Encode image.
    ier=jpc_encode(&image,jpcstream,opts);
    if ( ier != 0 ) {
       fprintf(stderr," jpc_encode return = %d \n",ier);
       return -3;
    }
// Clean up JasPer work structures.
    rwcnt=jpcstream->rwcnt_;
    ier=jas_stream_close(istream);
    ier=jas_stream_close(jpcstream);
// Return size of jpeg2000 code stream
    return (rwcnt);
}
#endif

#endif
