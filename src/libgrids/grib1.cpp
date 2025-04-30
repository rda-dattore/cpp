#include <iomanip>
#include <grid.hpp>
#include <gridutils.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>
#include <tempfile.hpp>
#include <myerror.hpp>

using std::runtime_error;
using std::string;
using std::to_string;

#ifndef __cosutils

GRIBMessage& GRIBMessage::operator=(const GRIBMessage& source)
{
  m_edition=source.m_edition;
  discipline=source.discipline;
  mlength=source.mlength;
  curr_off=source.curr_off;
  m_offsets=source.m_offsets;
  m_lengths=source.m_lengths;
  pds_supp.reset(new unsigned char[m_lengths.pds_supp]);
    for (int n=0; n < m_lengths.pds_supp; ++n) {
      pds_supp[n]=source.pds_supp[n];
    }
  gds_included=source.gds_included;
  bms_included=source.bms_included;
  grids=source.grids;
  return *this;
}

void GRIBMessage::append_grid(const Grid *grid)
{
  Grid *g=nullptr;
  switch (m_edition) {
    case 0:
    case 1: {
      g=new GRIBGrid;
      *(reinterpret_cast<GRIBGrid *>(g))=*(reinterpret_cast<GRIBGrid *>(const_cast<Grid *>(grid)));
      if (reinterpret_cast<GRIBGrid *>(g)->bitmap.applies) {
        bms_included=true;
      }
      break;
    }
    case 2: {
      g=new GRIB2Grid;
      *(reinterpret_cast<GRIB2Grid *>(g))=*(reinterpret_cast<GRIB2Grid *>(const_cast<Grid *>(grid)));
      break;
     }
  }
  grids.emplace_back(g);
}

void GRIBMessage::convert_to_grib1()
{
  if (grids.size() > 1) {
    myerror="unable to convert multiple grids to GRIB1";
    exit(1);
  }
  if (grids.size() == 0) {
    myerror="no grid found";
    exit(1);
  }
  auto g=reinterpret_cast<GRIBGrid *>(grids.front().get());
  switch (m_edition) {
    case 0: {
      m_lengths.is=8;
      m_lengths.pds=28;
      m_edition=1;
      g->grib.table=3;
      if (g->m_reference_date_time.year() == 0 || g->m_reference_date_time.year() > 30) {
        g->grib.century=20;
        g->m_reference_date_time.set_year(g->m_reference_date_time.year()+1900);
      }
      else {
        g->grib.century=21;
        g->m_reference_date_time.set_year(g->m_reference_date_time.year()+2000);
      }
      g->grib.sub_center=0;
      g->grib.D=0;
      g->remap_parameters();
      break;
    }
  }
}

void GRIBMessage::pack_length(unsigned char *output_buffer) const
{
  switch (m_edition) {
    case 1: {
      bits::set(output_buffer,mlength,32,24);
      break;
    }
    case 2: {
      bits::set(output_buffer,mlength,64,64);
      break;
    }
  }
}

void GRIBMessage::pack_is(unsigned char *output_buffer,off_t& offset,Grid *g) const
{
  bits::set(output_buffer,0x47524942,0,32);
  switch (m_edition) {
    case 1: {
      bits::set(output_buffer,m_edition,56,8);
      offset=8;
      break;
    }
    case 2: {
      bits::set(output_buffer,0,32,16);
      bits::set(output_buffer,(reinterpret_cast<GRIB2Grid *>(g))->discipline(),48,8);
      bits::set(output_buffer,m_edition,56,8);
      offset=16;
      break;
    }
    default: {
      myerror="unable to encode GRIB "+strutils::itos(m_edition);
      exit(1);
    }
  }
}

void GRIBMessage::pack_pds(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  unsigned char flag=0x0;
  short hr,min;
  size_t dum;
  GRIBGrid *g;

  g=reinterpret_cast<GRIBGrid *>(grid);
  if (m_lengths.pds_supp > 0) {
    bits::set(output_buffer,41+m_lengths.pds_supp,off,24);
    offset+=(41+m_lengths.pds_supp);
  }
  else {
    bits::set(output_buffer,28,off,24);
    if ((g->m_reference_date_time.year() % 100) != 0) {
      bits::set(output_buffer,g->m_reference_date_time.year() % 100,off+96,8);
    }
    else {
      bits::set(output_buffer,100,off+96,8);
    }
    offset+=28;
  }
  if (m_edition == 0) {
    bits::set(output_buffer,m_edition,off+24,8);
    bits::set(output_buffer,g->m_reference_date_time.year(),off+96,8);
  }
  else {
    bits::set(output_buffer,g->grib.table,off+24,8);
  }
  bits::set(output_buffer,g->grid.src,off+32,8);
  bits::set(output_buffer,g->grib.process,off+40,8);
  bits::set(output_buffer,g->grib.grid_catalog_id,off+48,8);
  if (gds_included) {
    flag|=0x80;
  }
  if (bms_included) {
    flag|=0x40;
  }
  bits::set(output_buffer,flag,off+56,8);
  bits::set(output_buffer,g->grid.param,off+64,8);
  bits::set(output_buffer,g->grid.level1_type,off+72,8);
  switch (g->grid.level1_type) {
    case 101: {
      dum=g->grid.level1/10.;
      bits::set(output_buffer,dum,off+80,8);
      dum=g->grid.level2/10.;
      bits::set(output_buffer,dum,off+88,8);
      break;
    }
    case 107:
    case 119: {
      dum=g->grid.level1*10000.;
      bits::set(output_buffer,dum,off+80,16);
      break;
    }
    case 108: {
      dum=g->grid.level1*100.;
      bits::set(output_buffer,dum,off+80,8);
      dum=g->grid.level2*100.;
      bits::set(output_buffer,dum,off+88,8);
      break;
    }
    case 114: {
      dum=475.-(g->grid.level1);
      bits::set(output_buffer,dum,off+80,8);
      dum=475.-(g->grid.level2);
      bits::set(output_buffer,dum,off+88,8);
      break;
    }
    case 128: {
      dum=(1.1-(g->grid.level1))*1000.;
      bits::set(output_buffer,dum,off+80,8);
      dum=(1.1-(g->grid.level2))*1000.;
      bits::set(output_buffer,dum,off+88,8);
      break;
    }
    case 141: {
      dum=g->grid.level1/10.;
      bits::set(output_buffer,dum,off+80,8);
      dum=1100.-(g->grid.level2);
      bits::set(output_buffer,dum,off+88,8);
      break;
    }
    case 104:
    case 106:
    case 110:
    case 112:
    case 116:
    case 120:
    case 121: {
      dum=g->grid.level1;
      bits::set(output_buffer,dum,off+80,8);
      dum=g->grid.level2;
      bits::set(output_buffer,dum,off+88,8);
      break;
    }
    default: {
      dum=g->grid.level1;
      bits::set(output_buffer,dum,off+80,16);
    }
  }
  bits::set(output_buffer,g->m_reference_date_time.month(),off+104,8);
  bits::set(output_buffer,g->m_reference_date_time.day(),off+112,8);
  hr=g->m_reference_date_time.time()/10000;
  min=(g->m_reference_date_time.time() % 10000)/100;
  bits::set(output_buffer,hr,off+120,8);
  bits::set(output_buffer,min,off+128,8);
  bits::set(output_buffer,g->grib.time_unit,off+136,8);
  if (g->grib.t_range != 10) {
    bits::set(output_buffer,g->grib.p1,off+144,8);
    bits::set(output_buffer,g->grib.p2,off+152,8);
  }
  else {
    bits::set(output_buffer,g->grib.p1,off+144,16);
  }
  bits::set(output_buffer,g->grib.t_range,off+160,8);
  bits::set(output_buffer,g->grid.nmean,off+168,16);
  bits::set(output_buffer,g->grib.nmean_missing,off+184,8);
  if (m_edition == 1) {
    if ((g->m_reference_date_time.year() % 100) != 0) {
      bits::set(output_buffer,g->m_reference_date_time.year()/100+1,off+192,8);
    }
    else {
      bits::set(output_buffer,g->m_reference_date_time.year()/100,off+192,8);
    }
    bits::set(output_buffer,g->grib.sub_center,off+200,8);
    if (g->grib.D < 0) {
      bits::set(output_buffer,-(g->grib.D)+0x8000,off+208,16);
    }
    else {
      bits::set(output_buffer,g->grib.D,off+208,16);
    }
  }
// pack the PDS supplement, if it exists
  if (m_lengths.pds_supp > 0) {
    bits::set(output_buffer,pds_supp.get(),off+320,8,0,m_lengths.pds_supp);
  }
}

void GRIBMessage::pack_gds(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  short sign;
  int dum;
  GRIBGrid *g;

  g=reinterpret_cast<GRIBGrid *>(grid);
bits::set(output_buffer,255,off+24,8);
bits::set(output_buffer,255,off+32,8);
  bits::set(output_buffer,g->grid.grid_type,off+40,8);
  switch (g->grid.grid_type) {
    case 0:
    case 4: {
// Latitude/Longitude
// Gaussian Lat/Lon
      bits::set(output_buffer,g->dim.x,off+48,16);
      bits::set(output_buffer,g->dim.y,off+64,16);
      dum=lround(g->def.slatitude*1000.);
      if (dum < 0) {
        sign=1;
        dum=-dum;
      }
      else
        sign=0;
      bits::set(output_buffer,sign,off+80,1);
      bits::set(output_buffer,dum,off+81,23);
      dum=lround(g->def.slongitude*1000.);
      if (dum < 0) {
        sign=1;
        dum=-dum;
      }
      else
        sign=0;
      bits::set(output_buffer,sign,off+104,1);
      bits::set(output_buffer,dum,off+105,23);
      bits::set(output_buffer,g->grib.rescomp,off+128,8);
      dum=lround(g->def.elatitude*1000.);
      if (dum < 0) {
        sign=1;
        dum=-dum;
      }
      else {
        sign=0;
      }
      bits::set(output_buffer,sign,off+136,1);
      bits::set(output_buffer,dum,off+137,23);
      dum=lround(g->def.elongitude*1000.);
      if (dum < 0) {
        sign=1;
        dum=-dum;
      }
      else
        sign=0;
      bits::set(output_buffer,sign,off+160,1);
      bits::set(output_buffer,dum,off+161,23);
      if (floatutils::myequalf(g->def.loincrement,-99.999)) {
        dum=0xffff;
      }
      else {
        dum=lround(g->def.loincrement*1000.);
      }
      bits::set(output_buffer,dum,off+184,16);
      if (g->grid.grid_type == 0) {
        if (floatutils::myequalf(g->def.laincrement,-99.999)) {
          dum=0xffff;
        }
        else {
          dum=lround(g->def.laincrement*1000.);
        }
      }
      else {
        if (g->def.num_circles == 0) {
          dum=0xffff;
        }
        else {
          dum=g->def.num_circles;
        }
      }
      bits::set(output_buffer,dum,off+200,16);
      bits::set(output_buffer,g->grib.scan_mode,off+216,8);
      bits::set(output_buffer,0,off+224,32);
      bits::set(output_buffer,32,off,24);
      offset+=32;
      break;
    }
    case 3:
    case 5: {
// Lambert Conformal
// Polar Stereographic
      bits::set(output_buffer,g->dim.x,off+48,16);
      bits::set(output_buffer,g->dim.y,off+64,16);
      dum=lround(g->def.slatitude*1000.);
      if (dum < 0) {
        sign=1;
        dum=-dum;
      }
      else
        sign=0;
      bits::set(output_buffer,sign,off+80,1);
      bits::set(output_buffer,dum,off+81,23);
      dum=lround(g->def.slongitude*1000.);
      if (dum < 0) {
        sign=1;
        dum=-dum;
      }
      else
        sign=0;
      bits::set(output_buffer,sign,off+104,1);
      bits::set(output_buffer,dum,off+105,23);
      bits::set(output_buffer,g->grib.rescomp,off+128,8);
      dum=lround(g->def.olongitude*1000.);
      if (dum < 0) {
        sign=1;
        dum=-dum;
      }
      else
        sign=0;
      bits::set(output_buffer,sign,off+136,1);
      bits::set(output_buffer,dum,off+137,23);
      dum=lround(g->def.dx*1000.);
      bits::set(output_buffer,dum,off+160,24);
      dum=lround(g->def.dy*1000.);
      bits::set(output_buffer,dum,off+184,24);
      bits::set(output_buffer,g->def.projection_flag,off+208,8);
      bits::set(output_buffer,g->grib.scan_mode,off+216,8);
      if (g->grid.grid_type == 3) {
        dum=lround(g->def.stdparallel1*1000.);
        if (dum < 0) {
          sign=1;
          dum=-dum;
        }
        else {
          sign=0;
        }
        bits::set(output_buffer,sign,off+224,1);
        bits::set(output_buffer,dum,off+225,23);
        dum=lround(g->def.stdparallel2*1000.);
        if (dum < 0) {
          sign=1;
          dum=-dum;
        }
        else {
          sign=0;
        }
        bits::set(output_buffer,sign,off+248,1);
        bits::set(output_buffer,dum,off+249,23);
        dum=lround(g->grib.sp_lat*1000.);
        if (dum < 0) {
            sign=1;
            dum=-dum;
        }
        else
            sign=0;
        bits::set(output_buffer,sign,off+272,1);
        bits::set(output_buffer,dum,off+273,23);
        dum=lround(g->grib.sp_lon*1000.);
        if (dum < 0) {
            sign=1;
            dum=-dum;
        }
        else
            sign=0;
        bits::set(output_buffer,sign,off+296,1);
        bits::set(output_buffer,dum,off+297,23);
        bits::set(output_buffer,40,off,24);
        offset+=40;
      }
      else {
        bits::set(output_buffer,0,off+224,32);
        bits::set(output_buffer,32,off,24);
        offset+=32;
      }
      break;
    }
  }
}

void GRIBMessage::pack_bms(unsigned char *output_buffer, off_t& offset, Grid
    *grid) const {
  off_t off=offset*8;
  int ub;
  GRIBGrid *g;
  GRIB2Grid *g2;

  switch (m_edition) {
    case 0:
    case 1: {
      g=reinterpret_cast<GRIBGrid *>(grid);
      ub=8-(g->dim.size % 8);
      bits::set(output_buffer,ub,off+24,8);
      bits::set(output_buffer,0,off+32,16);
      bits::set(output_buffer,g->bitmap.map,off+48,1,0,g->dim.size);
      bits::set(output_buffer,(g->dim.size+7)/8+6,off,24);
      offset+=(g->dim.size+7)/8+6;
      break;
    }
    case 2: {
      g2=reinterpret_cast<GRIB2Grid *>(grid);
      bits::set(output_buffer,6,off+32,8);
      if (g2->bitmap.applies) {
bits::set(output_buffer,0,off+40,8);
        bits::set(output_buffer,g2->bitmap.map,off+48,1,0,g2->dim.size);
        bits::set(output_buffer,(g2->dim.size+7)/8+6,off,32);
        offset+=(g2->dim.size+7)/8;
      }
      else {
        bits::set(output_buffer,255,off+40,8);
        bits::set(output_buffer,6,off,32);
      }
      offset+=6;
      break;
    }
  }
}

void GRIBMessage::pack_bds(unsigned char *output_buffer,off_t& offset,Grid *grid) const
{
  off_t off=offset*8;
  double d,e,range;
  int E=0,n,m,len;
  int cnt=0,num_packed=0,*packed,max_pack;
  short ub,flag;
  GRIBGrid *g=reinterpret_cast<GRIBGrid *>(grid);

  if (!g->grid.filled) {
    myerror="empty grid";
    exit(1);
  }
  d=pow(10.,g->grib.D);
  range=(g->stats.max_val-g->stats.min_val)*d;
  if (range > 0.) {
    max_pack=pow(2.,g->grib.pack_width)-1;
    if (!floatutils::myequalf(range,0.)) {
      while (lround(range) <= max_pack) {
        range*=2.;
        E--;
      }
      while (lround(range) > max_pack) {
        range*=0.5;
        E++;
      }
    }
    num_packed=g->dim.size-(g->grid.num_missing);
    len=num_packed*g->grib.pack_width;
    bits::set(output_buffer,(len+7)/8+11,off,24);
    if ( (ub=(len % 8)) > 0) {
      ub=8-ub;
    }
    bits::set(output_buffer,ub,off+28,4);
    bits::set(output_buffer,0,off+88+len,ub);
    offset+=(len+7)/8+11;
  } else {
    bits::set(output_buffer,11,off,24);
    g->grib.pack_width=0;
    offset+=11;
  }
  bits::set(output_buffer,g->grib.pack_width,off+80,8);
  flag=0x0;
  bits::set(output_buffer,flag,off+24,4);
  auto ibm_rep=floatutils::ibmconv(g->stats.min_val*d);
  bits::set(output_buffer,abs(E),off+32,16);
  if (E < 0) {
    bits::set(output_buffer,1,off+32,1);
  }
  bits::set(output_buffer,ibm_rep,off+48,32);
  if (g->grib.pack_width == 0) {
    return;
  }
  e=pow(2.,E);
  if ((flag & 0x4) == 0) {
// simple packing
    packed=new int[num_packed];
    cnt=0;
    if (g->grid.src == 7 && g->grib.grid_catalog_id != 255) {
      switch (g->grib.grid_catalog_id) {
        case 23:
        case 24:
        case 26:
        case 63:
        case 64: {
//          packed[cnt]=lroundf(((double)g->grid.pole-(double)g->stats.min_val)*d/e);
packed[cnt]=lround((lround(g->grid.pole*d)-lround(g->stats.min_val*d))/e);
          ++cnt;
          break;
        }
      }
    }
    switch (g->grid.grid_type) {
      case 0:
      case 3:
      case 4: {
        for (n=0; n < g->dim.y; ++n) {
          for (m=0; m < g->dim.x; ++m) {
            if (!floatutils::myequalf(g->m_gridpoints[n][m],Grid::MISSING_VALUE)) {
//              packed[cnt]=lroundf(((double)g->m_gridpoints[n][m]-(double)g->stats.min_val)*d/e);
packed[cnt]=lround((lround(g->m_gridpoints[n][m]*d)-lround(g->stats.min_val*d))/e);
              ++cnt;
            }
          }
        }
        if (g->grid.src == 7 && g->grib.grid_catalog_id != 255) {
          switch (g->grib.grid_catalog_id) {
            case 21:
            case 22:
            case 25:
            case 61:
            case 62: {
//              packed[cnt]=lroundf(((double)g->grid.pole-(double)g->stats.min_val)*d/e);
packed[cnt]=lround((lround(g->grid.pole*d)-lround(g->stats.min_val*d))/e);
              break;
            }
          }
        }
        break;
      }
      case 5: {
        for (n=0; n < g->dim.y; ++n) {
          for (m=0; m < g->dim.x; ++m) {
            if (!floatutils::myequalf(g->m_gridpoints[n][m],Grid::MISSING_VALUE)) {
//              packed[cnt]=lroundf(((double)g->m_gridpoints[n][m]-(double)g->stats.min_val)*d/e);
packed[cnt]=lround((lround(g->m_gridpoints[n][m]*d)-lround(g->stats.min_val*d))/e);
              ++cnt;
            }
          }
        }
        break;
      }
      default: {
        std::cerr << "Warning: pack_bds does not recognize grid type " << g->grid.grid_type << std::endl;
      }
    }
    if (cnt != num_packed) {
      mywarning="pack_bds: specified number of gridpoints "+strutils::itos(num_packed)+" does not agree with actual number packed "+strutils::itos(cnt)+"  date: "+g->m_reference_date_time.to_string()+" "+strutils::itos(g->grid.param);
    }
    bits::set(output_buffer,packed,off+88,g->grib.pack_width,0,num_packed);
  }
  else {
// second-order packing
    myerror="complex packing not defined";
    exit(1);
  }
  if (num_packed > 0) {
    delete[] packed;
  }
}

void GRIBMessage::pack_end(unsigned char *output_buffer,off_t& offset) const
{
  bits::set(output_buffer,0x37373737,offset*8,32);
  offset+=4;
}

off_t GRIBMessage::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length)
{
  off_t offset;

  pack_is(output_buffer,offset,grids.front().get());
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copy_to_buffer()";
    exit(1);
  }
  pack_pds(output_buffer,offset,grids.front().get());
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copy_to_buffer()";
    exit(1);
  }
  if (gds_included) {
    pack_gds(output_buffer,offset,grids.front().get());
    if (offset > static_cast<off_t>(buffer_length)) {
      myerror="buffer overflow in copy_to_buffer()";
      exit(1);
    }
  }
  if (bms_included) {
    pack_bms(output_buffer,offset,grids.front().get());
  }
  pack_bds(output_buffer,offset,grids.front().get());
  if (offset > static_cast<off_t>(buffer_length)) {
    myerror="buffer overflow in copy_to_buffer()";
    exit(1);
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

void GRIBMessage::clear_grids()
{
  for (auto& grid : grids) {
    grid.reset();
  }
  grids.clear();
}

void GRIBMessage::unpack_is(const unsigned char *stream_buffer)
{
  int test;

  bits::get(stream_buffer,test,0,32);
  if (test != 0x47524942) {
    myerror="not a GRIB message";
    exit(1);
  }
  bits::get(stream_buffer,m_edition,56,8);
  switch (m_edition) {
    case 0: {
      mlength=m_lengths.is=m_offsets.is=curr_off=4;
      break;
    }
    case 1: {
      bits::get(stream_buffer,mlength,32,24);
      m_lengths.is=m_offsets.is=curr_off=8;
      break;
    }
    case 2: {
      bits::get(stream_buffer,mlength,64,64);
      m_lengths.is=m_offsets.is=curr_off=16;
      break;
    }
  }
}

void GRIBMessage::unpack_pds(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  short flag,soff;
  short yr,mo,dy,hr,min;
  int dum;
  GRIBGrid *g;

  m_offsets.pds=curr_off;
  g=reinterpret_cast<GRIBGrid *>(grids.back().get());
  g->ensdata.fcst_type="";
  g->ensdata.id="";
  g->ensdata.total_size=0;
  bits::get(stream_buffer,m_lengths.pds,off,24);
  if (m_edition == 1) {
    bits::get(stream_buffer,g->grib.table,off+24,8);
  }
  g->grib.last_src=g->grid.src;
  bits::get(stream_buffer,g->grid.src,off+32,8);
  bits::get(stream_buffer,g->grib.process,off+40,8);
  bits::get(stream_buffer,g->grib.grid_catalog_id,off+48,8);
  g->grib.scan_mode=0x40;
  if (g->grid.src == 7) {
    switch (g->grib.grid_catalog_id) {
      case 21: {
        g->dim.y=36;
        g->dim.x=37;
        g->dim.size=1333;
        g->def.slatitude=0.;
        g->def.elatitude=90.;
        g->def.laincrement=2.5;
        g->def.slongitude=0.;
        g->def.elongitude=180.;
        g->def.loincrement=5.;
        break;
      }
      case 22: {
        g->dim.y=36;
        g->dim.x=37;
        g->dim.size=1333;
        g->def.slatitude=0.;
        g->def.elatitude=90.;
        g->def.laincrement=2.5;
        g->def.slongitude=-180.;
        g->def.elongitude=0.;
        g->def.loincrement=5.;
        break;
      }
      case 23: {
        g->dim.y=36;
        g->dim.x=37;
        g->dim.size=1333;
        g->def.slatitude=-90.;
        g->def.elatitude=0.;
        g->def.laincrement=2.5;
        g->def.slongitude=0.;
        g->def.elongitude=180.;
        g->def.loincrement=5.;
        break;
      }
      case 24: {
        g->dim.y=36;
        g->dim.x=37;
        g->dim.size=1333;
        g->def.slatitude=-90.;
        g->def.elatitude=0.;
        g->def.laincrement=2.5;
        g->def.slongitude=-180.;
        g->def.elongitude=0.;
        g->def.loincrement=5.;
        break;
      }
      case 25: {
        g->dim.y=18;
        g->dim.x=72;
        g->dim.size=1297;
        g->def.slatitude=0.;
        g->def.elatitude=90.;
        g->def.laincrement=5.;
        g->def.slongitude=0.;
        g->def.elongitude=355.;
        g->def.loincrement=5.;
        break;
      }
      case 26: {
        g->dim.y=18;
        g->dim.x=72;
        g->dim.size=1297;
        g->def.slatitude=-90.;
        g->def.elatitude=0.;
        g->def.laincrement=5.;
        g->def.slongitude=0.;
        g->def.elongitude=355.;
        g->def.loincrement=5.;
        break;
      }
      case 61: {
        g->dim.y=45;
        g->dim.x=91;
        g->dim.size=4096;
        g->def.slatitude=0.;
        g->def.elatitude=90.;
        g->def.laincrement=2.;
        g->def.slongitude=0.;
        g->def.elongitude=180.;
        g->def.loincrement=2.;
        break;
      }
      case 62: {
        g->dim.y=45;
        g->dim.x=91;
        g->dim.size=4096;
        g->def.slatitude=0.;
        g->def.elatitude=90.;
        g->def.laincrement=2.;
        g->def.slongitude=-180.;
        g->def.elongitude=0.;
        g->def.loincrement=2.;
        break;
      }
      case 63: {
        g->dim.y=45;
        g->dim.x=91;
        g->dim.size=4096;
        g->def.slatitude=-90.;
        g->def.elatitude=0.;
        g->def.laincrement=2.;
        g->def.slongitude=0.;
        g->def.elongitude=180.;
        g->def.loincrement=2.;
        break;
      }
      case 64: {
        g->dim.y=45;
        g->dim.x=91;
        g->dim.size=4096;
        g->def.slatitude=-90.;
        g->def.elatitude=0.;
        g->def.laincrement=2.;
        g->def.slongitude=-180.;
        g->def.elongitude=0.;
        g->def.loincrement=2.;
        break;
      }
      default: {
        g->grib.scan_mode=0x0;
      }
    }
  }
  bits::get(stream_buffer,flag,off+56,8);
  if ( (flag & 0x80) == 0x80) {
    gds_included=true;
  }
  else {
    gds_included=false;
    m_lengths.gds=0;
  }
  if ( (flag & 0x40) == 0x40) {
    bms_included=true;
  }
  else {
    bms_included=false;
    m_lengths.bms=0;
  }
  bits::get(stream_buffer,g->grid.param,off+64,8);
  bits::get(stream_buffer,g->grid.level1_type,off+72,8);
  switch (g->grid.level1_type) {
    case 100:
    case 103:
    case 105:
    case 109:
    case 111:
    case 113:
    case 115:
    case 125:
    case 160:
    case 200:
    case 201: {
      bits::get(stream_buffer,dum,off+80,16);
      g->grid.level1=dum;
      g->grid.level2=0.;
      g->grid.level2_type=-1;
      break;
    }
    case 101: {
      bits::get(stream_buffer,dum,off+80,8);
      g->grid.level1=dum*10.;
      bits::get(stream_buffer,dum,off+88,8);
      g->grid.level2=dum*10.;
      g->grid.level2_type=g->grid.level1_type;
      break;
    }
    case 107:
    case 119: {
      bits::get(stream_buffer,dum,off+80,16);
      g->grid.level1=dum/10000.;
      g->grid.level2=0.;
      g->grid.level2_type=-1;
      break;
    }
    case 108:
    case 120: {
      bits::get(stream_buffer,dum,off+80,8);
      g->grid.level1=dum/100.;
      bits::get(stream_buffer,dum,off+88,8);
      g->grid.level2=dum/100.;
      g->grid.level2_type=g->grid.level1_type;
      break;
    }
    case 114: {
      bits::get(stream_buffer,dum,off+80,8);
      g->grid.level1=475.-dum;
      bits::get(stream_buffer,dum,off+88,8);
      g->grid.level2=475.-dum;
      g->grid.level2_type=g->grid.level1_type;
      break;
    }
    case 121: {
      bits::get(stream_buffer,dum,off+80,8);
      g->grid.level1=1100.-dum;
      bits::get(stream_buffer,dum,off+88,8);
      g->grid.level2=1100.-dum;
      g->grid.level2_type=g->grid.level1_type;
      break;
    }
    case 128: {
      bits::get(stream_buffer,dum,off+80,8);
      g->grid.level1=1.1-dum/1000.;
      bits::get(stream_buffer,dum,off+88,8);
      g->grid.level2=1.1-dum/1000.;
      g->grid.level2_type=g->grid.level1_type;
      break;
    }
    case 141: {
      bits::get(stream_buffer,dum,off+80,8);
      g->grid.level1=dum*10.;
      bits::get(stream_buffer,dum,off+88,8);
      g->grid.level2=1100.-dum;
      g->grid.level2_type=g->grid.level1_type;
      break;
    }
    default: {
      bits::get(stream_buffer,dum,off+80,8);
      g->grid.level1=dum;
      bits::get(stream_buffer,dum,off+88,8);
      g->grid.level2=dum;
      g->grid.level2_type=g->grid.level1_type;
     }
  }
  bits::get(stream_buffer,yr,off+96,8);
  bits::get(stream_buffer,mo,off+104,8);
  bits::get(stream_buffer,dy,off+112,8);
  bits::get(stream_buffer,hr,off+120,8);
  bits::get(stream_buffer,min,off+128,8);
  hr=hr*100+min;
  bits::get(stream_buffer,g->grib.time_unit,off+136,8);
  switch (m_edition) {
    case 0: {
      if (yr > 20) {
        yr+=1900;
      }
      else {
        yr+=2000;
      }
      g->grib.sub_center=0;
      g->grib.D=0;
      break;
    }
    case 1: {
      bits::get(stream_buffer,g->grib.century,off+192,8);
      yr=yr+(g->grib.century-1)*100;
      bits::get(stream_buffer,g->grib.sub_center,off+200,8);
      bits::get(stream_buffer,g->grib.D,off+208,16);
      if (g->grib.D > 0x8000) {
        g->grib.D=0x8000-g->grib.D;
      }
      break;
    }
  }
  g->m_reference_date_time.set(yr,mo,dy,hr*100);
  bits::get(stream_buffer,g->grib.t_range,off+160,8);
  g->grid.nmean=g->grib.nmean_missing=0;
  g->grib.p2=0;
  switch (g->grib.t_range) {
    case 10: {
      bits::get(stream_buffer,g->grib.p1,off+144,16);
      break;
    }
    default: {
      bits::get(stream_buffer,g->grib.p1,off+144,8);
      bits::get(stream_buffer,g->grib.p2,off+152,8);
      bits::get(stream_buffer,g->grid.nmean,off+168,16);
      bits::get(stream_buffer,g->grib.nmean_missing,off+184,8);
      if (g->grib.t_range >= 113 && g->grib.time_unit == 1 && g->grib.p2 > 24) {
        std::cerr << "Warning: invalid P2 " << g->grib.p2;
        bits::get(stream_buffer,g->grib.p2,off+152,4);
        std::cerr << " re-read as " << g->grib.p2 << " for time range " << g->grib.t_range << std::endl;
      }
    }
  }
  g->m_forecast_date_time = g->m_valid_date_time = g->m_reference_date_time;
  std::tie(g->grib.prod_descr, g->m_forecast_date_time, g->m_valid_date_time,
      g->grid.fcst_time) = grib_utils::grib_product(g);

  // unpack PDS supplement, if it exists
  if (m_lengths.pds > 28) {
    if (m_lengths.pds < 41) {
      mywarning="supplement to PDS is in reserved position starting at octet 29";
      m_lengths.pds_supp=m_lengths.pds-28;
      soff=28;
    }
    else {
      m_lengths.pds_supp=m_lengths.pds-40;
      soff=40;
    }
    pds_supp.reset(new unsigned char[m_lengths.pds_supp]);
    std::copy(&stream_buffer[m_lengths.is+soff],&stream_buffer[m_lengths.is+soff+m_lengths.pds_supp],pds_supp.get());
// NCEP ensemble grids are described in the PDS extension
    if (g->grid.src == 7 && g->grib.sub_center == 2 && pds_supp[0] == 1) {
      switch (pds_supp[1]) {
        case 1: {
          switch (pds_supp[2]) {
            case 1: {
              g->ensdata.fcst_type="HRCTL";
              break;
            }
            case 2: {
              g->ensdata.fcst_type="LRCTL";
              break;
            }
          }
          break;
        }
        case 2: {
          g->ensdata.fcst_type="NEG";
          g->ensdata.id=strutils::itos(pds_supp[2]);
          break;
        }
        case 3: {
          g->ensdata.fcst_type="POS";
          g->ensdata.id=strutils::itos(pds_supp[2]);
          break;
        }
        case 5: {
          g->ensdata.id="ALL";
          switch (static_cast<int>(pds_supp[3])) {
            case 1: {
              g->ensdata.fcst_type="UNWTD_MEAN";
              break;
            }
            case 11: {
              g->ensdata.fcst_type="STDDEV";
              break;
            }
            default: {
              myerror="PDS byte 44 value "+std::string(&(reinterpret_cast<char *>(pds_supp.get()))[3],1)+" not recognized";
              exit(1);
            }
          }
          if (m_lengths.pds_supp > 20) {
            g->ensdata.total_size=pds_supp[20];
          }
          break;
        }
        default: {
          myerror="PDS byte 42 value "+std::string(&(reinterpret_cast<char *>(pds_supp.get()))[1],1)+" not recognized";
          exit(1);
        }
      }
    }
  }
  else {
    m_lengths.pds_supp=0;
    pds_supp.reset(nullptr);
  }
  g->grid.num_missing=0;
  curr_off+=m_lengths.pds;
}

void GRIBMessage::unpack_gds(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  GRIBGrid *g;
  int n,noff;
  int dum,sl_sign;
  short PV,NV,PL;

  m_offsets.gds=curr_off;
  g=reinterpret_cast<GRIBGrid *>(grids.back().get());
  bits::get(stream_buffer,m_lengths.gds,off,24);
  PL=0xff;
  switch (m_edition) {
    case 1: {
      bits::get(stream_buffer,PV,off+32,8);
      if (PV != 0xff) {
        bits::get(stream_buffer,NV,off+24,8);
        if (NV > 0)
          PL=(4*NV)+PV;
        else
          PL=PV;
/*
    if (grib.PL >= grib.m_lengths.gds) {
      std::cerr << "Error in unpack_gds: grib.PL (" << grib.PL << ") exceeds grib.m_lengths.gds (" << grib.m_lengths.gds << "); NV=" << grib.NV << ", PV=" << grib.PV << std::endl;
      exit(1);
    }
*/
        if (PL > m_lengths.gds) {
          PL=0xff;
        }
        else {
          if (g->plist != nullptr) {
            delete[] g->plist;
          }
          bits::get(stream_buffer,g->dim.y,off+64,16);
          g->plist=new int[g->dim.y];
          bits::get(stream_buffer,g->plist,(PL+curr_off-1)*8,16,0,g->dim.y);
        }
      }
      break;
    }
  }
  bits::get(stream_buffer,g->grid.grid_type,off+40,8);
  switch (g->grid.grid_type) {
    case 0: {
      g->def.type=Grid::Type::latitudeLongitude;
      break;
    }
    case 1: {
      g->def.type=Grid::Type::mercator;
      break;
    }
    case 3: {
      g->def.type=Grid::Type::lambertConformal;
      break;
    }
    case 5: {
      g->def.type=Grid::Type::polarStereographic;
      break;
    }
    case 4:
    case 10: {
      g->def.type=Grid::Type::gaussianLatitudeLongitude;
      break;
    }
    case 50: {
      g->def.type=Grid::Type::sphericalHarmonics;
      break;
    }
  }
  switch (g->grid.grid_type) {
    case 0:
    case 4:
    case 10: {
// Latitude/Longitude
// Gaussian Lat/Lon
// Rotated Lat/Lon
      bits::get(stream_buffer,g->dim.x,off+48,16);
      bits::get(stream_buffer,g->dim.y,off+64,16);
      if (g->dim.x == -1 && PL != 0xff) {
        noff=off+(PL-1)*8;
        g->dim.size=0;
        for (n=PL; n < m_lengths.gds; n+=2) {
          bits::get(stream_buffer,dum,noff,16);
          g->dim.size+=dum;
          noff+=16;
        }
      }
      else {
        g->dim.size=g->dim.x*g->dim.y;
      }
      bits::get(stream_buffer,dum,off+80,24);
      if (dum > 0x800000) {
        dum=0x800000-dum;
      }
      g->def.slatitude=dum/1000.;
      bits::get(stream_buffer,dum,off+104,24);
      if (dum > 0x800000) {
        dum=0x800000-dum;
      }
      g->def.slongitude=dum/1000.;
/*
      dum=lround(64.*g->def.slongitude);
      g->def.slongitude=dum/64.;
*/
      bits::get(stream_buffer,g->grib.rescomp,off+128,8);
      bits::get(stream_buffer,dum,off+136,24);
      if (dum > 0x800000) {
        dum=0x800000-dum;
      }
      g->def.elatitude=dum/1000.;
      bits::get(stream_buffer,sl_sign,off+160,1);
      sl_sign= (sl_sign == 0) ? 1 : -1;
      bits::get(stream_buffer,dum,off+161,23);
      g->def.elongitude=(dum/1000.)*sl_sign;
/*
      dum=lround(64.*g->def.elongitude);
      g->def.elongitude=dum/64.;
*/
      bits::get(stream_buffer,dum,off+184,16);
      if (dum == 0xffff) {
        dum=-99000;
      }
      g->def.loincrement=dum/1000.;
/*
      dum=lround(64.*g->def.loincrement);
      g->def.loincrement=dum/64.;
*/
      bits::get(stream_buffer,dum,off+200,16);
      if (dum == 0xffff) {
        dum=-99999;
      }
      if (g->grid.grid_type == 0) {
        g->def.laincrement=dum/1000.;
      }
      else {
        g->def.num_circles=dum;
      }
      bits::get(stream_buffer,g->grib.scan_mode,off+216,8);
/*
      if (g->def.elongitude < g->def.slongitude && g->grib.scan_mode < 0x80) {
        g->def.elongitude+=360.;
      }
*/
      break;
    }
    case 3:
    case 5: {
// Lambert Conformal
// Polar Stereographic
      bits::get(stream_buffer,g->dim.x,off+48,16);
      bits::get(stream_buffer,g->dim.y,off+64,16);
      g->dim.size=g->dim.x*g->dim.y;
      bits::get(stream_buffer,dum,off+80,24);
      if (dum > 0x800000) {
        dum=0x800000-dum;
      }
      g->def.slatitude=dum/1000.;
      bits::get(stream_buffer,dum,off+104,24);
      if (dum > 0x800000) {
        dum=0x800000-dum;
      }
      g->def.slongitude=dum/1000.;
      bits::get(stream_buffer,dum,off+104,24);
      if (dum > 0x800000) {
        dum=0x800000-dum;
      }
      g->def.slongitude=dum/1000.;
      bits::get(stream_buffer,g->grib.rescomp,off+128,8);
      bits::get(stream_buffer,dum,off+136,24);
      if (dum > 0x800000) {
        dum=0x800000-dum;
      }
      g->def.olongitude=dum/1000.;
      bits::get(stream_buffer,dum,off+160,24);
      g->def.dx=dum/1000.;
      bits::get(stream_buffer,dum,off+184,24);
      g->def.dy=dum/1000.;
      bits::get(stream_buffer,g->def.projection_flag,off+208,8);
      bits::get(stream_buffer,g->grib.scan_mode,off+216,8);
      if (g->grid.grid_type == 3) {
        bits::get(stream_buffer,dum,off+224,24);
        if (dum > 0x800000) {
          dum=0x800000-dum;
        }
        g->def.stdparallel1=dum/1000.;
        bits::get(stream_buffer,dum,off+248,24);
        if (dum > 0x800000) {
          dum=0x800000-dum;
        }
        g->def.stdparallel2=dum/1000.;
        bits::get(stream_buffer,dum,off+272,24);
        if (dum > 0x800000) {
          dum=0x800000-dum;
        }
        g->grib.sp_lat=dum/1000.;
        bits::get(stream_buffer,dum,off+296,24);
        if (dum > 0x800000) {
          dum=0x800000-dum;
        }
        g->grib.sp_lon=dum/1000.;
        g->def.llatitude= (fabs(g->def.stdparallel1) > fabs(g->def.stdparallel2)) ? g->def.stdparallel1 : g->def.stdparallel2;
        if ( (g->def.projection_flag & 0x40) == 0x40) {
          g->def.num_centers=2;
        }
        else {
          g->def.num_centers=1;
        }
      }
      else {
        if (g->def.projection_flag < 0x80) {
          g->def.llatitude=60.;
        }
        else {
          g->def.llatitude=-60.;
        }
      }
      break;
    }
    case 50: {
      g->dim.x=g->dim.y=g->dim.size=1;
      bits::get(stream_buffer,g->def.trunc1,off+48,16);
      bits::get(stream_buffer,g->def.trunc2,off+64,16);
      bits::get(stream_buffer,g->def.trunc3,off+80,16);
      break;
    }
    default: {
      std::cerr << "Warning: unpack_gds does not recognize data representation " << g->grid.grid_type << std::endl;
    }
  }
  curr_off+=m_lengths.gds;
}

void GRIBMessage::unpack_bms(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  GRIBGrid *g;
  GRIB2Grid *g2;
  short sec_num,ind;
  int n,len;

  m_offsets.bms=curr_off;
  switch (m_edition) {
    case 1: {
      g=reinterpret_cast<GRIBGrid *>(grids.back().get());
      bits::get(stream_buffer,m_lengths.bms,off,24);
      bits::get(stream_buffer,ind,off+32,16);
      if (ind == 0) {
        if (g->dim.size > g->bitmap.capacity) {
          if (g->bitmap.map != nullptr) {
            delete[] g->bitmap.map;
          }
          g->bitmap.capacity=g->dim.size;
          g->bitmap.map=new unsigned char[g->bitmap.capacity];
        }
        bits::get(stream_buffer,g->bitmap.map,off+48,1,0,g->dim.size);
        for (n=0; n < static_cast<int>(g->dim.size); ++n) {
          if (g->bitmap.map[n] == 0) {
            ++g->grid.num_missing;
          }
        }
      }
      break;
    }
    case 2: {
      g2=reinterpret_cast<GRIB2Grid *>(grids.back().get());
      bits::get(stream_buffer,m_lengths.bms,off,32);
      bits::get(stream_buffer,sec_num,off+32,8);
      if (sec_num != 6) {
        mywarning="may not be unpacking the Bitmap Section";
      }
      bits::get(stream_buffer,ind,off+40,8);
      switch (ind) {
        case 0: {
// bitmap applies and is specified in this section
          if ( (len=(m_lengths.bms-6)*8) > 0) {
            if (len > static_cast<int>(g2->bitmap.capacity)) {
              if (g2->bitmap.map != nullptr) {
                delete[] g2->bitmap.map;
              }
              g2->bitmap.capacity=len;
              g2->bitmap.map=new unsigned char[g2->bitmap.capacity];
            }
/*
            for (n=0; n < len; n++) {
              bits::get(stream_buffer,bit,off+48+n,1);
              g2->bitmap.map[n]=bit;
            }
*/
bits::get(stream_buffer,g2->bitmap.map,off+48,1,0,len);
            g2->bitmap.applies=true;
          }
          else {
            g2->bitmap.applies=false;
          }
          break;
        }
        case 254: {
// bitmap has already been defined for the current GRIB2 message
          g2->bitmap.applies=true;
          break;
        }
        case 255: {
// bitmap does not apply to this message
          g2->bitmap.applies=false;
          break;
        }
        default: {
          myerror="not currently setup to deal with pre-defined bitmaps";
          exit(1);
        }
      }
      break;
    }
  }
  curr_off+=m_lengths.bms;
}

void GRIBMessage::unpack_bds(const unsigned char *stream_buffer,bool fill_header_only)
{
  off_t off=curr_off*8;
  GRIBGrid *g;
  short bds_flag,unused;
  bool simple_packing=true;
  double d,e;
  size_t num_packed=0,npoints;
  short ydim;
  int *pval,pole_at=-1,ystart=0;
  int cnt=0,bcnt=0,avg_cnt=0;
  int n,m;

  m_offsets.bds=curr_off;
  g=reinterpret_cast<GRIBGrid *>(grids.back().get());
  bits::get(stream_buffer,m_lengths.bds,off,24);
  if (mlength >= 0x800000 && m_lengths.bds < 120) {
// ECMWF large-file
    mlength&=0x7fffff;
    mlength*=120;
    mlength-=m_lengths.bds;
    mlength+=4;
    m_lengths.bds=mlength-curr_off-4;
  }
  bits::get(stream_buffer,bds_flag,off+24,4);
  if ((bds_flag & 0x4) == 0x4) {
    simple_packing=false;
  }
  bits::get(stream_buffer,unused,off+28,4);
  d=pow(10.,g->grib.D);
  g->stats.min_val=floatutils::ibmconv(stream_buffer,off+48)/d;
  if ((bds_flag & 0x2) == 0x2) {
    g->stats.min_val=lround(g->stats.min_val);
  }
  bits::get(stream_buffer,g->grib.pack_width,off+80,8);
  bits::get(stream_buffer,g->grib.E,off+32,16);
  if (g->grib.E > 0x8000) {
    g->grib.E=0x8000-g->grib.E;
  }
  if (simple_packing) {
    if (g->grib.pack_width > 0) {
      if (g->def.type != Grid::Type::sphericalHarmonics) {
        num_packed=g->dim.size-g->grid.num_missing;
        npoints=((m_lengths.bds-11)*8-unused)/g->grib.pack_width;
        if (npoints != num_packed && g->dim.size != 0 && npoints != g->dim.size)
          std::cerr << "Warning: unpack_bds: specified # gridpoints " << num_packed << " vs. # packed " << npoints << "  date: " << g->m_reference_date_time.to_string() << "  param: " << g->grid.param << "  lengths.bds: " << m_lengths.bds << "  ubits: " << unused << "  pack_width: " << g->grib.pack_width << "  grid size: " << g->dim.size << "  #missing: " << g->grid.num_missing << std::endl;
      }
      else {
      }
    }
    else
      num_packed=0;
  }
  ydim=g->dim.y;
  if (fill_header_only) {
    if (g->dim.x == -1) {
      n=ydim/2;
      if (g->def.elongitude >= g->def.slongitude)
        g->def.loincrement=(g->def.elongitude-g->def.slongitude)/(g->plist[n]-1);
      else
        g->def.loincrement=(g->def.elongitude+360.-(g->def.slongitude))/(g->plist[n]-1);
    }
  }
  else {
    e=pow(2.,g->grib.E);
    g->grid.filled=true;
    g->grid.pole=Grid::MISSING_VALUE;
    g->stats.max_val=-Grid::MISSING_VALUE;
    g->stats.avg_val=0.;
    g->galloc();
    if (simple_packing) {
      pval=new int[num_packed];
      bits::get(stream_buffer,pval,off+88,g->grib.pack_width,0,num_packed);
      g->stats.min_i=-1;
      if (g->grid.num_in_pole_sum < 0) {
        pole_at=-(g->grid.num_in_pole_sum+1);
        if (pole_at == 0)
          ystart=1;
        else if (pole_at == static_cast<int>(g->dim.size-1))
          ydim--;
        else {
          myerror="don't know how to unpack grid";
          exit(1);
        }
      }
      switch (g->grid.grid_type) {
        case 0:
        case 3:
        case 4:
        case 5: {
          if (pole_at >= 0) {
            g->grid.pole=g->stats.min_val+pval[pole_at]*e/d;
          }
          if (pole_at == 0) {
            ++cnt;
          }
          for (n=ystart; n < ydim; ++n) {
// non-reduced grids
            if (g->dim.x != -1) {
              for (m=0; m < g->dim.x; ++m) {
                if (bms_included) {
                  if (g->bitmap.map[bcnt] == 1) {
                    if (g->grib.pack_width > 0) {
                      g->m_gridpoints[n][m]=g->stats.min_val+pval[cnt]*e/d;
                      if (pval[cnt] == 0) {
                        if (g->stats.min_i < 0) {
                          g->stats.min_i=m+1;
                          if (g->def.type == Grid::Type::gaussianLatitudeLongitude && g->grib.scan_mode == 0x40) {
                            g->stats.min_j=ydim-n;
                          }
                          else {
                            g->stats.min_j=n+1;
                          }
                        }
                      }
                    }
                    else {
                      g->m_gridpoints[n][m]=g->stats.min_val;
                      if (g->stats.min_i < 0) {
                        g->stats.min_i=m+1;
                        if (g->def.type == Grid::Type::gaussianLatitudeLongitude && g->grib.scan_mode == 0x40)
                          g->stats.min_j=ydim-n;
                        else
                          g->stats.min_j=n+1;
                      }
                    }
                    if (g->m_gridpoints[n][m] > g->stats.max_val) {
                      g->stats.max_val=g->m_gridpoints[n][m];
                      g->stats.max_i=m+1;
                      g->stats.max_j=n+1;
                    }
                    g->stats.avg_val+=g->m_gridpoints[n][m];
                    ++avg_cnt;
                    ++cnt;
                  }
                  else {
                    g->m_gridpoints[n][m]=Grid::MISSING_VALUE;
                  }
                  ++bcnt;
                }
                else {
                  if (g->grib.pack_width > 0) {
                    g->m_gridpoints[n][m]=g->stats.min_val+pval[cnt]*e/d;
                    if (pval[cnt] == 0) {
                      if (g->stats.min_i < 0) {
                        g->stats.min_i=m+1;
                        if (g->def.type == Grid::Type::gaussianLatitudeLongitude && g->grib.scan_mode == 0x40)
                           g->stats.min_j=ydim-n;
                        else
                          g->stats.min_j=n+1;
                      }
                    }
                  }
                  else {
                    g->m_gridpoints[n][m]=g->stats.min_val;
                    if (g->stats.min_i < 0) {
                      g->stats.min_i=m+1;
                      if (g->def.type == Grid::Type::gaussianLatitudeLongitude && g->grib.scan_mode == 0x40)
                        g->stats.min_j=ydim-n;
                      else
                        g->stats.min_j=n+1;
                    }
                  }
                  if (g->m_gridpoints[n][m] > g->stats.max_val) {
                    g->stats.max_val=g->m_gridpoints[n][m];
                    g->stats.max_i=m+1;
                    g->stats.max_j=n+1;
                  }
                  g->stats.avg_val+=g->m_gridpoints[n][m];
                  ++avg_cnt;
                  ++cnt;
                }
              }
            }
// reduced grids
            else {
              for (m=1; m <= static_cast<int>(g->m_gridpoints[n][0]); ++m) {
                if (bms_included) {
myerror="unable to unpack reduced grids with bitmap";
exit(1);
                  if (g->grib.pack_width > 0) {
                  }
                  else {
                  }
                }
                else {
                  if (g->grib.pack_width > 0) {
                    g->m_gridpoints[n][m]=g->stats.min_val+pval[cnt]*e/d;
                    if (pval[cnt] == 0) {
                      if (g->stats.min_i < 0) {
                        g->stats.min_i=m;
                        if (g->def.type == Grid::Type::gaussianLatitudeLongitude && g->grib.scan_mode == 0x40) {
                           g->stats.min_j=ydim-n;
                        }
                        else {
                          g->stats.min_j=n+1;
                        }
                      }
                    }
                  }
                  else {
                    g->m_gridpoints[n][m]=g->stats.min_val;
                    if (g->stats.min_i < 0) {
                      g->stats.min_i=m;
                      if (g->def.type == Grid::Type::gaussianLatitudeLongitude && g->grib.scan_mode == 0x40) {
                        g->stats.min_j=ydim-n;
                      }
                      else {
                        g->stats.min_j=n+1;
                      }
                    }
                  }
                  if (g->m_gridpoints[n][m] > g->stats.max_val) {
                     g->stats.max_val=g->m_gridpoints[n][m];
                    g->stats.max_i=m;
                    g->stats.max_j=n+1;
                  }
                  g->stats.avg_val+=g->m_gridpoints[n][m];
                  ++avg_cnt;
                  ++cnt;
                }
              }
            }
          }
          if (pole_at >= 0) {
// pole was first point packed
            if (ystart == 1) {
              for (n=0; n < g->dim.x; g->m_gridpoints[0][n++]=g->grid.pole);
            }
// pole was last point packed
            else if (ydim != g->dim.y) {
              for (n=0; n < g->dim.x; g->m_gridpoints[ydim][n++]=g->grid.pole);
            }
          }
          else {
            if (floatutils::myequalf(g->def.slatitude,90.)) {
              g->grid.pole=g->m_gridpoints[0][0];
            }
            else if (floatutils::myequalf(g->def.elatitude,90.)) {
              g->grid.pole=g->m_gridpoints[ydim-1][0];
            }
          }
          break;
        }
        default: {
          std::cerr << "Warning: unpack_bds does not recognize grid type " << g->grid.grid_type << std::endl;
        }
      }
      delete[] pval;
    }
    else {
// second-order packing
       if ((bds_flag & 0x4) == 0x4) {
        if ((bds_flag & 0xc) == 0xc) {
/*
std::cerr << "here" << std::endl;
bits::get(input_buffer,n,off+88,16);
std::cerr << "n=" << n << std::endl;
bits::get(input_buffer,n,off+104,16);
std::cerr << "p=" << n << std::endl;
bits::get(input_buffer,n,off+120,8);
bits::get(input_buffer,m,off+128,8);
bits::get(input_buffer,cnt,off+136,8);
std::cerr << "j,k,m=" << n << "," << m << "," << cnt << std::endl;
*/
        }
        else {
if (bms_included) {
myerror="unable to decode complex packing with bitmap defined";
exit(1);
}
          short n1,ext_flg,n2,p1,p2;
          bits::get(stream_buffer,n1,off+88,16);
          bits::get(stream_buffer,ext_flg,off+104,8);
if ( (ext_flg & 0x20) == 0x20) {
myerror="unable to decode complex packing with secondary bitmap defined";
exit(1);
}
          bits::get(stream_buffer,n2,off+112,16);
          bits::get(stream_buffer,p1,off+128,16);
          bits::get(stream_buffer,p2,off+144,16);
          if ( (ext_flg & 0x8) == 0x8) {
// ECMWF extension for complex packing with spatial differencing
            short width_pack_width,length_pack_width,nl,order_vals_width;
            bits::get(stream_buffer,width_pack_width,off+168,8);
            bits::get(stream_buffer,length_pack_width,off+176,8);
            bits::get(stream_buffer,nl,off+184,16);
            bits::get(stream_buffer,order_vals_width,off+200,8);
            auto norder=(ext_flg & 0x3);
            std::unique_ptr<int []> first_vals;
            first_vals.reset(new int[norder]);
            bits::get(stream_buffer,first_vals.get(),off+208,order_vals_width,0,norder);
            short sign;
            int omin=0;
            auto noff=off+208+norder*order_vals_width;
            bits::get(stream_buffer,sign,noff,1);
            bits::get(stream_buffer,omin,noff+1,order_vals_width-1);
            if (sign == 1) {
              omin=-omin;
            }
            noff+=order_vals_width;
            auto pad=(noff % 8);
            if (pad > 0) {
              pad=8-pad;
            }
            noff+=pad;
            std::unique_ptr<int []> widths(new int[p1]),lengths(new int[p1]);
            bits::get(stream_buffer,widths.get(),noff,width_pack_width,0,p1);
            bits::get(stream_buffer,lengths.get(),off+(nl-1)*8,length_pack_width,0,p1);
            int max_length=0,lsum=0;
            for (auto n=0; n < p1; ++n) {
              if (lengths[n] > max_length) {
                max_length=lengths[n];
              }
              lsum+=lengths[n];
            }
            if (lsum != p2) {
              myerror="inconsistent number of 2nd-order values - computed: "+strutils::itos(lsum)+" vs. encoded: "+strutils::itos(p2);
              exit(1);
            }
            std::unique_ptr<int []> group_ref_vals(new int[p1]);
            bits::get(stream_buffer,group_ref_vals.get(),off+(n1-1)*8,8,0,p1);
            std::unique_ptr<int []> pvals(new int[max_length]);
            noff=off+(n2-1)*8;
            for (auto n=0; n < norder; ++n) {
              auto j=n/g->dim.x;
              auto i=(n % g->dim.x);
              g->m_gridpoints[j][i]=g->stats.min_val+first_vals[n]*e/d;
              if (g->m_gridpoints[j][i] > g->stats.max_val) {
                g->stats.max_val=g->m_gridpoints[j][i];
              }
              g->stats.avg_val+=g->m_gridpoints[j][i];
              ++avg_cnt;
            }
// unpack the field of differences
            for (int n=0,l=norder; n < p1; ++n) {
              if (widths[n] > 0) {
                bits::get(stream_buffer,pvals.get(),noff,widths[n],0,lengths[n]);
                noff+=widths[n]*lengths[n];
                 for (auto m=0; m < lengths[n]; ++m) {
                  auto j=l/g->dim.x;
                  auto i=(l % g->dim.x);
                  g->m_gridpoints[j][i]=pvals[m]+group_ref_vals[n]+omin;
                  ++l;
                }
              }
              else {
// constant group
                 for (auto m=0; m < lengths[n]; ++m) {
                  auto j=l/g->dim.x;
                  auto i=(l % g->dim.x);
                  g->m_gridpoints[j][i]=group_ref_vals[n]+omin;
                  ++l;
                }
              }
            }
            double lastgp;
            for (int n=norder-1; n > 0; --n) {
              lastgp=first_vals[n]-first_vals[n-1];
              for (size_t l=n+1; l < g->dim.size; ++l) {
                auto j=l/g->dim.x;
                auto i=(l % g->dim.x);
                g->m_gridpoints[j][i]+=lastgp;
                lastgp=g->m_gridpoints[j][i];
              }
            }
            lastgp=g->stats.min_val*d/e+first_vals[norder-1];
            for (size_t l=norder; l < g->dim.size; ++l) {
              auto j=l/g->dim.x;
              auto i=(l % g->dim.x);
              lastgp+=g->m_gridpoints[j][i];
              g->m_gridpoints[j][i]=lastgp*e/d;
              if (g->m_gridpoints[j][i] > g->stats.max_val) {
                g->stats.max_val=g->m_gridpoints[j][i];
              }
              g->stats.avg_val+=g->m_gridpoints[j][i];
              ++avg_cnt;
            }
            if ((ext_flg & 0x2) == 0x2) {
// m_gridpoints are boustrophedonic
               for (auto j=0; j < g->dim.y; ++j) {
                if ( (j % 2) == 1) {
                  for (auto i=0,l=g->dim.x-1; i < g->dim.x/2; ++i,--l) {
                    auto temp=g->m_gridpoints[j][i];
                    g->m_gridpoints[j][i]=g->m_gridpoints[j][l];
                    g->m_gridpoints[j][l]=temp;
                  }
                }
              }
            }
          }
          else {
            std::unique_ptr<short []> p2_widths;
            if ((ext_flg & 0x10) == 0x10) {
              p2_widths.reset(new short[p1]);
              bits::get(stream_buffer,p2_widths.get(),off+168,8,0,p1);
            }
            else {
              p2_widths.reset(new short[1]);
            }
          }
        }
      }
      else {
        myerror="complex unpacking not defined for bds_flag = "+strutils::itos(bds_flag);
        exit(1);
      }
    }
    if (avg_cnt > 0) {
      g->stats.avg_val/=static_cast<double>(avg_cnt);
    }
  }
  curr_off+=m_lengths.bds;
}

void GRIBMessage::unpack_end(const unsigned char *stream_buffer)
{
  off_t off=curr_off*8;
  int test;
  bits::get(stream_buffer,test,off,32);
  if (test != 0x37373737) {
    myerror="bad END Section - found *"+std::string(reinterpret_cast<char *>(const_cast<unsigned char *>(&stream_buffer[curr_off])),4)+"*";
    exit(1);
  }
}

void GRIBMessage::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  if (stream_buffer == nullptr) {
    myerror="empty file stream";
    exit(1);
  }
  clear_grids();
  unpack_is(stream_buffer);
  if (m_edition > 1) {
    myerror="can't decode edition "+strutils::itos(m_edition);
    exit(1);
  }
  auto g=new GRIBGrid;
  g->grid.filled=false;
  grids.emplace_back(g);
  unpack_pds(stream_buffer);
  if (m_edition == 0) {
    mlength+=m_lengths.pds;
  }
  if (gds_included) {
    unpack_gds(stream_buffer);
    if (m_edition == 0) {
      mlength+=m_lengths.gds;
    }
  }
  else {
    static my::map<xmlutils::GridMapEntry> grid_map_table;
    xmlutils::GridMapEntry gme;
    gme.key=strutils::itos(g->grid.src)+"-"+strutils::itos(g->grib.sub_center);
    if (!grid_map_table.found(gme.key,gme)) {
      static TempDir *temp_dir=nullptr;
      if (temp_dir == nullptr) {
        temp_dir=new TempDir;
        if (!temp_dir->create("/tmp")) {
          myerror="GRIBMessage::fill(): unable to create temporary directory";
          exit(1);
        }
      }
      gme.g.fill(xmlutils::map_filename("GridTables",gme.key,temp_dir->name()));
      grid_map_table.insert(gme);
    }
    auto catalog_id=strutils::itos(g->grib.grid_catalog_id);
    g->def=gme.g.grid_definition(catalog_id);
    g->dim=gme.g.grid_dimensions(catalog_id);
    if (gme.g.is_single_pole_point(catalog_id)) {
      g->grid.num_in_pole_sum=-std::stoi(gme.g.pole_point_location(catalog_id));
    }
    g->grib.scan_mode=0x40;
  }
  if (g->dim.x == 0 || g->dim.y == 0 || g->dim.size == 0) {
    mywarning="grid dimensions not defined";
  }
  if (bms_included) {
    unpack_bms(stream_buffer);
    if (m_edition == 0) {
      mlength+=m_lengths.bms;
    }
  }
  unpack_bds(stream_buffer,fill_header_only);
  if (m_edition == 0) {
    mlength+=(m_lengths.bds+4);
  }
  unpack_end(stream_buffer);
}

Grid *GRIBMessage::grid(size_t grid_number) const
{
  if (grids.size() == 0 || grid_number >= grids.size()) {
    return nullptr;
  }
  else {
    return grids[grid_number].get();
  }
}

void GRIBMessage::pds_supplement(unsigned char *pds_supplement,size_t& pds_supplement_length) const
{
  pds_supplement_length=m_lengths.pds_supp;
  if (pds_supplement_length > 0) {
    for (size_t n=0; n < pds_supplement_length; ++n) {
      pds_supplement[n]=pds_supp[n];
    }
  }
  else {
    pds_supplement=nullptr;
  }
}

void GRIBMessage::initialize(short m_editionnumber,unsigned char *pds_supplement,int pds_supplement_length,bool gds_is_included,bool bms_is_included)
{
  m_edition=m_editionnumber;
  switch (m_edition) {
    case 0: {
      myerror="can't create GRIB0";
      exit(1);
    }
    case 1: {
      m_lengths.is=8;
      break;
    }
    case 2: {
      m_lengths.is=16;
      break;
    }
  }
  m_lengths.pds_supp=pds_supplement_length;
  if (m_lengths.pds_supp > 0) {
    pds_supp.reset(new unsigned char[m_lengths.pds_supp]);
    std::copy(pds_supplement,pds_supplement+m_lengths.pds_supp,pds_supp.get());
  }
  else {
    pds_supp.reset(nullptr);
  }
  gds_included=gds_is_included;
  bms_included=bms_is_included;
  clear_grids();
}

void GRIBMessage::print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
  if (verbose) {
    outs << "  GRIB Ed: " << m_edition << "  Length: " << mlength << std::endl;
    if (m_lengths.pds_supp > 0) {
      outs << "\n  Supplement to the PDS:";
      for (int n=0; n < m_lengths.pds_supp; ++n) {
        if (pds_supp[n] < 32 || pds_supp[n] > 127) {
          outs << " \\" << std::setw(3) << std::setfill('0') << std::oct << static_cast<int>(pds_supp[n]) << std::setfill(' ') << std::dec;
        }
        else {
          outs << " " << pds_supp[n];
        }
      }
    }
    (reinterpret_cast<GRIBGrid *>(grids.front().get()))->print_header(outs,verbose,path_to_parameter_map);
  }
  else {
    outs << " Ed=" << m_edition;
    (reinterpret_cast<GRIBGrid *>(grids.front().get()))->print_header(outs,verbose,path_to_parameter_map);
  }
}

const char *GRIBGrid::LEVEL_TYPE_SHORT_NAME[] = { "0Reserved", "SFC" , "CBL" ,
    "CTL" , "0DEG", "ADCL", "MWSL", "TRO" , "NTAT", "SEAB", "", "", "", "",
    "", "", "", "", "", "", "TMPL", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "ISBL", "ISBY", "MSL" , "GPML",
    "GPMY", "HTGL", "HTGY", "SIGL", "SIGY", "HYBL", "HYBY", "DBLL", "DBLY",
    "THEL", "THEY", "SPDL", "SPDY", "PVL" , "", "ETAL", "ETAY", "IBYH", "", "",
    "", "HGLH", "", "", "SGYH", "", "", "", "", "", "", "", "", "", "", "", "",
    "IBYM", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "DBSL", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "EATM", "EOCN" };
const char *GRIBGrid::LEVEL_TYPE_UNITS[] = { "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "mbars",
    "mbars", "", "m above MSL", "hm above MSL", "m AGL", "hm AGL", "sigma",
    "sigma", "", "", "cm below surface", "cm below surface", "degK", "degK",
    "mbars", "mbars", "10^-6Km^2/kgs", "", "", "", "mbars", "", "", "",
    "cm AGL", "", "", "sigma", "", "", "", "", "", "", "", "", "", "", "", "",
    "mbars", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "m below MSL", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "sigma", "" };
const char *GRIBGrid::TIME_UNITS[] = { "Minutes", "Hours", "Days", "Months",
    "Years", "Decades", "Normals", "Centuries", "", "", "3Hours", "6Hours",
    "12Hours", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "Seconds",
    "Missing" };
const short GRIBGrid::OCTAGONAL_GRID_PARAMETER_MAP[] = { 0, 7, 35, 0, 7, 39, 1,
     0, 0, 0, 11, 12, 0, 0, 0, 0, 0, 0, 0, 18, 15, 16, 0, 0, 0, 0, 0, 0, 2, 0,
     33, 34, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 52, 0, 0, 80, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 66 };
const short GRIBGrid::NAVY_GRID_PARAMETER_MAP[] = { 0, 7, 0, 0, 0, 0, 2, 0, 0,
     0, 11, 0, 18, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 33, 34, 0,
     31, 32, 0, 0, 0, 0, 0, 40, 0, 0, 0, 0, 55, 0, 0, 0, 0, 0, 0, 0, 0, 0, 11,
     11, 11, 11, 11 };
const short GRIBGrid::ON84_GRID_PARAMETER_MAP[] = { 0, 7, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 11, 17, 18, 13, 15, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 39, 0, 0, 0, 0, 0, 0, 0, 33, 34, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 41, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 52, 54, 61, 0, 0, 66, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 210, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0 };

GRIBGrid::~GRIBGrid()
{
  if (plist != nullptr) {
    delete[] plist;
    plist=nullptr;
  }
  if (bitmap.map != nullptr) {
    delete[] bitmap.map;
    bitmap.map=nullptr;
  }
  if (m_gridpoints != nullptr) {
    for (size_t n=0; n < grib.capacity.y; ++n) {
      delete[] m_gridpoints[n];
    }
    delete[] m_gridpoints;
    m_gridpoints=nullptr;
  }
  if (map.param != nullptr) {
    delete[] map.param;
    map.param=nullptr;
    delete[] map.level_type;
    delete[] map.lvl;
    delete[] map.lvl2;
    delete[] map.mult;
  }
}

GRIBGrid& GRIBGrid::operator=(const GRIBGrid& source)
{
  size_t n,m;

  if (this == &source) {
    return *this;
  }
  m_reference_date_time=source.m_reference_date_time;
  m_valid_date_time=source.m_valid_date_time;
  dim=source.dim;
  def=source.def;
  stats=source.stats;
  ensdata=source.ensdata;
  grid=source.grid;
  if (source.plist != nullptr) {
    if (plist != nullptr) {
      delete[] plist;
    }
    plist=new int[dim.y];
    for (n=0; n < static_cast<size_t>(dim.y); ++n) {
      plist[n]=source.plist[n];
    }
  }
  else {
    if (plist != nullptr) {
      delete[] plist;
    }
    plist=nullptr;
  }
  bitmap.applies=source.bitmap.applies;
  resize_bitmap(source.bitmap.capacity);
  for (n=0; n < bitmap.capacity; ++n) {
    bitmap.map[n]=source.bitmap.map[n];
  }
  if (m_gridpoints != nullptr && grib.capacity.points < source.grib.capacity.points) {
    for (n=0; n < grib.capacity.y; ++n) {
      delete[] m_gridpoints[n];
    }
    delete[] m_gridpoints;
    m_gridpoints=nullptr;
  }
  grib=source.grib;
  if (source.grid.filled) {
    galloc();
    for (n=0; n < static_cast<size_t>(source.dim.y); ++n) {
      for (m=0; m < static_cast<size_t>(source.dim.x); ++m) {
        m_gridpoints[n][m]=source.m_gridpoints[n][m];
      }
    }
  }
  if (source.map.param != nullptr) {
    if (map.param != nullptr) {
      delete[] map.param;
      delete[] map.level_type;
      delete[] map.lvl;
      delete[] map.lvl2;
      delete[] map.mult;
    }
    map.param=new short[255];
    map.level_type=new short[255];
    map.lvl=new short[255];
    map.lvl2=new short[255];
    map.mult=new float[255];
    for (n=0; n < 255; ++n) {
      map.param[n]=source.map.param[n];
      map.level_type[n]=source.map.level_type[n];
      map.lvl[n]=source.map.lvl[n];
      map.lvl2[n]=source.map.lvl2[n];
      map.mult[n]=source.map.mult[n];
    }
  }
  else {
    if (map.param != nullptr) {
      delete[] map.param;
      delete[] map.level_type;
      delete[] map.lvl;
      delete[] map.lvl2;
      delete[] map.mult;
    }
    map.param=map.level_type=map.lvl=map.lvl2=nullptr;
    map.mult=nullptr;
  }
  return *this;
}

void GRIBGrid::galloc()
{
  size_t n;

  if (m_gridpoints != nullptr && dim.size > grib.capacity.points) {
    for (n=0; n < grib.capacity.y; ++n) {
      delete[] m_gridpoints[n];
    }
    delete[] m_gridpoints;
    m_gridpoints=nullptr;
  }
  if (m_gridpoints == nullptr) {
    grib.capacity.y=dim.y;
    m_gridpoints=new double *[grib.capacity.y];
    if (dim.x == -1) {
      for (n=0; n < grib.capacity.y; ++n) {
        m_gridpoints[n]=new double[plist[n]+1];
        m_gridpoints[n][0]=plist[n];
      }
    }
    else {
      for (n=0; n < grib.capacity.y; ++n) {
        m_gridpoints[n]=new double[dim.x];
      }
    }
    grib.capacity.points=dim.size;
  }
}

void GRIBGrid::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  myerror="a GRIB grid must be filled from a GRIB message";
  exit(1);
}

void GRIBGrid::fill_from_grib_data(const GRIBData& source)
{
  int n,m,cnt=0,avg_cnt=0;
  my::map<Grid::GLatEntry> gaus_lats;

  grib.table=source.param_version;
  grid.src=source.source;
  grib.process=source.process;
  grib.grid_catalog_id=source.grid_catalog_id;
  grib.scan_mode=source.scan_mode;
  grid.grid_type=source.grid_type;
  dim=source.dim;
  def=source.def;
  if (def.type == Grid::Type::gaussianLatitudeLongitude) {
    if (_path_to_gauslat_lists.empty()) {
      myerror="path to gaussian latitude data was not specified";
      exit(0);
    }
    else if (!gridutils::filled_gaussian_latitudes(_path_to_gauslat_lists,gaus_lats,def.num_circles,(grib.scan_mode&0x40) != 0x40)) {
      myerror="unable to get gaussian latitudes for "+strutils::itos(def.num_circles)+" circles from '"+_path_to_gauslat_lists+"'";
      exit(0);
    }
  }
  grid.param=source.param_code;
  grid.level1_type=source.level_types[0];
  grid.level2_type=source.level_types[1];
  switch (grid.level1_type) {
    case 1:
    case 100:
    case 103:
    case 105:
    case 107:
    case 109:
    case 111:
    case 113:
    case 115:
    case 117:
    case 125:
    case 160:
    case 200:
    case 201: {
      grid.level1=source.levels[0];
      grid.level2=0.;
      break;
    }
    default: {
      grid.level1=source.levels[0];
      grid.level2=source.levels[1];
    }
  }
  m_reference_date_time=source.reference_date_time;
  m_valid_date_time=source.valid_date_time;
  grib.time_unit=source.time_unit;
  grib.t_range=source.time_range;
  grid.nmean=0;
  grib.nmean_missing=0;
  switch (grib.t_range) {
    case 4:
    case 10: {
      grib.p1=source.p1;
      grib.p2=0;
      break;
    }
    default: {
      grib.p1=source.p1;
      grib.p2=source.p2;
    }
  }
  grid.nmean=source.num_averaged;
  grib.nmean_missing=source.num_averaged_missing;
  grib.sub_center=source.sub_center;
  grib.D=source.D;
  grid.num_missing=0;
  resize_bitmap(dim.size);
  bitmap.applies=false;
  stats.min_val=Grid::MISSING_VALUE;
  stats.max_val=-Grid::MISSING_VALUE;
  stats.avg_val=0.;
  grid.num_missing=0;
  galloc();
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
      m_gridpoints[n][m]=source.gridpoints[n][m];
      if (floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE)) {
        bitmap.map[cnt++]=0;
        ++grid.num_missing;
        bitmap.applies=true;
      }
      else {
        bitmap.map[cnt++]=1;
        if (m_gridpoints[n][m] > stats.max_val) {
          stats.max_val=m_gridpoints[n][m];
        }
        if (m_gridpoints[n][m] < stats.min_val) {
          stats.min_val=m_gridpoints[n][m];
        }
        stats.avg_val+=m_gridpoints[n][m];
        ++avg_cnt;
      }
    }
  }
  stats.avg_val/=static_cast<double>(avg_cnt);
  grib.pack_width=source.pack_width;
  grid.filled=true;
}

float GRIBGrid::latitude_at(int index,my::map<GLatEntry> *gaus_lats) const
{
  GLatEntry glat_entry;
  float findex=index;
  int n;

  if (def.type == Grid::Type::gaussianLatitudeLongitude) {
    glat_entry.key=def.num_circles;
    if (gaus_lats == nullptr || !gaus_lats->found(glat_entry.key,glat_entry)) {
      return -99.9;
    }
    if (static_cast<size_t>(dim.y) == def.num_circles*2) {
      return glat_entry.lats[index];
    }
    else {
      n=0;
      while (static_cast<size_t>(n) < def.num_circles*2) {
        if (fabs(def.slatitude-glat_entry.lats[n]) < 0.001) {
          break;
        }
        ++n;
      }
      if (grib.scan_mode == 0x0) {
        if (static_cast<size_t>(n+index) < def.num_circles*2) {
          return glat_entry.lats[n+index];
        }
        else {
          myerror="error finding first latitude for gaussian circles "+strutils::itos(def.num_circles);
          exit(1);
        }
      }
      else {
        if ( (dim.y-(n+index)) > 0) {
          return glat_entry.lats[dim.y-(n+index)-1];
        }
        else {
          myerror="error finding first latitude for gaussian circles "+strutils::itos(def.num_circles);
          exit(1);
        }
      }
    }
  }
  else {
    findex*=def.laincrement;
    if (grib.scan_mode == 0x0) {
      findex*=-1.;
    }
    return def.slatitude+findex;
  }
}

int GRIBGrid::latitude_index_of(float latitude,my::map<GLatEntry> *gaus_lats) const
{
  size_t n=0;
  if (def.type == Grid::Type::gaussianLatitudeLongitude) {
    GLatEntry glat_entry;
    glat_entry.key=def.num_circles;
    if (gaus_lats == nullptr || !gaus_lats->found(glat_entry.key,glat_entry)) {
      return -1;
    }
    glat_entry.key*=2;
    while (n < glat_entry.key) {
      if (floatutils::myequalf(latitude,glat_entry.lats[n],0.01)) {
        return n;
      }
      ++n;
    }
  }
  else {
    return Grid::latitude_index_of(latitude,gaus_lats);
  }
  return -1;
}

int GRIBGrid::latitude_index_north_of(float latitude,my::map<GLatEntry> *gaus_lats) const
{
  GLatEntry glat_entry;
  int n;

  if (def.type == Grid::Type::gaussianLatitudeLongitude) {
    glat_entry.key=def.num_circles;
    if (gaus_lats == nullptr || !gaus_lats->found(glat_entry.key,glat_entry)) {
      return -1;
    }
    glat_entry.key*=2;
    if (grib.scan_mode == 0x0) {
      n=0;
      while (n < static_cast<int>(glat_entry.key)) {
        if (latitude > glat_entry.lats[n]) {
          return (n-1);
        }
        ++n;
      }
    }
    else {
      n=glat_entry.key-1;
      while (n >= 0) {
        if (latitude > glat_entry.lats[n]) {
          return (n < static_cast<int>(glat_entry.key-1)) ? (n+1) : -1;
        }
        --n;
      }
    }
  }
  else {
    return Grid::latitude_index_north_of(latitude,gaus_lats);
  }
  return -1;
}

int GRIBGrid::latitude_index_south_of(float latitude,my::map<GLatEntry> *gaus_lats) const
{
  GLatEntry glat_entry;
  int n;

  if (def.type == Grid::Type::gaussianLatitudeLongitude) {
    glat_entry.key=def.num_circles;
    if (gaus_lats == nullptr || !gaus_lats->found(glat_entry.key,glat_entry)) {
      return -1;
    }
    glat_entry.key*=2;
    if (grib.scan_mode == 0x0) {
      n=0;
      while (n < static_cast<int>(glat_entry.key)) {
        if (latitude > glat_entry.lats[n]) {
          return n;
        }
        ++n;
      }
    }
    else {
      n=glat_entry.key-1;
      while (n >= 0) {
        if (latitude > glat_entry.lats[n]) {
          return n;
        }
        --n;
      }
    }
  }
  else {
    return Grid::latitude_index_south_of(latitude,gaus_lats);
  }
  return -1;
}

void GRIBGrid::resize_bitmap(size_t new_size) {
  if (bitmap.capacity < new_size) {
    if (bitmap.map != nullptr) {
        delete[] bitmap.map;
        bitmap.map = nullptr;
    }
    bitmap.capacity = new_size;
    bitmap.map = new unsigned char[bitmap.capacity];
  }
}

void GRIBGrid::remap_parameters()
{
  std::ifstream ifs;
  char line[80];
  std::string file_name;
  size_t n;

  if (!grib.map_available) {
    return;
  }
  if (!grib.map_filled || grib.last_src != grid.src) {
    if (map.param != nullptr) {
      delete[] map.param;
      delete[] map.level_type;
      delete[] map.lvl;
      delete[] map.lvl2;
      delete[] map.mult;
    }
    ifs.open(("/glade/u/home/rdadata/share/GRIB/parameter_map."+strutils::itos(grid.src)).c_str());
    if (ifs.is_open()) {
      map.param=new short[255];
      map.level_type=new short[255];
      map.lvl=new short[255];
      map.lvl2=new short[255];
      map.mult=new float[255];
      for (n=0; n < 255; ++n) {
        map.param[n]=map.level_type[n]=-1;
      }
      ifs.getline(line,80);
      while (!ifs.eof()) {
        strutils::strget(line,n,3);
        strutils::strget(&line[4],map.param[n],3);
        strutils::strget(&line[8],map.level_type[n],3);
        if (map.level_type[n] != -1) {
          strutils::strget(&line[12],map.lvl[n],5);
          strutils::strget(&line[18],map.lvl2[n],5);
        }
        strutils::strget(&line[24],map.mult[n],14);
        ifs.getline(line,80);
      }
      grib.map_filled=true;
      ifs.close();
    }
    else {
      grib.map_available=false;
    }
  }
  if (!grib.map_available) {
    return;
  }
  if (map.level_type[grid.param] != -1) {
    grid.level1_type=map.level_type[grid.param];
    grid.level1=map.lvl[grid.param];
    grid.level2=map.lvl2[grid.param];
  }
  if (!floatutils::myequalf(map.mult[grid.param],1.)) {
    this->multiply_by(map.mult[grid.param]);
  }
  if (map.param[grid.param] != -1) {
    grid.param=map.param[grid.param];
  }
}

void GRIBGrid::set_scale_and_packing_width(short decimal_scale_factor, short
    maximum_pack_width) {
  grib.D=decimal_scale_factor;
  double d=pow(10.,grib.D);
  double range=lroundf(stats.max_val*d)-lroundf(stats.min_val*d);
  long long lrange=llround(range);
  grib.pack_width=1;
  while (grib.pack_width < maximum_pack_width && lrange >= pow(2.,grib.pack_width)) {
    ++grib.pack_width;
  }
  grib.E=0;
  while (grib.E < 20 && lrange >= pow(2.,grib.pack_width)) {
    ++grib.E;
    lrange/=2;
  }
  if (lrange >= pow(2.,grib.pack_width)) {
myerror="unable to scale - range: "+strutils::lltos(lrange)+" pack width: "+strutils::itos(grib.pack_width);
exit(1);
  }
}

void GRIBGrid::operator+=(const GRIBGrid& source) {
  int n,m;
  size_t l,avg_cnt=0,cnt=0;
  double mult;

/*
  grid.src = 60;
  grib.sub_center = 1;
*/
  if (grib.capacity.points == 0) {
    *this = source;
    if (source.grib.t_range == 51) {
      grid.nmean = source.grid.nmean;
      this->multiply_by(grid.nmean);
    } else {
      grid.nmean = 1;
    }
    grib.t_range = 0;
  } else {
    if (dim != source.dim) {
      std::cerr << "Warning: unable to perform grid addition" << std::endl;
      return;
    }
    resize_bitmap(dim.size);
    grid.num_missing = 0;
    stats.max_val = -Grid::MISSING_VALUE;
    stats.min_val = Grid::MISSING_VALUE;
    stats.avg_val = 0.;
    if (source.grib.t_range == 51) {
      mult=source.grid.nmean;
    } else {
      mult=1.;
    }
    for (n=0; n < dim.y; ++n) {
      for (m=0; m < dim.x; ++m) {
        l= (source.grib.scan_mode == grib.scan_mode) ? n : dim.y-n-1;
        if (!floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE) && !floatutils::myequalf(source.m_gridpoints[l][m],Grid::MISSING_VALUE)) {
          m_gridpoints[n][m]+=(source.m_gridpoints[l][m]*mult);
          bitmap.map[cnt++]=1;
          if (m_gridpoints[n][m] > stats.max_val) {
            stats.max_val=m_gridpoints[n][m];
          }
          if (m_gridpoints[n][m] < stats.min_val) {
            stats.min_val=m_gridpoints[n][m];
          }
          stats.avg_val+=m_gridpoints[n][m];
          ++avg_cnt;
        }
        else {
          m_gridpoints[n][m]=(Grid::MISSING_VALUE*mult);
          bitmap.map[cnt++]=0;
          ++grid.num_missing;
        }
      }
    }
    if (grid.num_missing == 0) {
      bitmap.applies=false;
    }
    else {
      bitmap.applies=true;
    }
    if (source.grib.t_range == 51) {
      grid.nmean=grid.nmean+source.grid.nmean;
    }
    else {
      ++grid.nmean;
    }
    if (avg_cnt > 0)
      stats.avg_val/=static_cast<double>(avg_cnt);
    if (grib.t_range == 0) {
      if (source.m_reference_date_time.month() == m_reference_date_time.month()) {
        if (source.m_reference_date_time.day() == m_reference_date_time.day() && source.m_reference_date_time.year() > m_reference_date_time.year()) {
          grib.t_range=151;
          grib.p1=0;
          grib.p2=1;
          grib.time_unit=3;
        }
        else if (source.m_reference_date_time.year() == m_reference_date_time.year() && source.m_reference_date_time.day() > m_reference_date_time.day()){
          grib.t_range=114;
          grib.p2=1;
          grib.time_unit=2;
        }
        else if (source.m_reference_date_time.year() == m_reference_date_time.year() && source.m_reference_date_time.day() == m_reference_date_time.day() && source.m_reference_date_time.time() > m_reference_date_time.time()) {
          grib.t_range=114;
          grib.p2=source.m_reference_date_time.time()-m_reference_date_time.time()/100.;
          grib.time_unit=1;
        }
      }
    }
// adding a mean and an accumulation indicates a variance grid
    else if ((grib.t_range == 113 && source.grib.t_range == 114) ||
             (grib.t_range == 114 && source.grib.t_range == 113)) {
      grib.t_range=118;
      grid.nmean--;
    }
  }
}

void GRIBGrid::operator-=(const GRIBGrid& source)
{
  int n,m;
  size_t l,avg_cnt=0,cnt=0;

  if (grib.capacity.points == 0) {
    *this=source;
    grib.t_range=5;
    grib.p2=source.grib.p2;
  }
  else {
    if (dim.size != source.dim.size || dim.x != source.dim.x || dim.y != source.dim.y) {
      std::cerr << "Warning: unable to perform grid subtraction" << std::endl;
      return;
    }
    else {
      if (bitmap.capacity == 0) {
        bitmap.capacity=dim.size;
        bitmap.map=new unsigned char[bitmap.capacity];
        grid.num_missing=0;
      }
      stats.max_val=-Grid::MISSING_VALUE;
      stats.min_val=Grid::MISSING_VALUE;
      stats.avg_val=0;
      for (n=0; n < dim.y; ++n) {
        for (m=0; m < dim.x; ++m) {
          l= (source.grib.scan_mode == grib.scan_mode) ? n : dim.y-n-1;
          if (!floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE) && !floatutils::myequalf(source.m_gridpoints[l][m],Grid::MISSING_VALUE)) {
            m_gridpoints[n][m]-=source.m_gridpoints[l][m];
            bitmap.map[cnt++]=1;
            if (m_gridpoints[n][m] > stats.max_val) {
              stats.max_val=m_gridpoints[n][m];
            }
            if (m_gridpoints[n][m] < stats.min_val) {
              stats.min_val=m_gridpoints[n][m];
            }
            stats.avg_val+=m_gridpoints[n][m];
            ++avg_cnt;
          }
          else {
            m_gridpoints[n][m]=Grid::MISSING_VALUE;
            bitmap.map[cnt++]=0;
            ++grid.num_missing;
          }
        }
      }
      if (grid.num_missing == 0) {
        bitmap.applies=false;
      }
      else {
        bitmap.applies=true;
      }
      if (avg_cnt > 0) {
        stats.avg_val/=static_cast<double>(avg_cnt);
      }
    }
    grib.p1=source.grib.p1;
  }
}

void GRIBGrid::operator*=(const GRIBGrid& source)
{
  int n,m;
  size_t avg_cnt=0;

  if (grib.capacity.points == 0)
    return;

  if (dim.size != source.dim.size || dim.x != source.dim.x || dim.y !=
      source.dim.y) {
    std::cerr << "Warning: unable to perform grid multiplication" << std::endl;
    return;
  }
  else {
    stats.max_val=-Grid::MISSING_VALUE;
    stats.min_val=Grid::MISSING_VALUE;
    stats.avg_val=0.;
    for (n=0; n < dim.y; ++n) {
      for (m=0; m < dim.x; ++m) {
        if (!floatutils::myequalf(source.m_gridpoints[n][m],Grid::MISSING_VALUE)) {
          m_gridpoints[n][m]*=source.m_gridpoints[n][m];
          if (m_gridpoints[n][m] > stats.max_val) {
            stats.max_val=m_gridpoints[n][m];
          }
          if (m_gridpoints[n][m] < stats.min_val) {
            stats.min_val=m_gridpoints[n][m];
          }
          stats.avg_val+=m_gridpoints[n][m];
          ++avg_cnt;
        }
        else {
          m_gridpoints[n][m]=Grid::MISSING_VALUE;
        }
      }
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
}

void GRIBGrid::operator/=(const GRIBGrid& source)
{
  int n,m;
  size_t avg_cnt=0;

  if (grib.capacity.points == 0) {
    return;
  }
  if (dim.size != source.dim.size || dim.x != source.dim.x || dim.y != source.dim.y) {
    std::cerr << "Warning: unable to perform grid division" << std::endl;
    return;
  }
  else {
    stats.max_val=-Grid::MISSING_VALUE;
    stats.min_val=Grid::MISSING_VALUE;
    stats.avg_val=0.;
    for (n=0; n < dim.y; ++n) {
      for (m=0; m < dim.x; ++m) {
        if (!floatutils::myequalf(source.m_gridpoints[n][m],Grid::MISSING_VALUE)) {
          m_gridpoints[n][m]/=source.m_gridpoints[n][m];
          if (m_gridpoints[n][m] > stats.max_val) {
            stats.max_val=m_gridpoints[n][m];
          }
          if (m_gridpoints[n][m] < stats.min_val) {
            stats.min_val=m_gridpoints[n][m];
          }
          stats.avg_val+=m_gridpoints[n][m];
          ++avg_cnt;
        }
        else {
          m_gridpoints[n][m]=Grid::MISSING_VALUE;
        }
      }
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
}

void GRIBGrid::divide_by(double div)
{
  int n,m;
  size_t avg_cnt=0;

  if (grib.capacity.points == 0) {
    return;
  }
  switch (grib.t_range) {
    case 114: {
      grib.t_range=113;
      break;
    }
    case 151: {
      grib.t_range=51;
      break;
    }
  }
  stats.min_val/=div;
  stats.max_val/=div;
  stats.avg_val=0.;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
      if (!floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE)) {
        m_gridpoints[n][m]/=div;
        stats.avg_val+=m_gridpoints[n][m];
        ++avg_cnt;
      }
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
}

GRIBGrid& GRIBGrid::operator=(const TDLGRIBGrid& source)
{
  int n,m;
  size_t avg_cnt=0,max_pack;
  double mult=1.,range;

  *this=*(reinterpret_cast<GRIBGrid *>(const_cast<TDLGRIBGrid *>(&source)));
  grid.num_missing=0;
  grib.table=3;
  grid.src=7;
  grib.grid_catalog_id=255;
  grib.rescomp=0;
  grib.p2=0;
  grib.t_range=10;
  switch (source.model()) {
    case 6: {
      switch (source.parameter()) {
        case 1000: {
// Height
          grid.param=7;
          grid.level1_type=100;
          break;
        }
        case 1201: {
// MSLP
          grid.param=2;
          grid.level1_type=102;
          mult=100.;
          grib.D-=2;
          break;
        }
        case 2000: {
// temperature
          grid.param=11;
          grid.level1_type=100;
          break;
        }
        case 3000: {
// RH
          grid.param=52;
          grid.level1_type=100;
          break;
        }
        case 3001:
        case 3002: {
// mean RH through a sigma layer
          grid.param=52;
          grid.level1_type=108;
          grid.level1=47.;
          grid.level2=100.;
          break;
        }
        case 3211: {
// 6-hr total precip
          grid.param=61;
          grid.level1_type=1;
          mult=1000.;
          grib.D-=3;
          if ( (source.grid.fcst_time % 10000) > 0) {
            grib.time_unit=0;
            grib.p2=source.grid.fcst_time/10000*60+(source.grid.fcst_time % 10000)/100;
            grib.p1=grib.p2-360;
          }
          else {
            grib.time_unit=1;
            grib.p2=source.grid.fcst_time/10000;
            grib.p1=grib.p2-6;
          }
          grib.t_range=4;
          break;
        }
        case 3221: {
// 12-hr total precip
          grid.param=61;
          grid.level1_type=1;
          mult=1000.;
          grib.D-=3;
          if ( (source.grid.fcst_time % 10000) > 0) {
            grib.time_unit=0;
            grib.p2=source.grid.fcst_time/10000*60+(source.grid.fcst_time % 10000)/100;
            grib.p1=grib.p2-720;
          }
          else {
            grib.time_unit=1;
            grib.p2=source.grid.fcst_time/10000;
            grib.p1=grib.p2-12;
          }
          grib.t_range=4;
          break;
        }
        case 3350: {
// total precipitable water
          grid.param=54;
          grid.level1_type=200;
          break;
        }
        case 4000: {
// u-component of the wind
          grid.param=33;
          grid.level1_type=100;
          grib.rescomp|=0x8;
          break;
        }
        case 4020: {
// 10m u-component
          grid.param=33;
          grid.level1_type=105;
          grib.rescomp|=0x8;
          break;
        }
        case 4100: {
// v-component of the wind
          grid.param=34;
          grid.level1_type=100;
          grib.rescomp|=0x8;
          break;
        }
        case 4120: {
// 10m v-component
          grid.param=34;
          grid.level1_type=105;
          grib.rescomp|=0x8;
          break;
        }
        case 5000:
        case 5003: {
// vertical velocity
          grid.param=39;
          grid.level1_type=100;
          mult=100.;
          grib.D-=2;
          break;
        }
        default: {
          myerror="unable to convert parameter "+strutils::itos(source.parameter());
          exit(1);
        }
      }
      grib.process=39;
      break;
    }
    default: {
      myerror="unrecognized model "+strutils::itos(source.model());
      exit(1);
    }
  }
  grid.level2_type=-1;
  grib.sub_center=11;
  grib.scan_mode=0x40;
  if (grid.grid_type == 5 && !floatutils::myequalf(def.elatitude,60.)) {
    myerror="unable to handle a grid length not at 60 degrees latitude";
    exit(1);
  }
  galloc();
  stats.avg_val=0.;
  stats.min_val=Grid::MISSING_VALUE;
  stats.max_val=-Grid::MISSING_VALUE;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
      m_gridpoints[n][m]=source.gridpoint(m,n)*mult;
      if (m_gridpoints[n][m] > stats.max_val) {
        stats.max_val=m_gridpoints[n][m];
      }
      if (m_gridpoints[n][m] < stats.min_val) {
        stats.min_val=m_gridpoints[n][m];
      }
      stats.avg_val+=m_gridpoints[n][m];
      ++avg_cnt;
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
  grib.pack_width=16;
  max_pack=0x8000;
  range=round(stats.max_val-stats.min_val);
  while (max_pack > 0 && max_pack > range) {
    grib.pack_width--;
    max_pack/=2;
  }
  grid.filled=true;
  return *this;
}

GRIBGrid& GRIBGrid::operator=(const OctagonalGrid& source)
{
  int n,m;
  size_t cnt=0,avg_cnt=0;
  size_t max_pack;
  double range;
  float factor=1.,offset=0.;

  grib.table=3;
  switch (source.source()) {
    case 1: {
      grid.src=7;
      break;
    }
    case 3: {
      grid.src=58;
      break;
    }
    default: {
      myerror="source code "+strutils::itos(source.source())+" not recognized";
      exit(1);
    }
  }
  grib.process=0;
  grib.grid_catalog_id=255;
  grid.param=OCTAGONAL_GRID_PARAMETER_MAP[source.parameter()];
  grid.level2_type=-1;
  switch (grid.param) {
    case 1: {
      if (floatutils::myequalf(source.first_level_value(),1013.)) {
        grid.level1_type=2;
        grid.param=2;
      }
      else
        grid.level1_type=1;
      factor=100.;
      break;
    }
    case 2: {
      grid.level1_type=2;
      break;
    }
    case 7: {
      grid.level1_type=100;
      break;
    }
    case 11:
    case 12: {
      if (floatutils::myequalf(source.first_level_value(),1001.)) {
        grid.level1_type=1;
      }
      else {
        grid.level1_type=100;
      }
      offset=273.15;
      break;
    }
    case 33:
    case 34:
    case 39: {
      grid.level1_type=100;
      break;
    }
    case 52: {
      grid.level1_type=101;
      break;
    }
    case 80: {
      grid.level1_type=1;
      offset=273.15;
      break;
    }
    default: {
      myerror="unable to convert parameter "+strutils::itos(source.parameter());
      exit(1);
    }
  }
  if (!floatutils::myequalf(source.second_level_value(),0.)) {
    grid.level1=source.second_level_value();
    grid.level2=source.first_level_value();
  }
  else {
    grid.level1=source.first_level_value();
    grid.level2=0.;
  }
  m_reference_date_time=source.reference_date_time();
  if (!source.is_averaged_grid()) {
    grib.p1=source.forecast_time()/10000;
    grib.time_unit=1;
    grib.p2=0;
    grib.t_range=10;
  }
  else {
    myerror="unable to convert a mean grid";
    exit(1);
  }
  grib.sub_center=0;
  grib.D=2;
  grid.grid_type=5;
  grib.scan_mode=0x40;
  dim=source.dimensions();
  def=source.definition();
  def.projection_flag=0x80;
  grib.rescomp=0x80;
  galloc();
  resize_bitmap(dim.size);
  grid.num_missing=0;
  stats.avg_val=0.;
  stats.min_val=Grid::MISSING_VALUE;
  stats.max_val=-Grid::MISSING_VALUE;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
      m_gridpoints[n][m]=source.gridpoint(m,n);
      if (floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE)) {
        bitmap.map[cnt]=0;
        ++grid.num_missing;
      }
      else {
        m_gridpoints[n][m]*=factor;
        m_gridpoints[n][m]+=offset;
        bitmap.map[cnt]=1;
        if (m_gridpoints[n][m] > stats.max_val) {
          stats.max_val=m_gridpoints[n][m];
        }
        if (m_gridpoints[n][m] < stats.min_val) {
          stats.min_val=m_gridpoints[n][m];
        }
        stats.avg_val+=m_gridpoints[n][m];
        ++avg_cnt;
      }
      ++cnt;
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
  grib.pack_width=16;
  max_pack=0x8000;
  range=round(stats.max_val-stats.min_val);
  while (max_pack > 0 && max_pack > range) {
    --grib.pack_width;
    max_pack/=2;
  }
  if (grid.num_missing == 0) {
    bitmap.applies=false;
  }
  else {
    bitmap.applies=true;
  }
  grid.filled=true;
  return *this;
}


GRIBGrid& GRIBGrid::operator=(const LatLonGrid& source)
{
  int n,m;
  size_t cnt=0,avg_cnt=0;
  size_t max_pack;
  double range;
  float factor=1.,offset=0.;

  grib.table=3;
  switch (source.source()) {
    case 1:
    case 5: {
      grid.src=7;
      break;
    }
    case 2: {
      grid.src=57;
      break;
    }
    case 3: {
      grid.src=58;
      break;
    }
    case 4: {
      grid.src=255;
/*
      grib.m_lengths.pds_supp=10;
      grib.m_lengths.pds+=(12+grib.m_lengths.pds_supp);
      if (pds_supp != nullptr)
        delete[] pds_supp;
      pds_supp=new unsigned char[11];
      strcpy((char *)pds_supp,"ESSPO GRID");
*/
      break;
    }
    case 9: {
      if (floatutils::myequalf(source.first_level_value(),700.)) {
        grid.src=7;
      }
      else {
        myerror="source code "+strutils::itos(source.source())+" not recognized";
        exit(1);
      }
      break;
    }
    default: {
      myerror="source code "+strutils::itos(source.source())+" not recognized";
      exit(1);
    }
  }
  grib.process=0;
  grib.grid_catalog_id=255;
  grid.param=OCTAGONAL_GRID_PARAMETER_MAP[source.parameter()];
  grid.level2_type=-1;
  switch (grid.param) {
    case 1: {
      if (floatutils::myequalf(source.first_level_value(),1013.)) {
        grid.level1_type=2;
        grid.param=2;
      }
      else {
        grid.level1_type=1;
      }
      factor=100.;
      break;
    }
    case 2: {
      grid.level1_type=2;
      break;
    }
    case 7: {
      grid.level1_type=100;
      if (floatutils::myequalf(source.first_level_value(),700.) && source.source() == 9) {
        factor=0.01;
      }
      break;
    }
    case 11:
    case 12: {
      if (floatutils::myequalf(source.first_level_value(),1001.)) {
        grid.level1_type=1;
      }
      else {
        grid.level1_type=100;
      }
      offset=273.15;
      break;
    }
    case 33:
    case 34:
    case 39: {
      grid.level1_type=100;
      break;
    }
    case 52: {
      grid.level1_type=101;
      break;
    }
    case 80: {
      grid.level1_type=1;
      offset=273.15;
      break;
    }
    default: {
      myerror="unable to convert parameter "+strutils::itos(source.parameter());
      exit(1);
    }
  }
  if (grid.level1_type > 10 && grid.level1_type != 102 && grid.level1_type != 200 && grid.level1_type != 201) {
    if (!floatutils::myequalf(source.second_level_value(),0.)) {
      grid.level1=source.first_level_value();
      grid.level2=source.second_level_value();
    }
    else {
      grid.level1=source.first_level_value();
      grid.level2=0.;
    }
  }
  else {
    grid.level1=grid.level2=0.;
  }
  m_reference_date_time=source.reference_date_time();
  if (!source.is_averaged_grid()) {
    grib.p1=source.forecast_time()/10000;
    grib.time_unit=1;
    grib.p2=0;
    grib.t_range=10;
    grid.nmean=0;
  }
  else {
    myerror="unable to convert a mean grid";
    exit(1);
  }
  grib.sub_center=0;
  grib.D=2;
  grid.grid_type=0;
  grib.scan_mode=0x40;
  dim=source.dimensions();
  def=source.definition();
  grib.rescomp=0x80;
  galloc();
  resize_bitmap(dim.size);
  grid.num_missing=0;
  stats.avg_val=0.;
  stats.min_val=Grid::MISSING_VALUE;
  stats.max_val=-Grid::MISSING_VALUE;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
      m_gridpoints[n][m]=source.gridpoint(m,n);
      if (floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE)) {
        bitmap.map[cnt]=0;
        ++grid.num_missing;
      }
      else {
        m_gridpoints[n][m]*=factor;
        m_gridpoints[n][m]+=offset;
        bitmap.map[cnt]=1;
        if (m_gridpoints[n][m] > stats.max_val) {
          stats.max_val=m_gridpoints[n][m];
        }
        if (m_gridpoints[n][m] < stats.min_val) {
          stats.min_val=m_gridpoints[n][m];
        }
        stats.avg_val+=m_gridpoints[n][m];
        ++avg_cnt;
      }
      ++cnt;
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
  grib.pack_width=16;
  max_pack=0x8000;
  range=round(stats.max_val-stats.min_val);
  while (max_pack > 0 && max_pack > range) {
    --grib.pack_width;
    max_pack/=2;
  }
  if (grid.num_missing == 0) {
    bitmap.applies=false;
  }
  else {
    bitmap.applies=true;
  }
  grid.filled=true;
  return *this;
}

GRIBGrid& GRIBGrid::operator=(const SLPGrid& source)
{
  int n,m;
  size_t cnt=0,avg_cnt=0;
  size_t max_pack;
  double range;
  short months[]={0,31,28,31,30,31,30,31,31,30,31,30,31};

  grib.table=3;
  switch (source.source()) {
    case 3: {
      grid.src=58;
      break;
    }
    case 4: {
      grid.src=57;
      break;
    }
    case 11: {
      grid.src=255;
      break;
    }
    case 10: {
      grid.src=59;
      break;
    }
    case 12: {
      grid.src=74;
      break;
    }
    case 28: {
      grid.src=7;
      break;
    }
    case 29: {
      grid.src=255;
      break;
    }
    default: {
      myerror="source code "+strutils::itos(source.source())+" not recognized";
      exit(1);
    }
  }
  grib.process=0;
  grib.grid_catalog_id=255;
  grid.param=2;
  grid.level1_type=102;
  grid.level1=0.;
  grid.level2_type=-1;
  grid.level2=0.;
  grib.p1=0;
  if (!source.is_averaged_grid()) {
    grib.p2=0;
    m_reference_date_time=source.reference_date_time();
    grib.time_unit=1;
    grib.t_range=10;
    grid.nmean=0;
    grib.nmean_missing=0;
  }
  else {
    if (source.reference_date_time().time() == 310000) {
      m_reference_date_time.set(source.reference_date_time().year(),source.reference_date_time().month(),1,0);
    }
    else {
      m_reference_date_time.set(source.reference_date_time().year(),source.reference_date_time().month(),1,source.reference_date_time().time());
    }
    grib.time_unit=3;
    grib.t_range=113;
    if ( (m_reference_date_time.year() % 4) == 0) {
      months[2]=29;
    }
    grid.nmean=months[m_reference_date_time.month()];
    if (source.number_averaged() > grid.nmean) {
      grid.nmean*=2;
      grib.time_unit=1;
      grib.p2=12;
    }
    else {
      grib.time_unit=2;
      grib.p2=1;
    }
    grib.nmean_missing=grid.nmean-source.number_averaged();
  }
  grib.sub_center=0;
  grib.D=2;
  grid.grid_type=0;
  grib.scan_mode=0x40;
  dim=source.dimensions();
  dim.size=dim.x*dim.y;
  def=source.definition();
  def.elatitude=90.;
  grib.rescomp=0x80;
  galloc();
  resize_bitmap(dim.size);
  grid.num_missing=0;
  stats.avg_val=0.;
  stats.min_val=Grid::MISSING_VALUE;
  stats.max_val=-Grid::MISSING_VALUE;
  for (n=0; n < dim.y-1; ++n) {
    for (m=0; m < dim.x; ++m) {
      m_gridpoints[n][m]=source.gridpoint(m,n);
      if (floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE)) {
        bitmap.map[cnt]=0;
        ++grid.num_missing;
      }
      else {
        m_gridpoints[n][m]*=100.;
        bitmap.map[cnt]=1;
        if (m_gridpoints[n][m] > stats.max_val) {
          stats.max_val=m_gridpoints[n][m];
        }
        if (m_gridpoints[n][m] < stats.min_val) {
          stats.min_val=m_gridpoints[n][m];
        }
        stats.avg_val+=m_gridpoints[n][m];
        ++avg_cnt;
      }
      ++cnt;
    }
  }
  for (m=0; m < dim.x; ++m) {
    m_gridpoints[n][m]=source.pole_value();
    if (floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE)) {
      bitmap.map[cnt]=0;
      ++grid.num_missing;
    }
    else {
      m_gridpoints[n][m]*=100.;
      bitmap.map[cnt]=1;
      if (m_gridpoints[n][m] > stats.max_val) {
        stats.max_val=m_gridpoints[n][m];
      }
      if (m_gridpoints[n][m] < stats.min_val) {
        stats.min_val=m_gridpoints[n][m];
      }
      stats.avg_val+=m_gridpoints[n][m];
      ++avg_cnt;
    }
    ++cnt;
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
  grib.pack_width=16;
  max_pack=0x8000;
  range=round(stats.max_val-stats.min_val);
  while (max_pack > 0 && max_pack > range) {
    --grib.pack_width;
    max_pack/=2;
  }
  if (grid.num_missing == 0) {
    bitmap.applies=false;
  }
  else {
    bitmap.applies=true;
  }
  set_scale_and_packing_width(grib.D,grib.pack_width);
  grid.filled=true;
  return *this;
}

GRIBGrid& GRIBGrid::operator=(const ON84Grid& source)
{
  int n,m;
  size_t cnt=0,avg_cnt=0;
  size_t max_pack;
  double mult=1.,range,precision;

  grib.table=3;
  grid.src=7;
  grib.process=source.generating_program();
  grib.grid_catalog_id=255;
  grid.param=ON84_GRID_PARAMETER_MAP[source.parameter()];
  grib.D=2;
  switch (grid.param) {
    case 1: {
      mult=100.;
      grib.D=0;
      break;
    }
    case 39: {
      mult=100.;
      grib.D=3;
      break;
    }
    case 61: {
      mult=1000.;
      grib.D=1;
      break;
    }
    case 210: {
      precision=5;
      grib.D=0;
      grib.table=129;
      break;
    }
  }
  if (grid.param == 0) {
    myerror="unable to convert parameter "+strutils::itos(source.parameter());
    exit(1);
  }
  grid.level2_type=-1;
  switch (source.first_level_type()) {
    case 6: {
// height above ground
      grid.level1_type=105;
      grid.level1=source.first_level_value();
      grid.level2=source.second_level_value();
      if (!floatutils::myequalf(grid.level1,source.first_level_value()) || !floatutils::myequalf(grid.level2,source.second_level_value())) {
        myerror="error converting fractional level(s) "+strutils::ftos(source.first_level_value())+" "+strutils::ftos(source.second_level_value());
        exit(1);
      }
      break;
    }
    case 8: {
// pressure level
      grid.level1_type=100;
      grid.level1=source.first_level_value();
      grid.level2=source.second_level_value();
      break;
    }
    case 128: {
// mean sea-level
      grid.level1_type=102;
      grid.level1=grid.level2=0.;
      if (grid.param == 1) grid.param=2;
      break;
    }
    case 129: {
// surface
      if (source.parameter() == 16) {
        grid.level1_type=105;
        grid.level1=2.;
        grid.level2=0.;
      }
      else {
        grid.level1_type=1;
        grid.level1=grid.level2=0.;
      }
      break;
    }
    case 130: {
// tropopause level
      grid.level1_type=7;
      grid.level1=grid.level2=0.;
      break;
    }
    case 131: {
// maximum wind speed level
      grid.level1_type=6;
      grid.level1=grid.level2=0.;
      break;
    }
    case 144: {
// boundary
      if (floatutils::myequalf(source.first_level_value(),0.) && floatutils::myequalf(source.second_level_value(),1.)) {
        grid.level1_type=107;
        grid.level1=0.9950;
        grid.level2=0.;
      }
      else {
        myerror="error converting boundary layer "+strutils::ftos(source.first_level_value())+" "+strutils::ftos(source.second_level_value());
        exit(1);
      }
      break;
    }
    case 145: {
// troposphere
// sigma level
      if (floatutils::myequalf(source.second_level_value(),0.)) {
        grid.level1_type=107;
        grid.level1=source.first_level_value();
        grid.level2=0.;
      }
// sigma layer
      else {
        if (floatutils::myequalf(source.first_level_value(),0.) && floatutils::myequalf(source.second_level_value(),1.)) {
          grid.level1_type=200;
          grid.level1=0.;
        }
        else {
          grid.level1_type=108;
          grid.level1=source.first_level_value();
          grid.level2=source.second_level_value();
        }
      }
      break;
    }
    case 148: {
// entire atmosphere
      grid.level1_type=200;
      grid.level1=grid.level2=0.;
      break;
    }
    default: {
      myerror="unable to convert level type "+strutils::itos(source.first_level_type());
      exit(1);
    }
  }
  m_reference_date_time=source.reference_date_time();
  if (!source.is_averaged_grid()) {
    switch (source.time_marker()) {
      case 0: {
        grib.p1=source.forecast_time()/10000;
        grib.p2=0;
        grib.t_range=10;
        grib.time_unit=1;
        break;
      }
      case 3: {
        if (grid.param == 61) {
          grib.p1=source.F1()-source.F2();
          grib.p2=source.F1();
          grib.t_range=5;
          grib.time_unit=1;
        }
        else {
          myerror="unable to convert time marker "+strutils::itos(source.time_marker());
          exit(1);
        }
        break;
      }
      default: {
        myerror="unable to convert time marker "+strutils::itos(source.time_marker());
        exit(1);
      }
    }
    grid.nmean=grib.nmean_missing=0;
  }
  else {
    myerror="unable to convert a mean grid";
    exit(1);
  }
  grib.sub_center=0;
  switch (source.type()) {
    case 29:
    case 30: {
      grid.grid_type=0;
      grib.scan_mode=0x40;
      break;
    }
    case 27:
    case 28:
    case 36:
    case 47: {
      grid.grid_type=5;
      def.projection_flag=0;
      grib.scan_mode=0x40;
      grib.sub_center=11;
      break;
    }
    default: {
      myerror="unable to convert grid type "+strutils::itos(source.type());
      exit(1);
    }
  }
  dim=source.dimensions();
  def=source.definition();
  grib.rescomp=0x80;
  galloc();
  resize_bitmap(dim.size);
  grid.num_missing=0;
  stats.avg_val=0.;
  stats.max_val=-Grid::MISSING_VALUE;
  stats.min_val=Grid::MISSING_VALUE;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
      m_gridpoints[n][m]=source.gridpoint(m,n);
      if (floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE)) {
        bitmap.map[cnt]=0;
        ++grid.num_missing;
      }
      else {
        m_gridpoints[n][m]*=mult;
        bitmap.map[cnt]=1;
        if (m_gridpoints[n][m] > stats.max_val) {
          stats.max_val=m_gridpoints[n][m];
        }
        if (m_gridpoints[n][m] < stats.min_val) {
          stats.min_val=m_gridpoints[n][m];
        }
        stats.avg_val+=m_gridpoints[n][m];
        ++avg_cnt;
      }
      ++cnt;
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
  grib.pack_width=16;
  max_pack=0x8000;
  range=round((stats.max_val-stats.min_val)*pow(10.,precision));
  while (max_pack > 0 && max_pack > range) {
    grib.pack_width--;
    max_pack/=2;
  }
  if (grid.num_missing == 0) {
    bitmap.applies=false;
  }
  else {
    bitmap.applies=true;
  }
  grid.filled=true;
  return *this;
}

GRIBGrid& GRIBGrid::operator=(const CGCM1Grid& source)
{
  int n,m;
  size_t l,cnt=0,avg_cnt=0;

  grib.table=3;
  grid.src=54;
  grib.process=0;
  grib.grid_catalog_id=255;
  grid.level2_type=-1;
  auto pname=std::string(source.parameter_name());
  if (pname == " ALB") {
    grid.param=84;
    grid.level1_type=1;
    grid.level1=0;
  }
  else if (pname == "  AU") {
    grid.param=33;
    grid.level1_type=105;
    grid.level1=10.;
  }
  else if (pname == "  AV") {
    grid.param=34;
    grid.level1_type=105;
    grid.level1=10.;
  }
  else if (pname == "CLDT") {
    grid.param=71;
    grid.level1_type=200;
    grid.level1=0.;
  }
  else if (pname == " FSR") {
    grid.param=116;
    grid.level1_type=8;
    grid.level1=0.;
  }
  else if (pname == " FSS") {
    grid.param=116;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (pname == "FLAG") {
    grid.param=115;
    grid.level1_type=8;
    grid.level1=0.;
  }
  else if (pname == "FSRG") {
    grid.param=116;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (pname == "  GT") {
    grid.param=11;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (pname == "  PS") {
    grid.param=2;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (pname == " PHI") {
    grid.param=7;
    grid.level1_type=100;
    grid.level1=source.first_level_value();
  }
  else if (pname == " PCP") {
    grid.param=61;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (pname == "PMSL") {
    grid.param=2;
    grid.level1_type=102;
    grid.level1=0.;
  }
  else if (pname == "  SQ") {
    grid.param=51;
    grid.level1_type=105;
    grid.level1=2.;
  }
  else if (pname == "  ST") {
    grid.param=11;
    grid.level1_type=105;
    grid.level1=2.;
  }
  else if (pname == " SNO") {
    grid.param=79;
    grid.level1_type=1;
    grid.level1=0.;
  }
  else if (pname == "SHUM") {
    grid.param=51;
    grid.level1_type=100;
    grid.level1=source.first_level_value();
  }
  else if (pname == "STMX") {
    grid.param=15;
    grid.level1_type=105;
    grid.level1=2.;
  }
  else if (pname == "STMN") {
    grid.param=16;
    grid.level1_type=105;
    grid.level1=2.;
  }
  else if (pname == "SWMX") {
    grid.param=32;
    grid.level1_type=105;
    grid.level1=10.;
  }
  else if (pname == "TEMP") {
    grid.param=11;
    grid.level1_type=100;
    grid.level1=source.first_level_value();
  }
  else if (pname == "   U") {
    grid.param=33;
    grid.level1_type=100;
    grid.level1=source.first_level_value();
  }
  else if (pname == "   V") {
    grid.param=34;
    grid.level1_type=100;
    grid.level1=source.first_level_value();
  }
  else {
    myerror="unable to convert parameter "+pname;
    exit(1);
  }
  grid.level2=0.;
  m_reference_date_time=source.reference_date_time();
  if (m_reference_date_time.day() == 0) {
    m_reference_date_time.set_day(1);
    grib.t_range=113;
    grib.time_unit=3;
  }
  else {
    grib.t_range=1;
    grib.time_unit=1;
  }
  grib.p1=0;
  grib.p2=0;
  grid.nmean=0;
  grib.nmean_missing=0;
  grib.sub_center=0;
  grid.grid_type=4;
  dim=source.dimensions();
  --dim.x;
  def=source.definition();
  def.type=Type::gaussianLatitudeLongitude;
  grib.rescomp=0x80;
  grib.scan_mode=0x0;
  dim.size=dim.x*dim.y;
  galloc();
  resize_bitmap(dim.size);
  grid.num_missing=0;
  stats.max_val=-Grid::MISSING_VALUE;
  stats.min_val=Grid::MISSING_VALUE;
  stats.avg_val=0.;
  l=dim.y-1;
  for (n=0; n < dim.y; ++n,l--) {
    for (m=0; m < dim.x; ++m) {
      m_gridpoints[n][m]=source.gridpoint(m,l);
      if (floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE)) {
        bitmap.map[cnt]=0;
        ++grid.num_missing;
      }
      else {
        bitmap.map[cnt]=1;
        switch (grid.param) {
          case 2: {
            m_gridpoints[n][m]*=100.;
            break;
          }
          case 11:
          case 15:
          case 16: {
            m_gridpoints[n][m]+=273.15;
            break;
          }
          case 61: {
            m_gridpoints[n][m]*=86400.;
            break;
          }
        }
        if (m_gridpoints[n][m] > stats.max_val) {
          stats.max_val=m_gridpoints[n][m];
        }
        if (m_gridpoints[n][m] < stats.min_val) {
          stats.min_val=m_gridpoints[n][m];
        }
        stats.avg_val+=m_gridpoints[n][m];
        ++avg_cnt;
      }
      ++cnt;
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
  if (grid.param == 2) {
    grib.D=0;
  }
  else {
    grib.D=2;
  }
  grib.pack_width=16;
  if (grid.num_missing == 0) {
    bitmap.applies=false;
  }
  else {
    bitmap.applies=true;
  }
  grid.filled=true;
  return *this;
}

bool GRIBGrid::is_averaged_grid() const
{
  switch (grib.t_range) {
    case 51:
    case 113:
    case 115:
    case 117:
    case 123: {
      return true;
    }
    default: {
      return false;
    }
  }
}

GRIBGrid& GRIBGrid::operator=(const USSRSLPGrid& source)
{
  int n,m;
  size_t cnt=0,avg_cnt=0;
  size_t max_pack;
  double range;

  grib.table=3;
  grid.src=4;
  grib.process=0;
  grib.grid_catalog_id=255;
  grid.param=2;
  grid.level1_type=102;
  grid.level1=0.;
  grid.level2_type=-1;
  grid.level2=0.;
  grib.p1=0;
  grib.p2=0;
  m_reference_date_time=source.reference_date_time();
  grib.time_unit=1;
  grib.t_range=10;
  grid.nmean=0;
  grib.nmean_missing=0;
  grib.sub_center=0;
  grib.D=2;
  grid.grid_type=0;
  grib.scan_mode=0x0;
  dim=source.dimensions();
  dim.size=dim.x*dim.y;
  def=source.definition();
  grib.rescomp=0x80;
  galloc();
  resize_bitmap(dim.size);
  grid.num_missing=0;
  stats.avg_val=0.;
  stats.min_val=Grid::MISSING_VALUE;
  stats.max_val=-Grid::MISSING_VALUE;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
      m_gridpoints[n][m]=source.gridpoint(m,n);
      if (floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE)) {
        bitmap.map[cnt]=0;
        ++grid.num_missing;
      }
      else {
        bitmap.map[cnt]=1;
        if (m_gridpoints[n][m] > stats.max_val) {
          stats.max_val=m_gridpoints[n][m];
        }
        if (m_gridpoints[n][m] < stats.min_val) {
          stats.min_val=m_gridpoints[n][m];
        }
        stats.avg_val+=m_gridpoints[n][m];
        ++avg_cnt;
      }
      ++cnt;
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<double>(avg_cnt);
  }
  grib.pack_width=16;
  max_pack=0x8000;
  range=round(stats.max_val-stats.min_val);
  while (max_pack > 0 && max_pack > range) {
    grib.pack_width--;
    max_pack/=2;
  }
  if (grid.num_missing == 0) {
    bitmap.applies=false;
  }
  else {
    bitmap.applies=true;
  }
  grid.filled=true;
  return *this;
}

short mapped_parameter_data(short center,size_t disc,short param_cat,short param_num,short& table)
{
  switch (disc) {
    case 0: {
// meteorological products
      switch (param_cat) {
        case 0: {
// temperature parameters
          switch (param_num) {
            case 0: {
              return 11;
            }
            case 1: {
              return 12;
            }
            case 2: {
              return 13;
            }
            case 3: {
              return 14;
            }
            case 4: {
              return 15;
            }
            case 5: {
              return 16;
            }
            case 6: {
              return 17;
            }
            case 7: {
              return 18;
            }
            case 8: {
              return 19;
            }
            case 9: {
              return 25;
            }
            case 10: {
              return 121;
            }
            case 11: {
              return 122;
            }
            case 12: {
              myerror="there is no GRIB1 parameter code for 'Heat index'";
              exit(1);
            }
            case 13: {
              myerror="there is no GRIB1 parameter code for 'Wind chill factor'";
              exit(1);
            }
            case 14: {
              myerror="there is no GRIB1 parameter code for 'Minimum dew point depression'";
              exit(1);
            }
            case 15: {
              myerror="there is no GRIB1 parameter code for 'Virtual potential temperature'";
              exit(1);
            }
            case 16: {
              myerror="there is no GRIB1 parameter code for 'Snow phase change heat flux'";
              exit(1);
            }
            case 21: {
              switch (center) {
                case 7: {
                  table=131;
                  return 193;
                }
              }
              myerror="there is no GRIB1 parameter code for 'Apparent temperature'";
              exit(1);
            }
            case 192: {
              switch (center) {
                case 7: {
                  return 229;
                }
              }
            }
          }
          break;
        }
        case 1: {
// moisture parameters
          switch (param_num) {
            case 0: {
              return 51;
            }
            case 1: {
              return 52;
            }
            case 2: {
              return 53;
            }
            case 3: {
              return 54;
            }
            case 4: {
              return 55;
            }
            case 5: {
              return 56;
            }
            case 6: {
              return 57;
            }
            case 7: {
              return 59;
            }
            case 8: {
              return 61;
            }
            case 9: {
              return 62;
            }
            case 10: {
              return 63;
            }
            case 11: {
              return 66;
            }
            case 12: {
              return 64;
            }
            case 13: {
              return 65;
            }
            case 14: {
              return 78;
            }
            case 15: {
              return 79;
            }
            case 16: {
              return 99;
            }
            case 17: {
              myerror="there is no GRIB1 parameter code for 'Snow age'";
              exit(1);
            }
            case 18: {
              myerror="there is no GRIB1 parameter code for 'Absolute humidity'";
              exit(1);
            }
            case 19: {
              myerror="there is no GRIB1 parameter code for 'Precipitation type'";
              exit(1);
            }
            case 20: {
              myerror="there is no GRIB1 parameter code for 'Integrated liquid water'";
              exit(1);
            }
            case 21: {
              myerror="there is no GRIB1 parameter code for 'Condensate water'";
              exit(1);
            }
            case 22: {
              switch (center) {
                case 7: {
                  return 153;
                }
              }
              myerror="there is no GRIB1 parameter code for 'Cloud mixing ratio'";
              exit(1);
            }
            case 23: {
              myerror="there is no GRIB1 parameter code for 'Ice water mixing ratio'";
              exit(1);
            }
            case 24: {
              myerror="there is no GRIB1 parameter code for 'Rain mixing ratio'";
              exit(1);
            }
            case 25: {
              myerror="there is no GRIB1 parameter code for 'Snow mixing ratio'";
              exit(1);
            }
            case 26: {
              myerror="there is no GRIB1 parameter code for 'Horizontal moisture convergence'";
              exit(1);
            }
            case 27: {
              myerror="there is no GRIB1 parameter code for 'Maximum relative humidity'";
              exit(1);
            }
            case 28: {
              myerror="there is no GRIB1 parameter code for 'Maximum absolute humidity'";
              exit(1);
            }
            case 29: {
              myerror="there is no GRIB1 parameter code for 'Total snowfall'";
              exit(1);
            }
            case 30: {
              myerror="there is no GRIB1 parameter code for 'Precipitable water category'";
              exit(1);
            }
            case 31: {
              myerror="there is no GRIB1 parameter code for 'Hail'";
              exit(1);
            }
            case 32: {
              myerror="there is no GRIB1 parameter code for 'Graupel (snow pellets)'";
              exit(1);
            }
            case 33: {
              myerror="there is no GRIB1 parameter code for 'Categorical rain'";
              exit(1);
            }
            case 34: {
              myerror="there is no GRIB1 parameter code for 'Categorical freezing rain'";
              exit(1);
            }
            case 35: {
              myerror="there is no GRIB1 parameter code for 'Categorical ice pellets'";
              exit(1);
            }
            case 36: {
              myerror="there is no GRIB1 parameter code for 'Categorical snow'";
              exit(1);
            }
            case 37: {
              myerror="there is no GRIB1 parameter code for 'Convective precipitation rate'";
              exit(1);
            }
            case 38: {
              myerror="there is no GRIB1 parameter code for 'Horizontal moisture divergence'";
              exit(1);
            }
            case 39: {
              switch (center) {
                case 7: {
                  return 194;
                }
              }
              myerror="there is no GRIB1 parameter code for 'Percent frozen precipitation'";
              exit(1);
            }
            case 40: {
              myerror="there is no GRIB1 parameter code for 'Potential evaporation'";
              exit(1);
            }
            case 41: {
              myerror="there is no GRIB1 parameter code for 'Potential evaporation rate'";
              exit(1);
            }
            case 42: {
              myerror="there is no GRIB1 parameter code for 'Snow cover'";
              exit(1);
            }
            case 43: {
              myerror="there is no GRIB1 parameter code for 'Rain fraction of total water'";
              exit(1);
            }
            case 44: {
              myerror="there is no GRIB1 parameter code for 'Rime factor'";
              exit(1);
            }
            case 45: {
              myerror="there is no GRIB1 parameter code for 'Total column integrated rain'";
              exit(1);
            }
            case 46: {
              myerror="there is no GRIB1 parameter code for 'Total column integrated snow'";
              exit(1);
            }
            case 192: {
              switch (center) {
                case 7: {
                  return 140;
                }
              }
            }
            case 193: {
              switch (center) {
                case 7: {
                  return 141;
                }
              }
            }
            case 194: {
              switch (center) {
                case 7: {
                  return 142;
                }
              }
            }
            case 195: {
              switch (center) {
                case 7: {
                  return 143;
                }
              }
            }
            case 196: {
              switch (center) {
                case 7: {
                  return 214;
                }
              }
            }
            case 197: {
              switch (center) {
                case 7: {
                  return 135;
                }
              }
            }
            case 199: {
              switch (center) {
                case 7: {
                  return 228;
                }
              }
            }
            case 200: {
              switch (center) {
                case 7: {
                  return 145;
                }
              }
            }
            case 201: {
              switch (center) {
                case 7: {
                  return 238;
                }
              }
            }
            case 206: {
              switch (center) {
                case 7: {
                  return 186;
                }
              }
            }
            case 207: {
              switch (center) {
                case 7: {
                  return 198;
                }
              }
            }
            case 208: {
              switch (center) {
                case 7: {
                  return 239;
                }
              }
            }
            case 213: {
              switch (center) {
                case 7: {
                  return 243;
                }
              }
            }
            case 214: {
              switch (center) {
                case 7: {
                  return 245;
                }
              }
            }
            case 215: {
              switch (center) {
                case 7: {
                  return 249;
                }
              }
            }
            case 216: {
              switch (center) {
                case 7: {
                  return 159;
                }
              }
            }
          }
          break;
        }
        case 2: {
// momentum parameters
          switch(param_num) {
            case 0: {
              return 31;
            }
            case 1: {
              return 32;
            }
            case 2: {
              return 33;
            }
            case 3: {
              return 34;
            }
            case 4: {
              return 35;
            }
            case 5: {
              return 36;
            }
            case 6: {
              return 37;
            }
            case 7: {
              return 38;
            }
            case 8: {
              return 39;
            }
            case 9: {
              return 40;
            }
            case 10: {
              return 41;
            }
            case 11: {
              return 42;
            }
            case 12: {
              return 43;
            }
            case 13: {
              return 44;
            }
            case 14: {
              return 4;
            }
            case 15: {
              return 45;
            }
            case 16: {
              return 46;
            }
            case 17: {
              return 124;
            }
            case 18: {
              return 125;
            }
            case 19: {
              return 126;
            }
            case 20: {
              return 123;
            }
            case 21: {
              myerror="there is no GRIB1 parameter code for 'Maximum wind speed'";
              exit(1);
            }
            case 22: {
              switch (center) {
                case 7: return 180;
                default: {
                  myerror="there is no GRIB1 parameter code for 'Wind speed (gust)'";
                  exit(1);
                }
              }
            }
            case 23: {
              myerror="there is no GRIB1 parameter code for 'u-component of wind (gust)'";
              exit(1);
            }
            case 24: {
              myerror="there is no GRIB1 parameter code for 'v-component of wind (gust)'";
              exit(1);
            }
            case 25: {
              myerror="there is no GRIB1 parameter code for 'Vertical speed shear'";
              exit(1);
            }
            case 26: {
              myerror="there is no GRIB1 parameter code for 'Horizontal momentum flux'";
              exit(1);
            }
            case 27: {
              myerror="there is no GRIB1 parameter code for 'u-component storm motion'";
              exit(1);
            }
            case 28: {
              myerror="there is no GRIB1 parameter code for 'v-component storm motion'";
              exit(1);
            }
            case 29: {
              myerror="there is no GRIB1 parameter code for 'Drag coefficient'";
              exit(1);
            }
            case 30: {
              myerror="there is no GRIB1 parameter code for 'Frictional velocity'";
              exit(1);
            }
            case 192: {
              switch (center) {
                case 7: {
                  return 136;
                }
              }
            }
            case 193: {
              switch (center) {
                case 7: {
                  return 172;
                }
              }
            }
            case 194: {
              switch (center) {
                case 7: {
                  return 196;
                }
              }
            }
            case 195: {
              switch (center) {
                case 7: {
                  return 197;
                }
              }
            }
            case 196: {
              switch (center) {
                case 7: {
                  return 252;
                }
              }
            }
            case 197: {
              switch (center) {
                case 7: {
                  return 253;
                }
              }
            }
            case 224: {
              switch (center) {
                case 7: {
                  table=129;
                  return 241;
                }
              }
            }
          }
          break;
        }
        case 3: {
// mass parameters
          switch (param_num) {
            case 0: {
              return 1;
            }
            case 1: {
              return 2;
            }
            case 2: {
              return 3;
            }
            case 3: {
              return 5;
            }
            case 4: {
              return 6;
            }
            case 5: {
              return 7;
            }
            case 6: {
              return 8;
            }
            case 7: {
              return 9;
            }
            case 8: {
              return 26;
            }
            case 9: {
              return 27;
            }
            case 10: {
              return 89;
            }
            case 11: {
              myerror="there is no GRIB1 parameter code for 'Altimeter setting'";
              exit(1);
            }
            case 12: {
              myerror="there is no GRIB1 parameter code for 'Thickness'";
              exit(1);
            }
            case 13: {
              myerror="there is no GRIB1 parameter code for 'Pressure altitude'";
              exit(1);
            }
            case 14: {
              myerror="there is no GRIB1 parameter code for 'Density altitude'";
              exit(1);
            }
            case 15: {
              myerror="there is no GRIB1 parameter code for '5-wave geopotential height'";
              exit(1);
            }
            case 16: {
              myerror="there is no GRIB1 parameter code for 'Zonal flux of gravity wave stress'";
              exit(1);
            }
            case 17: {
              myerror="there is no GRIB1 parameter code for 'Meridional flux of gravity wave stress'";
              exit(1);
            }
            case 18: {
              myerror="there is no GRIB1 parameter code for 'Planetary boundary layer height'";
              exit(1);
            }
            case 19: {
              myerror="there is no GRIB1 parameter code for '5-wave geopotential height anomaly'";
              exit(1);
            }
            case 192: {
              switch (center) {
                case 7: {
                  return 130;
                }
              }
            }
            case 193: {
              switch (center) {
                case 7: {
                  return 222;
                }
              }
            }
            case 194: {
              switch (center) {
                case 7: {
                  return 147;
                }
              }
            }
            case 195: {
              switch (center) {
                case 7: {
                  return 148;
                }
              }
            }
            case 196: {
              switch (center) {
                case 7: {
                  return 221;
                }
              }
            }
            case 197: {
              switch (center) {
                case 7: {
                  return 230;
                }
              }
            }
            case 198: {
              switch (center) {
                case 7: {
                  return 129;
                }
              }
            }
            case 199: {
              switch (center) {
                case 7: {
                  return 137;
                }
              }
            }
            case 200: {
              switch (center) {
                case 7: {
                  table=129;
                  return 141;
                }
              }
            }
          }
          break;
        }
        case 4: {
// short-wave radiation parameters
          switch (param_num) {
            case 0: {
              return 111;
            }
            case 1: {
              return 113;
            }
            case 2: {
              return 116;
            }
            case 3: {
              return 117;
            }
            case 4: {
              return 118;
            }
            case 5: {
              return 119;
            }
            case 6: {
              return 120;
            }
            case 7: {
              myerror="there is no GRIB1 parameter code for 'Downward short-wave radiation flux'";
              exit(1);
            }
            case 8: {
              myerror="there is no GRIB1 parameter code for 'Upward short-wave radiation flux'";
              exit(1);
            }
            case 192: {
              switch (center) {
                case 7: {
                  return 204;
                }
              }
            }
            case 193: {
              switch (center) {
                case 7: {
                  return 211;
                }
              }
            }
            case 196: {
              switch (center) {
                case 7: {
                  return 161;
                }
              }
            }
          }
          break;
        }
        case 5: {
// long-wave radiation parameters
          switch (param_num) {
            case 0: {
              return 112;
            }
            case 1: {
              return 114;
            }
            case 2: {
              return 115;
            }
            case 3: {
              myerror="there is no GRIB1 parameter code for 'Downward long-wave radiation flux'";
              exit(1);
            }
            case 4: {
              myerror="there is no GRIB1 parameter code for 'Upward long-wave radiation flux'";
              exit(1);
            }
            case 192: {
              switch (center) {
                case 7: {
                  return 205;
                }
              }
            }
            case 193: {
              switch (center) {
                case 7: {
                  return 212;
                }
              }
            }
          }
          break;
        }
        case 6: {
// cloud parameters
          switch (param_num) {
            case 0: {
              return 58;
            }
            case 1: {
              return 71;
            }
            case 2: {
              return 72;
            }
            case 3: {
              return 73;
            }
            case 4: {
              return 74;
            }
            case 5: {
              return 75;
            }
            case 6: {
              return 76;
            }
            case 7: {
              myerror="there is no GRIB1 parameter code for 'Cloud amount'";
              exit(1);
            }
            case 8: {
              myerror="there is no GRIB1 parameter code for 'Cloud type'";
              exit(1);
            }
            case 9: {
              myerror="there is no GRIB1 parameter code for 'Thunderstorm maximum tops'";
              exit(1);
            }
            case 10: {
              myerror="there is no GRIB1 parameter code for 'Thunderstorm coverage'";
              exit(1);
            }
            case 11: {
              myerror="there is no GRIB1 parameter code for 'Cloud base'";
              exit(1);
            }
            case 12: {
              myerror="there is no GRIB1 parameter code for 'Cloud top'";
              exit(1);
            }
            case 13: {
              myerror="there is no GRIB1 parameter code for 'Ceiling'";
              exit(1);
            }
            case 14: {
              myerror="there is no GRIB1 parameter code for 'Non-convective cloud cover'";
              exit(1);
            }
            case 15: {
              myerror="there is no GRIB1 parameter code for 'Cloud work function'";
              exit(1);
            }
            case 16: {
              myerror="there is no GRIB1 parameter code for 'Convective cloud efficiency'";
              exit(1);
            }
            case 17: {
              myerror="there is no GRIB1 parameter code for 'Total condensate'";
              exit(1);
            }
            case 18: {
              myerror="there is no GRIB1 parameter code for 'Total column-integrated cloud water'";
              exit(1);
            }
            case 19: {
              myerror="there is no GRIB1 parameter code for 'Total column-integrated cloud ice'";
              exit(1);
            }
            case 20: {
              myerror="there is no GRIB1 parameter code for 'Total column-integrated cloud condensate'";
              exit(1);
            }
            case 21: {
              myerror="there is no GRIB1 parameter code for 'Ice fraction of total condensate'";
              exit(1);
            }
            case 192: {
              switch (center) {
                case 7: {
                  return 213;
                }
              }
            }
            case 193: {
              switch (center) {
                case 7: {
                  return 146;
                }
              }
            }
            case 201: {
              switch (center) {
                case 7: {
                  table=133;
                  return 191;
                }
              }
            }
          }
          break;
        }
        case 7: {
// thermodynamic stability index parameters
          switch (param_num) {
            case 0: {
              return 24;
            }
            case 1: {
              return 77;
            }
            case 2: {
              myerror="there is no GRIB1 parameter code for 'K index'";
              exit(1);
            }
            case 3: {
              myerror="there is no GRIB1 parameter code for 'KO index'";
              exit(1);
            }
            case 4: {
              myerror="there is no GRIB1 parameter code for 'Total totals index'";
              exit(1);
            }
            case 5: {
              myerror="there is no GRIB1 parameter code for 'Sweat index'";
              exit(1);
            }
            case 6: {
              switch (center) {
                case 7: {
                  return 157;
                }
                default: {
                  myerror="there is no GRIB1 parameter code for 'Convective available potential energy'";
                  exit(1);
                }
              }
            }
            case 7: {
              switch (center) {
                case 7: {
                  return 156;
                }
                default: {
                  myerror="there is no GRIB1 parameter code for 'Convective inhibition'";
                  exit(1);
                }
              }
            }
            case 8: {
              switch (center) {
                case 7: {
                  return 190;
                }
                default: {
                  myerror="there is no GRIB1 parameter code for 'Storm-relative helicity'";
                  exit(1);
                }
              }
            }
            case 9: {
              myerror="there is no GRIB1 parameter code for 'Energy helicity index'";
              exit(1);
            }
            case 10: {
              myerror="there is no GRIB1 parameter code for 'Surface lifted index'";
              exit(1);
            }
            case 11: {
              myerror="there is no GRIB1 parameter code for 'Best (4-layer) lifted index'";
              exit(1);
            }
            case 12: {
              myerror="there is no GRIB1 parameter code for 'Richardson number'";
              exit(1);
            }
            case 192: {
              switch (center) {
                case 7: {
                  return 131;
                }
              }
            }
            case 193: {
              switch (center) {
                case 7: {
                  return 132;
                }
              }
            }
            case 194: {
              switch (center) {
                case 7: {
                  return 254;
                }
              }
            }
          }
          break;
        }
        case 13: {
// aerosol parameters
          switch (param_num) {
            case 0: {
              myerror="there is no GRIB1 parameter code for 'Aerosol type'";
              exit(1);
            }
          }
          break;
        }
        case 14: {
// trace gas parameters
          switch (param_num) {
            case 0: {
              return 10;
            }
            case 1: {
              myerror="there is no GRIB1 parameter code for 'Ozone mixing ratio'";
              exit(1);
            }
            case 192: {
              switch (center) {
                case 7: {
                  return 154;
                }
              }
            }
          }
          break;
        }
        case 15: {
// radar parameters
          switch (param_num) {
            case 0: {
              myerror="there is no GRIB1 parameter code for 'Base spectrum width'";
              exit(1);
            }
            case 1: {
              myerror="there is no GRIB1 parameter code for 'Base reflectivity'";
              exit(1);
            }
            case 2: {
              myerror="there is no GRIB1 parameter code for 'Base radial velocity'";
              exit(1);
            }
            case 3: {
              myerror="there is no GRIB1 parameter code for 'Vertically-integrated liquid'";
              exit(1);
            }
            case 4: {
              myerror="there is no GRIB1 parameter code for 'Layer-maximum base reflectivity'";
              exit(1);
            }
            case 5: {
              myerror="there is no GRIB1 parameter code for 'Radar precipitation'";
              exit(1);
            }
            case 6: {
              return 21;
            }
            case 7: {
              return 22;
            }
            case 8: {
              return 23;
            }
          }
          break;
        }
        case 18: {
// nuclear/radiology parameters
          break;
        }
        case 19: {
// physical atmospheric property parameters
          switch (param_num) {
            case 0: {
              return 20;
            }
            case 1: {
              return 84;
            }
            case 2: {
              return 60;
            }
            case 3: {
              return 67;
            }
            case 4: {
              myerror="there is no GRIB1 parameter code for 'Volcanic ash'";
              exit(1);
            }
            case 5: {
              myerror="there is no GRIB1 parameter code for 'Icing top'";
              exit(1);
            }
            case 6: {
              myerror="there is no GRIB1 parameter code for 'Icing base'";
              exit(1);
            }
            case 7: {
              myerror="there is no GRIB1 parameter code for 'Icing'";
              exit(1);
            }
            case 8: {
              myerror="there is no GRIB1 parameter code for 'Turbulence top'";
              exit(1);
            }
            case 9: {
              myerror="there is no GRIB1 parameter code for 'Turbulence base'";
              exit(1);
            }
            case 10: {
              myerror="there is no GRIB1 parameter code for 'Turbulence'";
              exit(1);
            }
            case 11: {
              myerror="there is no GRIB1 parameter code for 'Turbulent kinetic energy'";
              exit(1);
            }
            case 12: {
              myerror="there is no GRIB1 parameter code for 'Planetary boundary layer regime'";
              exit(1);
            }
            case 13: {
              myerror="there is no GRIB1 parameter code for 'Contrail intensity'";
              exit(1);
            }
            case 14: {
              myerror="there is no GRIB1 parameter code for 'Contrail engine type'";
              exit(1);
            }
            case 15: {
              myerror="there is no GRIB1 parameter code for 'Contrail top'";
              exit(1);
            }
            case 16: {
              myerror="there is no GRIB1 parameter code for 'Contrail base'";
              exit(1);
            }
            case 17: {
              myerror="there is no GRIB1 parameter code for 'Maximum snow albedo'";
              exit(1);
            }
            case 18: {
              myerror="there is no GRIB1 parameter code for 'Snow-free albedo'";
              exit(1);
            }
            case 204: {
              switch (center) {
                case 7: {
                  return 209;
                }
              }
            }
          }
          break;
        }
      }
      break;
    }
    case 1: {
// hydrologic products
      switch (param_cat) {
        case 0: {
// hydrology basic products
          switch (param_num) {
            case 192: {
              switch (center) {
                case 7: {
                  return 234;
                }
              }
            }
            case 193: {
              switch (center) {
                case 7: {
                  return 235;
                }
              }
            }
          }
        }
        case 1: {
          switch (param_num) {
            case 192: {
              switch (center) {
                case 7: {
                  return 194;
                }
              }
            }
            case 193: {
              switch (center) {
                case 7: {
                  return 195;
                }
              }
            }
          }
          break;
        }
      }
    }
    case 2: {
// land surface products
      switch (param_cat) {
        case 0: {
// vegetation/biomass
          switch (param_num) {
            case 0: {
              return 81;
            }
            case 1: {
              return 83;
            }
            case 2: {
              return 85;
            }
            case 3: {
              return 86;
            }
            case 4: {
              return 87;
            }
            case 5: {
              return 90;
            }
            case 192: {
              switch (center) {
                case 7: {
                  return 144;
                }
              }
            }
            case 193: {
              switch (center) {
                case 7: {
                  return 155;
                }
              }
            }
            case 194: {
              switch (center) {
                case 7: {
                  return 207;
                }
              }
            }
            case 195: {
              switch (center) {
                case 7: {
                  return 208;
                }
              }
            }
            case 196: {
              switch (center) {
                case 7: {
                  return 223;
                }
              }
            }
            case 197: {
              switch (center) {
                case 7: {
                  return 226;
                }
              }
            }
            case 198: {
              switch (center) {
                case 7: {
                  return 225;
                }
              }
            }
            case 201: {
              switch (center) {
                case 7: {
                  table=130;
                  return 219;
                }
              }
            }
            case 207: {
              switch (center) {
                case 7: {
                  return 201;
                }
              }
            }
          }
          break;
        }
        case 3: {
// soil products
           switch (param_num) {
            case 203: {
              switch (center) {
                case 7: {
                  table=130;
                  return 220;
                }
              }
            }
           }
          break;
        }
        case 4: {
// fire weather
          switch (param_num) {
            case 2: {
              switch (center) {
                case 7: {
                  table=129;
                  return 250;
                }
              }
            }
          }
          break;
        }
      }
      break;
    }
    case 10: {
// oceanographic products
      switch (param_cat) {
        case 0: {
// waves parameters
          switch (param_num) {
            case 0: {
              return 28;
            }
            case 1: {
              return 29;
            }
            case 2: {
              return 30;
            }
            case 3: {
              return 100;
            }
            case 4: {
              return 101;
            }
            case 5: {
              return 102;
            }
            case 6: {
              return 103;
            }
            case 7: {
              return 104;
            }
            case 8: {
              return 105;
            }
            case 9: {
              return 106;
            }
            case 10: {
              return 107;
            }
            case 11: {
              return 108;
            }
            case 12: {
              return 109;
            }
            case 13: {
              return 110;
            }
          }
          break;
        }
        case 1: {
// currents parameters
          switch (param_num) {
            case 0: {
              return 47;
            }
            case 1: {
              return 48;
            }
            case 2: {
              return 49;
            }
            case 3: {
              return 50;
            }
          }
          break;
        }
        case 2: {
// ice parameters
          switch (param_num) {
            case 0: {
              return 91;
            }
            case 1: {
              return 92;
            }
            case 2: {
              return 93;
            }
            case 3: {
              return 94;
            }
            case 4: {
              return 95;
            }
            case 5: {
              return 96;
            }
            case 6: {
              return 97;
            }
            case 7: {
              return 98;
            }
          }
          break;
        }
        case 3: {
// surface properties parameters
          switch (param_num) {
            case 0: {
              return 80;
            }
            case 1: {
              return 82;
            }
          }
          break;
        }
        case 4: {
// sub-surface properties parameters
          switch (param_num) {
            case 0: {
              return 69;
            }
            case 1: {
              return 70;
            }
            case 2: {
              return 68;
            }
            case 3: {
              return 88;
            }
          }
          break;
        }
      }
      break;
    }
  }
  myerror="there is no GRIB1 parameter code for discipline "+strutils::itos(disc)+", parameter category "+strutils::itos(param_cat)+", parameter number "+strutils::itos(param_num);
  exit(1);
}

void fill_level_data(const GRIB2Grid& grid,short& level_type,double& level1,double& level2)
{
  if (grid.second_level_type() != 255 && grid.first_level_type() != grid.second_level_type()) {
    myerror="unable to indicate a layer bounded by different level types "+strutils::itos(grid.first_level_type())+" and "+strutils::itos(grid.second_level_type())+" in GRIB1";
    exit(1);
  }
  level1=level2=0.;
  switch (grid.first_level_type()) {
    case 1: {
      level_type=1;
      break;
    }
    case 2: {
      level_type=2;
      break;
    }
    case 3: {
      level_type=3;
      break;
    }
    case 4: {
      level_type=4;
      break;
    }
    case 5: {
      level_type=5;
      break;
    }
    case 6: {
      level_type=6;
      break;
    }
    case 7: {
      level_type=7;
      break;
    }
    case 8: {
      level_type=8;
      break;
    }
    case 9: {
      level_type=9;
      break;
    }
    case 20: {
      level_type=20;
      break;
    }
    case 100: {
      if (grid.second_level_type() == 255) {
        level_type=100;
        level1=grid.first_level_value();
      }
      else {
        level_type=101;
        level1=grid.first_level_value()/10.;
        level2=grid.second_level_value()/10.;
      }
      break;
    }
    case 101: {
      level_type=102;
      break;
    }
    case 102: {
      if (grid.second_level_type() == 255) {
        level_type=103;
        level1=grid.first_level_value();
      }
      else {
        level_type=104;
        level1=grid.first_level_value()/100.;
        level2=grid.second_level_value()/100.;
      }
      break;
    }
    case 103: {
      if (grid.second_level_type() == 255) {
        level_type=105;
        level1=grid.first_level_value();
      }
      else {
        level_type=106;
        level1=grid.first_level_value()/100.;
        level2=grid.second_level_value()/100.;
      }
      break;
    }
    case 104: {
      if (grid.second_level_type() == 255) {
        level_type=107;
        level1=grid.first_level_value()*10000.;
      }
      else {
        level_type=108;
        level1=grid.first_level_value()*100.;
        level2=grid.second_level_value()*100.;
      }
      break;
    }
    case 105: {
      level1=grid.first_level_value();
      if (floatutils::myequalf(grid.second_level_value(),255.)) {
        level_type=109;
      }
      else {
        level_type=110;
        level2=grid.second_level_value();
      }
      break;
    }
    case 106: {
      level1=grid.first_level_value()*100.;
      if (grid.second_level_type() == 255) {
        level_type=111;
      }
      else {
        level_type=112;
        level2=grid.second_level_value()*100.;
      }
      break;
    }
    case 107: {
      if (grid.second_level_type() == 255) {
        level_type=113;
        level1=grid.first_level_value();
      }
      else {
        level_type=114;
        level1=475.-grid.first_level_value();
        level2=475.-grid.second_level_value();
      }
      break;
    }
    case 108: {
      level1=grid.first_level_value();
      if (grid.second_level_type() == 255) {
        level_type=115;
      }
      else {
        level_type=116;
        level2=grid.second_level_value();
      }
      break;
    }
    case 109: {
      level_type=117;
      level1=grid.first_level_value();
      break;
    }
    case 111: {
      if (grid.second_level_type() == 255) {
        level_type=119;
        level1=grid.first_level_value()*10000.;
      }
      else {
        level_type=120;
        level1=grid.first_level_value()*100.;
        level2=grid.second_level_value()*100.;
      }
      break;
    }
    case 117: {
      myerror="there is no GRIB1 level code for 'Mixed layer depth'";
      exit(1);
    }
    case 160: {
      level_type=160;
      level1=grid.first_level_value();
      break;
    }
    case 200: {
      switch (grid.source()) {
        case 7: {
          level_type=200;
          break;
        }
      }
      break;
    }
  }
}

void fill_time_range_data(const GRIB2Grid& grid, short& p1, short& p2, short&
    t_range, short& nmean, short& nmean_missing) {
  switch (grid.product_type()) {
    case 0:
    case 1:
    case 2: {
      t_range = 0;
      p1 = grid.forecast_time();
      p2 = 0;
      nmean = 0;
      nmean_missing = 0;
      break;
    }
    case 8:
    case 11:
    case 12: {
      if (grid.number_of_statistical_process_ranges() > 1) {
        myerror = "unable to map multiple (" + to_string(
            grid.number_of_statistical_process_ranges()) + ") statistical "
            "processes to GRIB1";
        exit(1);
      }
      auto& stat_process_ranges = grid.statistical_process_ranges();
      for (size_t n = 0; n < stat_process_ranges.size(); ++n) {
        switch(stat_process_ranges[n].type) {
          case 0:
          case 1:
          case 4: {
            switch(stat_process_ranges[n].type) {
              case 0: {
                // average
                t_range = 3;
                break;
              }
              case 1: {
                // accumulation
                t_range = 4;
                break;
              }
              case 4: {
                // difference
                t_range = 5;
                break;
              }
            }
            p1 = grid.forecast_time();
            p2 = grib_utils::p2_from_statistical_end_time(grid);
            if (stat_process_ranges[n].period_time_increment.value == 0) {
              nmean = 0;
            } else {
              myerror = "unable to map discrete processing to GRIB1";
              exit(1);
            }
            break;
          }
          case 2:
          case 3: {
            // maximum
            // minimum
            t_range = 2;
            p1 = grid.forecast_time();
            p2 = grib_utils::p2_from_statistical_end_time(grid);
            if (stat_process_ranges[n].period_time_increment.value == 0) {
              nmean = 0;
            } else {
              myerror = "unable to map discrete processing to GRIB1";
              exit(1);
            }
            break;
          }
          default: {
            // patch for NCEP grids
            if (stat_process_ranges[n].type == 255 && grid.source() == 7) {
               if (grid.discipline() == 0) {
                if (grid.parameter_category() == 0) {
                  switch (grid.parameter()) {
                    case 4:
                    case 5: {
                      t_range = 2;
                      p1 = grid.forecast_time();
                      p2 = grib_utils::p2_from_statistical_end_time(grid);
                      if (stat_process_ranges[n].period_time_increment.value ==
                          0) {
                        nmean = 0;
                      } else {
                        myerror = "unable to map discrete processing to GRIB1";
                        exit(1);
                      }
                      break;
                    }
                  }
                }
              }
            } else {
              myerror = "unable to map statistical process " + to_string(
                  stat_process_ranges[n].type) + " to GRIB1";
              exit(1);
            }
          }
        }
      }
      nmean_missing = grid.number_missing_from_statistical_process();
      break;
    }
    default: {
      myerror = "unable to map time range for Product Definition Template " +
          to_string(grid.product_type()) + " into GRIB1";
      exit(1);
    }
  }
}

GRIBGrid& GRIBGrid::operator=(const GRIB2Grid& source)
{
  int n,m;

  grib=source.grib;
  grid=source.grid;
  if (!source.ensdata.fcst_type.empty()) {
/*
    if (pds_supp != nullptr)
      delete[] pds_supp;
    if (source.ensdata.id == "ALL") {
      grib.m_lengths.pds_supp=ensdata.fcst_type.getLength()+1;
      pds_supp=new unsigned char[grib.m_lengths.pds_supp];
      memcpy(pds_supp,ensdata.fcst_type.toChar(),ensdata.fcst_type.getLength());
    }
    else {
      grib.m_lengths.pds_supp=ensdata.fcst_type.getLength()+2;
      pds_supp=new unsigned char[grib.m_lengths.pds_supp];
      memcpy(pds_supp,(ensdata.fcst_type+ensdata.id).toChar(),ensdata.fcst_type.getLength()+1);
    }
    pds_supp[grib.m_lengths.pds_supp-1]=(unsigned char)ensdata.total_size;
    grib.m_lengths.pds=40+grib.m_lengths.pds_supp;
*/
  }
  grib.table=3;
  grib.grid_catalog_id=255;
  grid.param=mapped_parameter_data(source.grid.src,source.discipline(),source.parameter_category(),source.grid.param,grib.table);
  fill_level_data(source,grid.level1_type,grid.level1,grid.level2);
  m_reference_date_time=source.reference_date_time();
  fill_time_range_data(source,grib.p1,grib.p2,grib.t_range,grid.nmean,grib.nmean_missing);
  m_reference_date_time=source.m_reference_date_time;
  switch (source.grid.grid_type) {
    case 0: {
// lat-lon
      grid.grid_type=0;
      dim=source.dim;
      def=source.def;
      grib.rescomp=0;
      if (source.grib.rescomp == 0x20) {
        grib.rescomp|=0x80;
      }
      if (source.earth_shape() == 2) {
        grib.rescomp|=0x40;
      }
      if ( (source.grib.rescomp&0x8) == 0x8) {
        grib.rescomp|=0x8;
      }
      break;
    }
    case 30: {
// lambert-conformal
      grid.grid_type=3;
      dim=source.dim;
      def=source.def;
      grib.rescomp=0;
      if (source.grib.rescomp == 0x20) {
        grib.rescomp|=0x80;
      }
      if (source.earth_shape() == 2) {
        grib.rescomp|=0x40;
      }
      if ( (source.grib.rescomp&0x8) == 0x8) {
        grib.rescomp|=0x8;
      }
      break;
    }
    default: {
      myerror="unable to map Grid Definition Template "+strutils::itos(source.grid.grid_type)+" into GRIB1";
      exit(1);
    }
  }
  if (source.bitmap.applies) {
    resize_bitmap(dim.size);
    for (n=0; n < static_cast<int>(dim.size); ++n) {
      bitmap.map[n]=source.bitmap.map[n];
    }
    bitmap.applies=true;
  }
  else {
    bitmap.applies=false;
  }
  stats=source.stats;
  galloc();
  if (dim.x < 0) {
    myerror="unable to convert reduced grids";
    exit(1);
  }
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
      m_gridpoints[n][m]=source.m_gridpoints[n][m];
    }
  }
  return *this;
}

bool GRIBGrid::is_accumulated_grid() const
{
  switch (grib.t_range) {
    case 114:
    case 116:
    case 124: {
      return true;
    }
    default: {
      return false;
    }
  }
}

void GRIBGrid::print(std::ostream& outs) const
{
  int n;
  float lon_print;
  int i,j,k,cnt,max_i;
  bool scientific=false;
  GLatEntry glat_entry;
  my::map<Grid::GLatEntry> gaus_lats;

  outs.setf(std::ios::fixed);
  outs.precision(2);
  if (grid.filled) {
    if (!floatutils::myequalf(stats.avg_val,0.) && fabs(stats.avg_val) < 0.01) {
      scientific=true;
    }
    if (def.type == Grid::Type::gaussianLatitudeLongitude) {
      if (!gridutils::filled_gaussian_latitudes(_path_to_gauslat_lists,gaus_lats,def.num_circles,(grib.scan_mode&0x40) != 0x40)) {
        myerror="unable to get gaussian latitudes for "+strutils::itos(def.num_circles)+" circles from '"+_path_to_gauslat_lists+"'";
        exit(0);
      }
    }
    switch (def.type) {
      case Grid::Type::latitudeLongitude:
      case Grid::Type::gaussianLatitudeLongitude: {
// non-reduced grid
        if (dim.x != -1) {
          for (n=0; n < dim.x; n+=14) {
            outs << "\n    \\ LON ";
            lon_print=def.slongitude+n*def.loincrement;
            if (def.slongitude > def.elongitude) {
              lon_print-=360.;
            }
            outs.precision(4);
            cnt=0;
            while (cnt < 14 && lon_print <= def.elongitude) {
              outs << std::setw(10) << lon_print;
              lon_print+=def.loincrement;
              ++cnt;
            }
            outs.precision(2);
            outs << "\n LAT \\  +-";
            for (i=0; i < cnt; ++i)
              outs << "----------";
            outs << std::endl;
            switch (grib.scan_mode) {
              case 0x0: {
                for (j=0; j < dim.y; ++j) {
                  if (def.type == Grid::Type::latitudeLongitude) {
                    outs.precision(3);
                    outs << std::setw(7) << (def.slatitude-j*def.laincrement) << " | ";
                    outs.precision(2);
                  }
                  else if (def.type == Grid::Type::gaussianLatitudeLongitude) {
                    outs.precision(3);
                    outs << std::setw(7) << latitude_at(j,&gaus_lats) << " | ";
                    outs.precision(2);
                  }
                  else
                    outs << std::setw(7) << j+1 << " | ";
                  if (scientific) {
                    outs.unsetf(std::ios::fixed);
                    outs.setf(std::ios::scientific);
                    outs.precision(3);
                  }
                  else {
                    if (grib.D > 0) {
                      outs.precision(grib.D);
                    } else {
                      if (grib.E >= 0) {
                        outs.precision(0);
                      } else {
                        outs.precision(-grib.E);
                      }
                    }
                  }
                  for (i=n; i < n+cnt; ++i) {
                    if (m_gridpoints[j][i] >= Grid::MISSING_VALUE)
                      outs << "   MISSING";
                    else
                      outs << std::setw(10) << m_gridpoints[j][i];
                  }
                  if (scientific) {
                    outs.unsetf(std::ios::scientific);
                    outs.setf(std::ios::fixed);
                  }
                  outs.precision(2);
                  outs << std::endl;
                }
                break;
              }
              case 0x40: {
                for (j=dim.y-1; j >= 0; j--) {
                  if (def.type == Grid::Type::latitudeLongitude) {
                    outs.precision(3);
                    outs << std::setw(7) << (def.elatitude-(dim.y-j-1)*def.laincrement) << " | ";
                    outs.precision(2);
                  }
                  else if (def.type == Grid::Type::gaussianLatitudeLongitude) {
                    outs.precision(3);
                    outs << std::setw(7) << latitude_at(j,&gaus_lats) << " | ";
                    outs.precision(2);
                  }
                  else
                    outs << std::setw(7) << dim.y-j << " | ";
                  if (scientific) {
                    outs.unsetf(std::ios::fixed);
                    outs.setf(std::ios::scientific);
                    outs.precision(3);
                  }
                  else
                    outs.precision(grib.D);
                  for (i=n; i < n+cnt; ++i) {
                    if (m_gridpoints[j][i] >= Grid::MISSING_VALUE)
                      outs << "   MISSING";
                    else
                      outs << std::setw(10) << m_gridpoints[j][i];
                  }
                  if (scientific) {
                    outs.unsetf(std::ios::scientific);
                    outs.setf(std::ios::fixed);
                  }
                  outs.precision(2);
                  outs << std::endl;
                }
                break;
              }
            }
          }
        }
// reduced grid
        else {
          glat_entry.key=def.num_circles;
          gaus_lats.found(glat_entry.key,glat_entry);
          for (j=0; j < dim.y; ++j) {
            std::cout << "\nLAT\\LON";
            for (i=1; i <= static_cast<int>(m_gridpoints[j][0]); ++i) {
              std::cout << std::setw(10) << (def.elongitude-def.slongitude)/(m_gridpoints[j][0]-1.)*static_cast<float>(i-1);
            }
            std::cout << std::endl;
            std::cout << std::setw(7) << glat_entry.lats[j];
            for (i=1; i <= static_cast<int>(m_gridpoints[j][0]); ++i) {
              std::cout << std::setw(10) << m_gridpoints[j][i];
            }
            std::cout << std::endl;
          }
        }
        break;
      }
      case Grid::Type::polarStereographic:
      case Grid::Type::lambertConformal: {
        for (i=0; i < dim.x; i+=14) {
          max_i=i+14;
          if (max_i > dim.x) max_i=dim.x;
          outs << "\n   \\ X";
          for (k=i; k < max_i; ++k) {
            outs << std::setw(10) << k+1;
          }
          outs << "\n Y  +-";
          for (k=i; k < max_i; ++k) {
            outs << "----------";
          }
          outs << std::endl;
          switch (grib.scan_mode) {
            case 0x0: {
              for (j=0; j < dim.y; ++j) {
                outs << std::setw(3) << dim.y-j << " | ";
                for (k=i; k < max_i; ++k) {
                  if (floatutils::myequalf(m_gridpoints[j][k],Grid::MISSING_VALUE)) {
                    outs << "   MISSING";
                  }
                  else {
                    if (scientific) {
                      outs.unsetf(std::ios::fixed);
                      outs.setf(std::ios::scientific);
                      outs.precision(3);
                      outs << std::setw(10) << m_gridpoints[j][k];
                      outs.unsetf(std::ios::scientific);
                      outs.setf(std::ios::fixed);
                      outs.precision(2);
                    }
                    else {
                      outs << std::setw(10) << m_gridpoints[j][k];
                    }
                  }
                }
                outs << std::endl;
              }
              break;
            }
            case 0x40: {
              for (j=dim.y-1; j >= 0; --j) {
                outs << std::setw(3) << j+1 << " | ";
                for (k=i; k < max_i; ++k) {
                  if (floatutils::myequalf(m_gridpoints[j][k],Grid::MISSING_VALUE)) {
                    outs << "   MISSING";
                  }
                  else {
                    if (scientific) {
                      outs.unsetf(std::ios::fixed);
                      outs.setf(std::ios::scientific);
                      outs.precision(3);
                      outs << std::setw(10) << m_gridpoints[j][k];
                      outs.unsetf(std::ios::scientific);
                      outs.setf(std::ios::fixed);
                      outs.precision(2);
                    }
                    else {
                      outs << std::setw(10) << m_gridpoints[j][k];
                    }
                  }
                }
                outs << std::endl;
              }
              break;
            }
          }
        }
        break;
      }
      default: { }
    }
    outs << std::endl;
  }
}

void GRIBGrid::print(std::ostream& outs,float bottom_latitude,float top_latitude,float west_longitude,float east_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
}

void GRIBGrid::print_ascii(std::ostream& outs) const
{
}

std::string GRIBGrid::build_level_search_key() const
{
  return strutils::itos(grid.src)+"-"+strutils::itos(grib.sub_center);
}

std::string GRIBGrid::first_level_description(xmlutils::LevelMapper& level_mapper) const
{
  short edition;
  if (grib.table < 0) {
    edition=0;
  }
  else {
    edition=1;
  }
  return level_mapper.description("WMO_GRIB"+strutils::itos(edition),strutils::itos(grid.level1_type),build_level_search_key());
}

std::string GRIBGrid::first_level_units(xmlutils::LevelMapper& level_mapper) const
{
  short edition;
  if (grib.table < 0) {
    edition=0;
  }
  else {
    edition=1;
  }
  return level_mapper.units("WMO_GRIB"+strutils::itos(edition),strutils::itos(grid.level1_type),build_level_search_key());
}

std::string GRIBGrid::build_parameter_search_key() const
{
  return strutils::itos(grid.src)+"-"+strutils::itos(grib.sub_center)+"."+strutils::itos(grib.table);
}

std::string GRIBGrid::parameter_cf_keyword(xmlutils::ParameterMapper& parameter_mapper) const
{
  short edition;
  if (grib.table < 0) {
    edition=0;
  }
  else {
    edition=1;
  }
  return parameter_mapper.cf_keyword("WMO_GRIB"+strutils::itos(edition),build_parameter_search_key()+":"+strutils::itos(grid.param));
}

std::string GRIBGrid::parameter_description(xmlutils::ParameterMapper& parameter_mapper) const
{
  short edition;
  if (grib.table < 0) {
    edition=0;
  }
  else {
    edition=1;
  }
  return parameter_mapper.description("WMO_GRIB"+strutils::itos(edition),build_parameter_search_key()+":"+strutils::itos(grid.param));
}

std::string GRIBGrid::parameter_short_name(xmlutils::ParameterMapper& parameter_mapper) const
{
  short edition;
  if (grib.table < 0) {
    edition=0;
  }
  else {
    edition=1;
  }
  return parameter_mapper.short_name("WMO_GRIB"+strutils::itos(edition),build_parameter_search_key()+":"+strutils::itos(grid.param));
}

std::string GRIBGrid::parameter_units(xmlutils::ParameterMapper& parameter_mapper) const
{
  short edition;
  if (grib.table < 0) {
    edition=0;
  }
  else {
    edition=1;
  }
  return parameter_mapper.units("WMO_GRIB"+strutils::itos(edition),build_parameter_search_key()+":"+strutils::itos(grid.param));
}

void GRIBGrid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
  static xmlutils::ParameterMapper parameter_mapper(path_to_parameter_map);
  bool scientific=false;

  outs.setf(std::ios::fixed);
  outs.precision(2);
  if (!floatutils::myequalf(stats.min_val,0.) && fabs(stats.min_val) < 0.01) {
    scientific=true;
  }
  if (verbose) {
    outs << "  Center: " << std::setw(3) << grid.src;
    if (grib.table >= 0) {
      outs << "-" << std::setw(2) << grib.sub_center << "  Table: " << std::setw(3) << grib.table;
    }
    outs << "  Process: " << std::setw(3) << grib.process << "  GridID: " << std::setw(3) << grib.grid_catalog_id << "  DataBits: " << std::setw(3) << grib.pack_width << std::endl;
    outs << "  RefTime: " << m_reference_date_time.to_string() << "  Fcst: " << std::setw(4) << std::setfill('0') << grid.fcst_time/100 << std::setfill(' ') << "  NumAvg: " << std::setw(3) << grid.nmean-grib.nmean_missing << "  TimeRng: " << std::setw(3) << grib.t_range << "  P1: " << std::setw(3) << grib.p1;
    if (grib.t_range != 10) {
      outs << " P2: " << std::setw(3) << grib.p2;
    }
    outs << " Units: ";
    if (grib.time_unit < 13) {
      outs << TIME_UNITS[grib.time_unit];
    }
    else {
      outs << grib.time_unit;
    }
    if (!ensdata.id.empty()) {
      outs << " Ensemble: " << ensdata.id;
      if (ensdata.id == "ALL" && ensdata.total_size > 0) {
        outs << "/" << ensdata.total_size;
      }
      outs << "/" << ensdata.fcst_type;
    }
    outs << "  Param: " << std::setw(3) << grid.param << "(" << parameter_short_name(parameter_mapper) << ")";
    if (grid.level1_type < 100 || grid.level1_type == 102 || grid.level1_type == 200 || grid.level1_type == 201) {
      outs << "  Level: " << LEVEL_TYPE_SHORT_NAME[grid.level1_type];
    }
    else {
      switch (grid.level1_type) {
        case 100:
        case 103:
        case 105:
        case 109:
        case 111:
        case 113:
        case 115:
        case 117:
        case 119:
        case 125:
        case 160: {
          outs << "  Level: " << std::setw(5) << grid.level1 << LEVEL_TYPE_UNITS[grid.level1_type] << " " << LEVEL_TYPE_SHORT_NAME[grid.level1_type];
          break;
        }
        case 107: {
          outs.precision(4);
          outs << "  Level: " << std::setw(6) << grid.level1 << " " << LEVEL_TYPE_SHORT_NAME[grid.level1_type];
          outs.precision(2);
          break;
        }
        case 101:
        case 104:
        case 106:
        case 110:
        case 112:
        case 114:
        case 116:
        case 120:
        case 121:
        case 141: {
          outs << "  Layer- Top: " << std::setw(5) << grid.level1 << LEVEL_TYPE_UNITS[grid.level1_type] << " Bottom: " << std::setw(5) << grid.level2 << LEVEL_TYPE_UNITS[grid.level1_type] << " " << LEVEL_TYPE_SHORT_NAME[grid.level1_type];
          break;
        }
        case 108: {
          outs << "  Layer- Top: " << std::setw(5) << grid.level1 << " Bottom: " << std::setw(5) << grid.level2 << " " << LEVEL_TYPE_SHORT_NAME[grid.level1_type];
          break;
        }
        case 128: {
          outs.precision(3);
          outs << "  Layer- Top: " << std::setw(5) << 1.1-grid.level1 << " Bottom: " << std::setw(5) << 1.1-grid.level2 << " " << LEVEL_TYPE_SHORT_NAME[grid.level1_type];
          outs.precision(2);
          break;
        }
        default: {
          outs << "  Level/Layer: " << std::setw(3) << grid.level1_type << " " << std::setw(5) << grid.level1 << " " << std::setw(5) << grid.level2;
        }
      }
    }
    switch (grid.grid_type) {
      case 0: {
// Latitude/Longitude
        outs.precision(3);
        outs << "\n  Grid: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  Type: " << std::setw(3) << grid.grid_type << "  LonRange: " << std::setw(7) << def.slongitude << " to " << std::setw(3) << def.elongitude << " by " << std::setw(5) << def.loincrement << "  LatRange: " << std::setw(3) << def.slatitude << " to " << std::setw(7) << def.elatitude << " by " << std::setw(5) << def.laincrement << "  ScanMode: 0x" << std::hex << grib.scan_mode << std::dec;
        outs.precision(2);
        break;
      }
      case 4:
      case 10: {
// Gaussian Lat/Lon
// Rotated Lat/Lon
        outs.precision(3);
        outs << "\n  Grid: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  Type: " << std::setw(3) << grid.grid_type << "  LonRange: " << std::setw(3) << def.slongitude << " to " << std::setw(7) << def.elongitude << " by " << std::setw(5) << def.loincrement << "  LatRange: " << std::setw(7) << def.slatitude << " to " << std::setw(7) << def.elatitude << "  Circles: " << std::setw(3) << def.num_circles << "  ScanMode: 0x" << std::hex << grib.scan_mode << std::dec;
        outs.precision(2);
        break;
      }
      case 3:
      case 5: {
// Lambert Conformal
// Polar Stereographic
        outs.precision(3);
        outs << "\n  Grid: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  Type: " << std::setw(3) << grid.grid_type << "  Lat: " << std::setw(7) << def.slatitude << " Lon: " << std::setw(7) << def.slongitude << " OrientLon: " << std::setw(7) << def.olongitude << " Proj: 0x" << std::hex << def.projection_flag << std::dec << "  Dx: " << std::setw(7) << def.dx << "km Dy: " << std::setw(7) << def.dy << "km  ScanMode: 0x" << std::hex << grib.scan_mode << std::dec;
        if (grid.grid_type == 3) {
          outs << std::endl;
          outs << "    Standard Parallels: " << std::setw(7) << def.stdparallel1 << " " << std::setw(7) << def.stdparallel2 << "  (Lat,Lon) of Southern Pole: (" << std::setw(7) << grib.sp_lat << "," << std::setw(7) << grib.sp_lon << ")";
        }
        outs.precision(2);
        break;
      }
      case 50: {
        outs << "\n  Type: " << std::setw(3) << grid.grid_type << "  Truncation Parameters: " << std::setw(3) << def.trunc1 << std::setw(5) << def.trunc2 << std::setw(5) << def.trunc3;
        break;
      }
    }
    outs << std::endl;
    outs << "  Decimal Scale (D): " << std::setw(3) << grib.D << "  Scale (E): " << std::setw(3) << grib.E << "  RefVal: ";
    if (scientific) {
      outs.unsetf(std::ios::fixed);
      outs.setf(std::ios::scientific);
      outs.precision(3);
      outs << stats.min_val;
      if (grid.filled) {
        outs << " (" << stats.min_i << "," << stats.min_j << ")  MaxVal: " << stats.max_val << " (" << stats.max_i << "," << stats.max_j << ")  AvgVal: " << stats.avg_val;
      }
      outs.unsetf(std::ios::scientific);
      outs.setf(std::ios::fixed);
      outs.precision(2);
    }
    else {
      outs << std::setw(9) << stats.min_val;
      if (grid.filled) {
        outs << " (" << stats.min_i << "," << stats.min_j << ")  MaxVal: " << std::setw(9) << stats.max_val << " (" << stats.max_i << "," << stats.max_j << ")  AvgVal: " << std::setw(9) << stats.avg_val;
      }
    }
    outs << std::endl;
  }
  else {
    if (grib.table < 0) {
      outs << " Ctr=" << grid.src;
    }
    else {
      outs << " Tbl=" << grib.table << " Ctr=" << grid.src << "-" << grib.sub_center;
    }
    outs << " ID=" << grib.grid_catalog_id << " RTime=" << m_reference_date_time.to_string("%Y%m%d%H%MM") << " Fcst=" << std::setw(4) << std::setfill('0') << grid.fcst_time/100 << std::setfill(' ') << " NAvg=" << grid.nmean-grib.nmean_missing << " TRng=" << grib.t_range << " P1=" << grib.p1;
    if (grib.t_range != 10) {
      outs << " P2=" << grib.p2;
    }
    outs << " Units=";
    if (grib.time_unit < 13) {
      outs << TIME_UNITS[grib.time_unit];
    }
    else {
      outs << grib.time_unit;
    }
    if (!ensdata.id.empty()) {
      outs << " Ens=" << ensdata.id;
      if (ensdata.id == "ALL" && ensdata.total_size > 0) {
        outs << "/" << ensdata.total_size;
      }
      outs << "/" << ensdata.fcst_type;
    }
    outs << " Param=" << grid.param;
    if (grid.level1_type < 100 || grid.level1_type == 102 || grid.level1_type == 200 || grid.level1_type == 201) {
      outs << " Level=" << LEVEL_TYPE_SHORT_NAME[grid.level1_type];
    }
    else {
      switch (grid.level1_type) {
        case 100:
        case 103:
        case 105:
        case 109:
        case 111:
        case 113:
        case 115:
        case 117:
        case 119:
        case 125:
        case 160: {
          outs << " Level=" << grid.level1 << LEVEL_TYPE_UNITS[grid.level1_type];
          break;
        }
        case 107: {
          outs.precision(4);
          outs << " Level=" << grid.level1;
          outs.precision(2);
          break;
        }
        case 101:
        case 104:
        case 106:
        case 110:
        case 112:
        case 114:
        case 116:
        case 120:
        case 121:
        case 141: {
          outs << " Layer=" << grid.level1 << LEVEL_TYPE_UNITS[grid.level1_type] << "," << grid.level2 << LEVEL_TYPE_UNITS[grid.level1_type];
          break;
        }
        case 108: {
          outs << " Layer=" << grid.level1 << "," << grid.level2;
          break;
        }
        case 128: {
          outs.precision(3);
          outs << " Layer=" << 1.1-grid.level1 << "," << 1.1-grid.level2;
          outs.precision(2);
          break;
        }
        default: {
          outs << " Levels=" << grid.level1_type << "," << grid.level1 << "," << grid.level2;
        }
      }
    }
    if (scientific) {
      outs.unsetf(std::ios::fixed);
      outs.setf(std::ios::scientific);
      outs.precision(3);
      outs << " R=" << stats.min_val;
      if (grid.filled)
        outs << " Max=" << stats.max_val << " Avg=" << stats.avg_val;
      outs << std::endl;
      outs.unsetf(std::ios::scientific);
      outs.setf(std::ios::fixed);
      outs.precision(2);
    }
    else {
      outs << " R=" << stats.min_val;
      if (grid.filled)
        outs << " Max=" << stats.max_val << " Avg=" << stats.avg_val;
      outs << std::endl;
    }
  }
}

void GRIBGrid::reverse_scan() {
  switch (grib.scan_mode) {
    case 0x0:
    case 0x40: {
      auto temp = def.slatitude;
      def.slatitude = def.elatitude;
      def.elatitude = temp;
      grib.scan_mode = 0x40 - grib.scan_mode;
      for (int n = 0; n < dim.y / 2; ++n) {
        for (int m = 0; m < dim.x; ++m) {
          auto temp = m_gridpoints[n][m];
          auto l = dim.y - n - 1;
          m_gridpoints[n][m] = m_gridpoints[l][m];
          m_gridpoints[l][m] = temp;
        }
      }
      if (bitmap.map != nullptr) {
        auto bcnt = 0;
        for (int n = 0; n < dim.y; ++n) {
          for (int m = 0; m < dim.x; ++m) {
            bitmap.map[bcnt] = floatutils::myequalf(m_gridpoints[n][m], Grid::MISSING_VALUE) ? 0 : 1;
            ++bcnt;
          }
        }
      }
      break;
    }
  }
}

GRIBGrid fabs(const GRIBGrid& source)
{
  GRIBGrid fabs_grid;
  fabs_grid=source;
  if (fabs_grid.grib.capacity.points == 0) {
    return fabs_grid;
  }
  fabs_grid.grid.pole=::fabs(fabs_grid.grid.pole);
  fabs_grid.stats.max_val=-Grid::MISSING_VALUE;
  fabs_grid.stats.min_val=Grid::MISSING_VALUE;
  size_t avg_cnt=0;
  fabs_grid.stats.avg_val=0.;
  for (int n=0; n < fabs_grid.dim.y; ++n) {
    for (int m=0; m < fabs_grid.dim.x; ++m) {
      if (!floatutils::myequalf(fabs_grid.m_gridpoints[n][m],Grid::MISSING_VALUE)) {
        fabs_grid.m_gridpoints[n][m]=::fabs(fabs_grid.m_gridpoints[n][m]);
        if (fabs_grid.m_gridpoints[n][m] > fabs_grid.stats.max_val) {
          fabs_grid.stats.max_val=fabs_grid.m_gridpoints[n][m];
        }
        if (fabs_grid.m_gridpoints[n][m] < fabs_grid.stats.min_val) {
          fabs_grid.stats.min_val=fabs_grid.m_gridpoints[n][m];
        }
        fabs_grid.stats.avg_val+=fabs_grid.m_gridpoints[n][m];
        ++avg_cnt;
      }
    }
  }
  if (avg_cnt > 0) {
    fabs_grid.stats.avg_val/=static_cast<double>(avg_cnt);
  }
  return fabs_grid;
}

GRIBGrid interpolate_gaussian_to_lat_lon(const GRIBGrid& source,std::string path_to_gauslat_lists)
{
  GRIBGrid latlon_grid;
  GRIBGrid::GRIBData grib_data;
  int n,m;
  int ni,si;
  float lat,dyt,dya,dyb;
  my::map<Grid::GLatEntry> gaus_lats;
  Grid::GLatEntry glat_entry;

  if (source.def.type != Grid::Type::gaussianLatitudeLongitude)
    return source;
  if (!gridutils::filled_gaussian_latitudes(path_to_gauslat_lists,gaus_lats,source.def.num_circles,(source.grib.scan_mode&0x40) != 0x40)) {
    myerror="unable to get gaussian latitudes for "+strutils::itos(source.def.num_circles)+" circles from '"+path_to_gauslat_lists+"'";
    exit(0);
  }
  grib_data.reference_date_time=source.m_reference_date_time;
  grib_data.valid_date_time=source.m_valid_date_time;
  grib_data.dim.x=source.dim.x;
  grib_data.dim.y=73;
  grib_data.def.slongitude=source.def.slongitude;
  grib_data.def.elongitude=source.def.elongitude;
  grib_data.def.loincrement=source.def.loincrement;
  if (!gaus_lats.found(source.def.num_circles,glat_entry)) {
    myerror="error getting gaussian latitudes for "+strutils::itos(source.def.num_circles)+" circles";
    exit(1);
  }
  if (source.grib.scan_mode == 0x0) {
    grib_data.def.slatitude=90.;
    while (grib_data.def.slatitude > glat_entry.lats[0]) {
      grib_data.def.slatitude-=2.5;
      grib_data.dim.y-=2;
    }
  }
  else {
    grib_data.def.slatitude=-90.;
    while (grib_data.def.slatitude < glat_entry.lats[0]) {
      grib_data.def.slatitude+=2.5;
      grib_data.dim.y-=2;
    }
  }
  grib_data.def.elatitude=-grib_data.def.slatitude;
  grib_data.def.laincrement=2.5;
  grib_data.dim.size=grib_data.dim.x*grib_data.dim.y;
  grib_data.def.projection_flag=source.def.projection_flag;
  grib_data.def.type=Grid::Type::latitudeLongitude;
  grib_data.param_version=source.grib.table;
  grib_data.source=source.grid.src;
  grib_data.process=source.grib.process;
  grib_data.grid_catalog_id=source.grib.grid_catalog_id;
  grib_data.time_unit=source.grib.time_unit;
  grib_data.time_range=source.grib.t_range;
  grib_data.sub_center=source.grib.sub_center;
  grib_data.D=source.grib.D;
  grib_data.grid_type=0;
  grib_data.scan_mode=source.grib.scan_mode;
  grib_data.param_code=source.grid.param;
  grib_data.level_types[0]=source.grid.level1_type;
  grib_data.level_types[1]=-1;
  grib_data.levels[0]=source.grid.level1;
  grib_data.levels[1]=source.grid.level2;
  grib_data.p1=source.grib.p1;
  grib_data.p2=source.grib.p2;
  grib_data.num_averaged=source.grid.nmean;
  grib_data.num_averaged_missing=source.grib.nmean_missing;
  grib_data.pack_width=source.grib.pack_width;
  grib_data.gds_included=true;
  grib_data.gridpoints=new double *[grib_data.dim.y];
  for (n=0; n < grib_data.dim.y; ++n) {
    grib_data.gridpoints[n]=new double[grib_data.dim.x];
  }
  for (n=0; n < grib_data.dim.y; ++n) {
    lat= (grib_data.scan_mode == 0x0) ? grib_data.def.slatitude-n*grib_data.def.laincrement : grib_data.def.slatitude+n*grib_data.def.laincrement;
    ni=source.latitude_index_north_of(lat,&gaus_lats);
    si=source.latitude_index_south_of(lat,&gaus_lats);
    for (m=0; m < grib_data.dim.x; ++m) {
      if (ni >= 0 && si >= 0) {
        dyt=source.latitude_at(ni,&gaus_lats)-source.latitude_at(si,&gaus_lats);
        dya=dyt-(source.latitude_at(ni,&gaus_lats)-lat);
        dyb=dyt-(lat-source.latitude_at(si,&gaus_lats));
        grib_data.gridpoints[n][m]=(source.gridpoint(m,ni)*dya+source.gridpoint(m,si)*dyb)/dyt;
      }
      else {
grib_data.gridpoints[n][m]=Grid::MISSING_VALUE;
      }
    }
  }
  latlon_grid.fill_from_grib_data(grib_data);
  return latlon_grid;
}


GRIBGrid pow(const GRIBGrid& source,double exponent)
{
  GRIBGrid pow_grid;
  int n,m;
  size_t avg_cnt=0;

  pow_grid=source;
  if (pow_grid.grib.capacity.points == 0)
    return pow_grid;

  pow_grid.grid.pole=::pow(pow_grid.grid.pole,exponent);

  pow_grid.stats.max_val=::pow(pow_grid.stats.max_val,exponent);
  pow_grid.stats.min_val=::pow(pow_grid.stats.min_val,exponent);
  pow_grid.stats.avg_val=0.;
  for (n=0; n < pow_grid.dim.y; ++n) {
    for (m=0; m < pow_grid.dim.x; ++m) {
      if (!floatutils::myequalf(pow_grid.m_gridpoints[n][m],Grid::MISSING_VALUE)) {
        pow_grid.m_gridpoints[n][m]=::pow(pow_grid.m_gridpoints[n][m],exponent);
        pow_grid.stats.avg_val+=pow_grid.m_gridpoints[n][m];
        ++avg_cnt;
      }
    }
  }
  if (avg_cnt > 0) {
    pow_grid.stats.avg_val/=static_cast<double>(avg_cnt);
  }
  return pow_grid;
}

GRIBGrid sqrt(const GRIBGrid& source)
{
  GRIBGrid sqrt_grid;
  int n,m;
  size_t avg_cnt=0;

  sqrt_grid=source;
  if (sqrt_grid.grib.capacity.points == 0) {
    return sqrt_grid;
  }
  sqrt_grid.grid.pole=::sqrt(sqrt_grid.grid.pole);
  sqrt_grid.stats.max_val=::sqrt(sqrt_grid.stats.max_val);
  sqrt_grid.stats.min_val=::sqrt(sqrt_grid.stats.min_val);
  sqrt_grid.stats.avg_val=0.;
  for (n=0; n < sqrt_grid.dim.y; ++n) {
    for (m=0; m < sqrt_grid.dim.x; ++m) {
      if (!floatutils::myequalf(sqrt_grid.m_gridpoints[n][m],Grid::MISSING_VALUE)) {
        sqrt_grid.m_gridpoints[n][m]=::sqrt(sqrt_grid.m_gridpoints[n][m]);
        sqrt_grid.stats.avg_val+=sqrt_grid.m_gridpoints[n][m];
        ++avg_cnt;
      }
    }
  }
  if (avg_cnt > 0) {
    sqrt_grid.stats.avg_val/=static_cast<double>(avg_cnt);
  }
  return sqrt_grid;
}

GRIBGrid combine_hemispheres(const GRIBGrid& nhgrid,const GRIBGrid& shgrid)
{
  GRIBGrid combined;
  const GRIBGrid *addon;
  int n,m,dim_y;
  size_t l;

  if (nhgrid.reference_date_time() != shgrid.reference_date_time()) {
    return combined;
  }
  if (nhgrid.dim.x != shgrid.dim.x || nhgrid.dim.y != shgrid.dim.y) {
    return combined;
  }
/*
  if (nhgrid.bitmap.map != nullptr || shgrid.bitmap.map != nullptr) {
myerror="no handling of missing data yet";
exit(1);
  }
*/
  combined.grib.capacity.points=nhgrid.dim.size*2-nhgrid.dim.x;
  dim_y=nhgrid.dim.y*2-1;
  combined.dim.x=nhgrid.dim.x;
  combined.m_gridpoints=new double *[dim_y];
  for (n=0; n < dim_y; ++n) {
    combined.m_gridpoints[n]=new double[combined.dim.x];
  }
  switch (nhgrid.grib.scan_mode) {
    case 0x0: {
      combined=nhgrid;
      combined.def.elatitude=shgrid.def.elatitude;
      addon=&shgrid;
      if (shgrid.stats.max_val > combined.stats.max_val) {
        combined.stats.max_val=shgrid.stats.max_val;
      }
      if (shgrid.stats.min_val < combined.stats.min_val) {
        combined.stats.min_val=shgrid.stats.min_val;
      }
      break;
    }
    case 0x40: {
      combined=shgrid;
      combined.def.elatitude=nhgrid.def.elatitude;
      addon=&nhgrid;
      if (nhgrid.stats.max_val > combined.stats.max_val) {
        combined.stats.max_val=nhgrid.stats.max_val;
      }
      if (nhgrid.stats.min_val < combined.stats.min_val) {
        combined.stats.min_val=nhgrid.stats.min_val;
      }
      break;
    }
    default: {
      return combined;
    }
  }
  combined.grib.capacity.y=combined.dim.y=dim_y;
  combined.dim.size = combined.dim.x * combined.dim.y;
  l=0;
  for (n=combined.dim.y/2; n < combined.dim.y; ++n) {
    for (m=0; m < combined.dim.x; ++m) {
      combined.m_gridpoints[n][m]=addon->m_gridpoints[l][m];
    }
    ++l;
  }
  combined.grid.num_missing = 0;
  if (combined.bitmap.map != nullptr) {
    delete[] combined.bitmap.map;
  }
  combined.bitmap.capacity = combined.dim.size;
  combined.bitmap.map=new unsigned char[combined.bitmap.capacity];
  for (int n = 0, l = 0; n < combined.dim.y; ++n) {
    for (int m = 0; m < combined.dim.x; ++m) {
      if (combined.m_gridpoints[n][m] == Grid::MISSING_VALUE) {
        ++combined.grid.num_missing;
        combined.bitmap.map[l] = 0;
      } else {
        combined.bitmap.map[l] = 1;
      }
      ++l;
    }
  }
  combined.grib.D= (nhgrid.grib.D > shgrid.grib.D) ? nhgrid.grib.D : shgrid.grib.D;
  combined.set_scale_and_packing_width(combined.grib.D,combined.grib.pack_width);
  return combined;
}

GRIBGrid create_subset_grid(const GRIBGrid& source, float bottom_latitude, float
    top_latitude, float left_longitude, float right_longitude) {
  auto r = right_longitude;
  if (r < 0.) {
    r += 360.;
  }
  auto l = left_longitude;
  if (l < 0.) {
    l += 360.;
  }
  if (r < l) {
    myerror = "bad subset specified";
    exit(1);
  }
  GRIBGrid subset_grid;
  subset_grid.m_reference_date_time = source.m_reference_date_time;
  subset_grid.m_valid_date_time = source.m_valid_date_time;
  subset_grid.grib.scan_mode = source.grib.scan_mode;
  subset_grid.def.laincrement = source.def.laincrement;
  subset_grid.def.type = source.def.type;
  Grid::GLatEntry glat_entry;
  my::map<Grid::GLatEntry> gaus_lats;
  if (subset_grid.def.type == Grid::Type::gaussianLatitudeLongitude) {
    if (source.path_to_gaussian_latitude_data().empty()) {
      throw runtime_error("path to gaussian latitude data was not specified");
    }
    else if (!gridutils::filled_gaussian_latitudes(
        source.path_to_gaussian_latitude_data(), gaus_lats,
        subset_grid.def.num_circles, (subset_grid.grib.scan_mode & 0x40) !=
        0x40)) {
      throw runtime_error("unable to get gaussian latitudes for " +
          strutils::itos(subset_grid.def.num_circles) + " circles from '" +
          source.path_to_gaussian_latitude_data() + "'");
    }
    glat_entry.key = subset_grid.def.num_circles;
    if (!gaus_lats.found(glat_entry.key, glat_entry)) {
      throw runtime_error("unable to subset gaussian grid with " +
          strutils::itos(subset_grid.def.num_circles) + " latitude circles");
    }
  }
  switch (subset_grid.grib.scan_mode) {

    // top down
    case 0x0: {
      if (!floatutils::myequalf(source.gridpoint_at(bottom_latitude,
          source.def.slongitude), Grid::BAD_VALUE)) {
        subset_grid.def.elatitude = bottom_latitude;
      } else {
        if (subset_grid.def.type == Grid::Type::gaussianLatitudeLongitude) {
          subset_grid.def.elatitude = glat_entry.lats[
              source.latitude_index_south_of(bottom_latitude, &gaus_lats)];
        } else {
          subset_grid.def.elatitude = source.def.slatitude -
              source.latitude_index_south_of(bottom_latitude, &gaus_lats) *
              subset_grid.def.laincrement;
        }
      }
      if (!floatutils::myequalf(source.gridpoint_at(top_latitude,
          source.def.slongitude), Grid::BAD_VALUE)) {
        subset_grid.def.slatitude = top_latitude;
      } else {
        if (subset_grid.def.type == Grid::Type::gaussianLatitudeLongitude) {
          subset_grid.def.slatitude = glat_entry.lats[
              source.latitude_index_north_of(top_latitude, &gaus_lats)];
        } else {
          subset_grid.def.slatitude = source.def.slatitude -
              source.latitude_index_north_of(top_latitude, &gaus_lats) *
              subset_grid.def.laincrement;
        }
      }
      if (subset_grid.def.type == Grid::Type::gaussianLatitudeLongitude) {
        subset_grid.dim.y = 0;
        for (size_t n = 0; n < subset_grid.def.num_circles * 2; ++n) {
          if ((glat_entry.lats[n] > subset_grid.def.elatitude &&
              glat_entry.lats[n] < subset_grid.def.slatitude) ||
              floatutils::myequalf(glat_entry.lats[n],
              subset_grid.def.elatitude) || floatutils::myequalf(
              glat_entry.lats[n], subset_grid.def.slatitude)) {
            ++subset_grid.dim.y;
          }
        }
      } else {
        subset_grid.dim.y=lroundf((subset_grid.def.slatitude -
            subset_grid.def.elatitude) / subset_grid.def.laincrement) + 1;
      }
      break;
    }

    // bottom up
    case 0x40: {
      if (!floatutils::myequalf(source.gridpoint_at(bottom_latitude,
          source.def.slongitude), Grid::BAD_VALUE)) {
        subset_grid.def.slatitude = bottom_latitude;
      } else {
        if (subset_grid.def.type == Grid::Type::gaussianLatitudeLongitude) {
          subset_grid.def.slatitude = glat_entry.lats[
              source.latitude_index_south_of(bottom_latitude, &gaus_lats)];
        } else {
          if (bottom_latitude < source.def.slatitude) {
            subset_grid.def.slatitude = source.def.slatitude;
          } else {
            subset_grid.def.slatitude = source.def.slatitude +
                source.latitude_index_south_of(bottom_latitude, &gaus_lats) *
                subset_grid.def.laincrement;
          }
        }
      }
      if (!floatutils::myequalf(source.gridpoint_at(top_latitude,
          source.def.slongitude), Grid::BAD_VALUE)) {
        subset_grid.def.elatitude = top_latitude;
      } else {
        if (subset_grid.def.type == Grid::Type::gaussianLatitudeLongitude) {
          subset_grid.def.elatitude = glat_entry.lats[
              source.latitude_index_north_of(top_latitude, &gaus_lats)];
        } else {
          if (top_latitude > source.def.elatitude) {
            subset_grid.def.elatitude = source.def.elatitude;
          } else {
            subset_grid.def.elatitude = source.def.slatitude +
                source.latitude_index_north_of(top_latitude, &gaus_lats) *
                subset_grid.def.laincrement;
          }
        }
      }
      if (subset_grid.def.type == Grid::Type::gaussianLatitudeLongitude) {
        subset_grid.dim.y = 0;
        for (size_t n = 0; n < subset_grid.def.num_circles * 2; ++n) {
          if (glat_entry.lats[n] >= subset_grid.def.slatitude &&
              glat_entry.lats[n] <= subset_grid.def.elatitude) {
            ++subset_grid.dim.y;
          }
        }
      } else {
        subset_grid.dim.y=lroundf((subset_grid.def.elatitude -
            subset_grid.def.slatitude) / subset_grid.def.laincrement) + 1;
      }
      break;
    }
  }
  auto lat_index = source.latitude_index_of(subset_grid.def.slatitude,
      &gaus_lats);
  if (lat_index >= source.dim.y) {
    subset_grid.grid.filled = false;
    return subset_grid;
  }
  subset_grid.def.loincrement = source.def.loincrement;
  if (!floatutils::myequalf(source.gridpoint_at(source.def.slatitude,
      left_longitude),Grid::BAD_VALUE)) {
    subset_grid.def.slongitude = left_longitude;
  } else {
    subset_grid.def.slongitude = subset_grid.def.loincrement *
        source.longitude_index_west_of(left_longitude);
  }
  if (!floatutils::myequalf(source.gridpoint_at(source.def.slatitude,
      right_longitude), Grid::BAD_VALUE)) {
    subset_grid.def.elongitude = right_longitude;
  } else {
    subset_grid.def.elongitude = source.longitude_index_east_of(
        right_longitude) - subset_grid.def.loincrement;
  }
  if (subset_grid.def.elongitude < subset_grid.def.slongitude) {
    subset_grid.dim.x = lroundf((subset_grid.def.elongitude + 360. -
        subset_grid.def.slongitude) / subset_grid.def.loincrement) + 1;
  } else {
    subset_grid.dim.x = lroundf((subset_grid.def.elongitude -
        subset_grid.def.slongitude) / subset_grid.def.loincrement) + 1;
  }
  subset_grid.dim.size = subset_grid.dim.x * subset_grid.dim.y;
  subset_grid.grid = source.grid;
  subset_grid.grib = source.grib;
  if (subset_grid.bitmap.capacity < subset_grid.dim.size) {
    if (subset_grid.bitmap.map != nullptr) {
      delete[] subset_grid.bitmap.map;
    }
    subset_grid.bitmap.capacity = subset_grid.dim.size;
    subset_grid.bitmap.map = new unsigned char[subset_grid.bitmap.capacity];
  }
  subset_grid.grid.num_missing = 0;
  subset_grid.galloc();
  subset_grid.stats.max_val = -Grid::MISSING_VALUE;
  subset_grid.stats.min_val = Grid::MISSING_VALUE;
  subset_grid.stats.avg_val = 0.;
  size_t cnt = 0, avg_cnt = 0;
  for (int n = 0; n < subset_grid.dim.y; ++n) {
    for (int m = 0; m < subset_grid.dim.x; ++m) {
      auto lon_index = source.longitude_index_of(subset_grid.def.slongitude +
          m * subset_grid.def.loincrement);
      subset_grid.m_gridpoints[n][m] = source.gridpoint(lon_index, lat_index);
      if (floatutils::myequalf(subset_grid.m_gridpoints[n][m],
          Grid::MISSING_VALUE)) {
        subset_grid.bitmap.map[cnt] = 0;
        ++subset_grid.grid.num_missing;
      } else {
        subset_grid.bitmap.map[cnt] = 1;
        if (subset_grid.m_gridpoints[n][m] > subset_grid.stats.max_val) {
          subset_grid.stats.max_val = subset_grid.m_gridpoints[n][m];
          subset_grid.stats.max_i = m;
          subset_grid.stats.max_j = n;
        }
        if (subset_grid.m_gridpoints[n][m] < subset_grid.stats.min_val) {
          subset_grid.stats.min_val = subset_grid.m_gridpoints[n][m];
          subset_grid.stats.min_i = m;
          subset_grid.stats.min_j = n;
        }
        subset_grid.stats.avg_val += subset_grid.m_gridpoints[n][m];
        ++avg_cnt;
      }
      ++cnt;
    }
    ++lat_index;
  }
  if (avg_cnt > 0) {
    subset_grid.stats.avg_val /= static_cast<double>(avg_cnt);
  }
  if (subset_grid.grid.num_missing == 0) {
    subset_grid.bitmap.applies = false;
  }
  else {
    subset_grid.bitmap.applies = true;
  }
  subset_grid.set_scale_and_packing_width(source.grib.D,
      subset_grid.grib.pack_width);
  subset_grid.grid.filled = true;
  return subset_grid;
}

#endif
