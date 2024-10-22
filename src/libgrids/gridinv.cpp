#include <iomanip>
#include <gridinv.hpp>
#include <utils.hpp>
#include <bits.hpp>

const bool GridInventory::date_only=true,
           GridInventory::full_inventory=false;

const char *GridInventory::format_descr[]={"","GRIB","ON84","NCAR 5-degree SLP","NCAR Octagonal","NCAR 5-degree Lat/Lon","NCAR Navy","USSR SLP"};
const short GridInventory::grid_types[]={-1,-1,-1,0,5,0,-1,0};
const size_t GridInventory::fixed_grid_lengths[]={0,0,0,2048,3000,2600,0,1158};

size_t GridInventory::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
  short hr,min;
  int duma=0,dumb=0;
  short off;

  if (datetime[0].year() == 0) {
    return 0;
  }
  bits::set(output_buffer,inventory_type_,12,4);
  bits::set(output_buffer,datetime[0].year(),16,12);
  bits::set(output_buffer,datetime[0].month(),28,4);
  bits::set(output_buffer,datetime[0].day(),32,5);
  hr=datetime[0].time()/100;
  if ( (hr % 100) != 0) {
    min=(hr % 100);
    hr=hr/100;
    hr=hr*100+lround(min/60.);
  }
  bits::set(output_buffer,hr,37,12);
  off=49;
  if (inventory_type_ == 2) {
    bits::set(output_buffer,datetime[0].year(),off,12);
    bits::set(output_buffer,datetime[0].month(),off+12,4);
    bits::set(output_buffer,datetime[0].day(),off+16,5);
    hr=datetime[0].time()/100;
    if ( (hr % 100) != 0) {
	min=(hr % 100);
	hr=hr/100;
	hr=hr*100+lround(min/60.);
    }
    bits::set(output_buffer,hr,off+21,12);
    off+=33;
  }
  bits::set(output_buffer,static_cast<int>(dsnum*10.),off,14);
  bits::set(output_buffer,format_,off+14,7);
  bits::set(output_buffer,time_range_,off+21,8);
  switch (time_range_) {
    case 0:
    case 1: {
	bits::set(output_buffer,period[0],off+29,8);
	bits::set(output_buffer,0,off+37,8);
	break;
    }
    case 2:
    case 3:
    case 4:
    case 5:
    case 51:
    case 113:
    case 114:
    case 115:
    case 116:
    case 117:
    case 118:
    case 119:
    case 123:
    case 124: {
	bits::set(output_buffer,period,off+29,8,0,2);
	break;
    }
    case 10: {
	bits::set(output_buffer,period[0],off+29,16);
	break;
    }
  }
  bits::set(output_buffer,time_unit_,off+45,8);
  if (inventory_type_ == 1)
    bits::set(output_buffer,number_averaged_,off+53,12);
  else
    bits::set(output_buffer,0,off+53,12);
  bits::set(output_buffer,parameter_,off+65,8);
  bits::set(output_buffer,level_type_,off+73,8);
  switch (level_type_) {
    case 20:
    case 100:
    case 103:
    case 105:
    case 107:
    case 109:
    case 111:
    case 113:
    case 115:
    case 117:
    case 119:
    case 125:
    case 160: {
	bits::set(output_buffer,level_id[0],off+81,16);
	break;
    }
    case 254: {
	if (level_id[1] != 1000) {
	  std::cerr << "Error: level type 254 specified but bottom level not 1000 hPa" << std::endl;
	  exit(1);
	}
	bits::set(output_buffer,level_id[0],off+81,16);
	break;
    }
    case 101: {
	bits::set(output_buffer,static_cast<int>(level_id[0]/10.),off+81,8);
	bits::set(output_buffer,static_cast<int>(level_id[1]/10.),off+89,8);
	break;
    }
    case 121: {
	bits::set(output_buffer,(1100-level_id[0]),off+81,8);
	bits::set(output_buffer,(1100-level_id[1]),off+89,8);
	break;
    }
    case 104:
    case 106:
    case 108:
    case 110:
    case 112:
    case 114:
    case 116:
    case 120:
    case 128:
    case 141: {
	bits::set(output_buffer,level_id,off+81,8,0,2);
	break;
    }
    default: {
	bits::set(output_buffer,0,off+81,16);
    }
  }
  if (gds_included)
    bits::set(output_buffer,1,off+97,1);
  else
    bits::set(output_buffer,0,off+97,1);
  off+=98;
  if (gds_included) {
    bits::set(output_buffer,grid_type_,off,8);
    bits::set(output_buffer,grid_length_,off+8,20);
    bits::set(output_buffer,nx,off+28,12);
    bits::set(output_buffer,ny,off+40,12);
    switch (grid_type_) {
	case 0:
	case 4: {
	  duma=dist_x*10000;
	  dumb=dist_y*10000;
	  break;
	}
	case 5: {
	  duma=dist_x;
	  dumb=dist_y;
	  break;
	}
    }
    bits::set(output_buffer,duma,off+52,17);
    bits::set(output_buffer,dumb,off+69,17);
    duma=flat*10000+900001;
    dumb=flon*10000+1800001;
    bits::set(output_buffer,duma,off+86,21);
    bits::set(output_buffer,dumb,off+107,22);
    bits::set(output_buffer,scan_dir,off+129,2);
    off+=131;
  }
  if (grid_type_ == 5) {
    bits::set(output_buffer,pole_proj,off,2);
    bits::set(output_buffer,static_cast<int>((olon*10000.)+1800001),off+2,22);
    off+=24;
  }
  bits::set(output_buffer,filename_.length(),off,8);
  bits::set(output_buffer,filename_.c_str(),off+8,8,0,filename_.length());
  off+=(8+filename_.length()*8);
  return (off+7)/8;
}

void GridInventory::fill(const unsigned char *buffer,bool fill_date_only)
{
  short yr,mo,dy,hr,min;
  int dum,dum2;
  short off,flen;
  char filename[256];

  bits::get(buffer,inventory_type_,12,4);
  bits::get(buffer,yr,16,12);
  bits::get(buffer,mo,28,4);
  bits::get(buffer,dy,32,5);
  bits::get(buffer,hr,37,12);
  if ( (hr % 100) != 0) {
    min=lround((hr % 100)/100.*60.);
    datetime[0].set(yr,mo,dy,(hr/100+min)*100);
  }
  else
    datetime[0].set(yr,mo,dy,hr*100);
  off=49;
  if (inventory_type_ == 2) {
    bits::get(buffer,yr,off,12);
    bits::get(buffer,mo,off+12,4);
    bits::get(buffer,dy,off+16,5);
    bits::get(buffer,hr,off+21,12);
    if ( (hr % 100) != 0) {
	min=lround((hr % 100)/100.*60.);
	datetime[1].set(yr,mo,dy,(hr/100+min)*100);
    }
    else
	datetime[1].set(yr,mo,dy,hr*10000);
    off+=33;
  }
  else
    datetime[1]=datetime[0];
  if (fill_date_only)
    return;

  bits::get(buffer,dum,off,14);
  dsnum=dum/10.;
  bits::get(buffer,format_,off+14,7);
  bits::get(buffer,time_range_,off+21,8);
  switch (time_range_) {
    case 0:
    case 1: {
	bits::get(buffer,period[0],off+29,8);
	break;
    }
    case 2:
    case 3:
    case 4:
    case 5:
    case 51:
    case 113:
    case 114:
    case 115:
    case 116:
    case 117:
    case 118:
    case 119:
    case 123:
    case 124: {
	bits::get(buffer,period,off+29,8,0,2);
	break;
    }
    case 10: {
	bits::get(buffer,period[0],off+29,16);
	break;
    }
  }
  bits::get(buffer,time_unit_,off+45,8);
  bits::get(buffer,number_averaged_,off+53,12);
  bits::get(buffer,parameter_,off+65,8);
  bits::get(buffer,level_type_,off+73,8);
  level_id[1]=0;
  switch (level_type_) {
    case 20:
    case 100:
    case 103:
    case 105:
    case 107:
    case 109:
    case 111:
    case 113:
    case 115:
    case 117:
    case 119:
    case 125:
    case 160: {
	bits::get(buffer,level_id[0],off+81,16);
	break;
    }
    case 254: {
	bits::get(buffer,level_id[0],off+81,16);
	level_id[1]=1000;
	break;
    }
    case 101: {
	bits::get(buffer,level_id,off+81,8,0,2);
	level_id[0]*=10;
	level_id[1]*=10;
	break;
    }
    case 104:
    case 106:
    case 108:
    case 110:
    case 112:
    case 114:
    case 116:
    case 120:
    case 121:
    case 128:
    case 141: {
	bits::get(buffer,level_id,off+81,8,0,2);
	break;
    }
    default: {
	level_id[0]=0;
    }
  }
  bits::get(buffer,dum,off+97,1);
  gds_included= (dum == 1) ? true : false;
  off+=98;
  if (gds_included) {
    bits::get(buffer,grid_type_,off,8);
    bits::get(buffer,grid_length_,off+8,20);
    bits::get(buffer,nx,off+28,12);
    bits::get(buffer,ny,off+40,12);
    bits::get(buffer,dum,off+52,17);
    bits::get(buffer,dum2,off+69,17);
    switch (grid_type_) {
// distances are in ten-thousandths of degrees
	case 0:
	case 4: {
	  dist_x=dum/10000.;
	  dist_y=dum2/10000.;
	  break;
	}
// distances are tens of meters
	case 5: {
	  dist_x=dum/100.;
	  dist_y=dum2/100.;
	  break;
	}
	default: {
	  std::cerr << "Error: unrecognized grid type " << grid_type_ << std::endl;
	  exit(1);
	}
    }
    bits::get(buffer,dum,off+86,21);
    flat=(dum-900001)/10000.;
    bits::get(buffer,dum,off+107,22);
    flon=(dum-1800001)/10000.;
    bits::get(buffer,scan_dir,off+129,2);
    off+=131;
    if (grid_type_ == 5) {
	bits::get(buffer,pole_proj,off,2);
	bits::get(buffer,dum,off+2,22);
	olon=(dum-1800001)/10000.;
	off+=24;
    }
  }
  else {
    grid_length_=fixed_grid_lengths[format_];
  }
  bits::get(buffer,flen,off,8);
  bits::get(buffer,filename,off+8,8,0,flen);
  filename_.assign(filename,flen);
}

void fill_time_info_from_NCAR_grids(const Grid *source,DateTime& datetime,short& trange,short& p2,short& tunit)
{
  int months[]={0,31,28,31,30,31,30,31,31,30,31,30,31};
  size_t ntimes;

  if (datetime.day() > 0) {
    trange=10;
    tunit=1;
  }
  else {
//    if (date.time() == 3100) {
//	date.set(date.year(),date.month(),1,0);
//    }
//    else {
	datetime.set_day(1);
//    }
    if ( (datetime.year() % 4) == 0) {
	months[2]=29;
    }
    trange=113;
    if (source->number_averaged() > months[datetime.month()]) {
	if (source->number_averaged() == 999) {
	  p2=0;
	  tunit=255;
	}
	else {
	  ntimes=(source->number_averaged()+months[datetime.month()]-1)/months[datetime.month()];
	  p2=24/ntimes;
	  tunit=1;
	}
    }
    else {
	p2=1;
	tunit=2;
    }
  }
}

void fill_level_info_from_NCAR_grids(const Grid *source,short& param,short& ltype,size_t *level_id)
{
  ltype=100;
  level_id[0]=source->first_level_value();
  level_id[1]=source->second_level_value();
  if (level_id[0] == 1001) {
    ltype=1;
    level_id[0]=0;
  }
  else if (level_id[0] == 1013) {
    ltype=102;
    level_id[0]=0;
  }
  if (level_id[1] != 0) {
    ltype=254;
    level_id[1]=1000;
  }
}

void GridInventory::fill_from_grid(const Grid *source,Grid::Format format,float dataset_number,std::string filename)
{
  GRIBGrid *g=reinterpret_cast<GRIBGrid *>(const_cast<Grid *>(source));

  inventory_type_=1;
  datetime[0]=source->reference_date_time();
  datetime[1]=datetime[0];
  dsnum=dataset_number;
  period[0]=source->forecast_time()/10000;
  number_averaged_=source->number_averaged();
  gds_included=false;
  switch (format) {
    case Grid::Format::grib: {
	format_=1;
	time_range_=g->time_range();
	period[1]=g->P2();
	time_unit_=g->time_unit();
	parameter_=g->parameter();
	level_type_=g->first_level_type();
	switch (level_type_) {
	  case 100:
	  case 103:
	  case 105:
	  case 107:
	  case 109:
	  case 111:
	  case 113:
	  case 115:
	  case 125:
	  case 160:
	  case 200:
	  case 201: {
	    level_id[0]=source->first_level_value();
	    level_id[1]=0;
	    break;
	  }
	  default: {
	    level_id[0]=source->first_level_value();
	    level_id[1]=source->second_level_value();
	  }
	}
	grid_type_=g->type();
	gds_included=true;
	switch (g->scan_mode()) {
	  case 0x0: {
	    scan_dir=1;
	    break;
	  }
	  case 0x40: {
	    scan_dir=0;
	    break;
	  }
	  default: {
	    std::cerr << "Error: scan mode " << g->scan_mode() << " not recognized" << std::endl;
	    exit(1);
	  }
	}
	break;
    }
    case Grid::Format::slp: {
	format_=3;
	fill_time_info_from_NCAR_grids(source,datetime[0],time_range_,period[1],time_unit_);
	parameter_=2;
	level_type_=102;
	break;
    }
    case Grid::Format::octagonal: {
	format_=4;
	fill_time_info_from_NCAR_grids(source,datetime[0],time_range_,period[1],time_unit_);
	parameter_=source->parameter();
	fill_level_info_from_NCAR_grids(source,parameter_,level_type_,level_id);
	break;
    }
    case Grid::Format::latlon: {
	format_=5;
	fill_time_info_from_NCAR_grids(source,datetime[0],time_range_,period[1],time_unit_);
	parameter_=source->parameter();
	fill_level_info_from_NCAR_grids(source,parameter_,level_type_,level_id);
	break;
    }
    case Grid::Format::navy: {
	format_=6;
	fill_time_info_from_NCAR_grids(source,datetime[0],time_range_,period[1],time_unit_);
	parameter_=source->parameter();
	switch (source->parameter()) {
	  case 6: {
	    level_type_=102;
	    break;
	  }
	  case 55:
	  case 56:
	  case 57:
	  case 58:
	  case 59: {
	    level_type_=1;
	    break;
	  }
	  default: {
	    level_type_=100;
	  }
	}
	level_id[0]=source->first_level_value();
	gds_included=true;
	switch (source->type()) {
	  case 10:
	  case 11: {
	    grid_type_=0;
	    break;
	  }
	  default: {
	    grid_type_=5;
	    pole_proj=2;
	  }
	}
	scan_dir=0;
	break;
    }
    case Grid::Format::ussrslp: {
	format_=7;
	if (floatutils::myequalf(source->statistics().avg_val,Grid::MISSING_VALUE)) {
	  datetime[0].set_year(0);
	  return;
	}
	time_range_=10;
	period[1]=0;
	time_unit_=1;
	parameter_=2;
	level_type_=102;
	break;
    }
    default: {
	std::cerr << "Unrecognized format number " << static_cast<int>(format) << std::endl;
	exit(1);
    }
  }

  if (gds_included) {
    grid_length_=fixed_grid_lengths[format_];
    if (grid_length_ == 0) {
	switch (format_) {
	  case 1: {
	    grid_length_=(source->dimensions().size*g->packing_width()+7)/8;
	    break;
	  }
	  case 6: {
	    if (source->type() == 3 || source->type() == 4) {
		grid_length_=7972;
	    }
	    else {
		std::cerr << "Error: no grid length available for navy grid type " << source->type() << std::endl;
		exit(1);
	    }
	    break;
	  }
	  default: {
	    std::cerr << "Error: no grid length available for format " << format_ << std::endl;
	    exit(1);
	  }
	}
    }
    nx=source->dimensions().x;
    ny=source->dimensions().y;
    flat=source->definition().slatitude;
    flon=source->definition().slongitude;
    switch (grid_type_) {
	case 0: {
	  dist_x=source->definition().loincrement;
	  dist_y=source->definition().laincrement;
	  break;
	}
	case 4: {
	  dist_x=source->definition().loincrement;
	  switch (source->definition().num_circles) {
	    case 24: {
		dist_y=3.7105;
		break;
	    }
	    case 32: {
		dist_y=2.7903;
		break;
	    }
	    case 47: {
		dist_y=1.9046;
		break;
	    }
	    default: {
		std::cerr << "Error: unknown resolution for " << source->definition().num_circles << " circles" << std::endl;
		exit(1);
	    }
	  }
	  break;
	}
	case 5: {
	  dist_x=source->definition().dx*100.;
	  dist_y=source->definition().dy*100.;
	  olon=source->definition().olongitude;
	  break;
	}
	default: {
	  std::cerr << "Error: grid type " << grid_type_ << " not recognized" << std::endl;
	  exit(1);
	}
    }
  }
  filename_=filename;
}

bool GridInventory::is_averaged_grid() const
{
  switch (time_range_) {
    case 3:
    case 113:
    case 115:
    case 117:
    case 123: {
	return true;
    }
  }

  return false;
}

std::ostream& operator<<(std::ostream& out_stream,const GridInventory& source)
{
  out_stream.setf(std::ios::fixed);
  out_stream.precision(1);

  if (source.verbose) {
    out_stream << "Type: " << source.inventory_type_ << "  Format: " << source.format_ << " (" << source.format_descr[source.format_] << ")  Dataset: " << std::setfill('0') << std::setw(5) << source.dsnum << "  File: " << source.filename_ << std::setfill(' ');
    switch (source.inventory_type_) {
	case 1: {
	  out_stream << "  Date: " << source.datetime[0].to_string("%Y-%m-%d %H%MM");
	  if (source.number_averaged_ > 0)
	    out_stream << "  Number Averaged: " << source.number_averaged_;
	  break;
	}
	case 2: {
	  out_stream << "  Date Range: " << source.datetime[0].to_string("%Y-%m-%d %H%MM") << " to " << source.datetime[1].to_string("%Y-%m-%d %H%MM");
	  break;
	}
    }
    out_stream << "  Time Range: " << source.time_range_;
    switch (source.time_range_) {
	case 0:
	case 1:
	case 10: {
	  out_stream << "  Period: " << source.period[0];
	  break;
	}
	case 2:
	case 3:
	case 4:
	case 5:
	case 51:
	case 113:
	case 114:
	case 115:
	case 116:
	case 117:
	case 118:
	case 119:
	case 123:
	case 124: {
	  out_stream << "  Periods: " << source.period[0] << "/" << source.period[1];
	  break;
	}
    }
    out_stream << " " << GRIBGrid::TIME_UNITS[source.time_unit_] << "  Parameter Code: " << source.parameter_;
    out_stream << "  Level: " << source.level_type_;
    if (source.level_type_ < 202)
	out_stream << " (" << GRIBGrid::LEVEL_TYPE_SHORT_NAME[source.level_type_] << ")";
    switch (source.level_type_) {
	case 20:
	case 100:
	case 103:
	case 105:
	case 107:
	case 109:
	case 111:
	case 113:
	case 115:
	case 117:
	case 119:
	case 125:
	case 160: {
	  out_stream << "/" << source.level_id[0] << std::endl;
	  break;
	}
	case 101:
	case 104:
	case 106:
	case 108:
	case 110:
	case 112:
	case 114:
	case 116:
	case 120:
	case 121:
	case 128:
	case 141: {
	  out_stream << "/" << source.level_id[0] << "," << source.level_id[1] << std::endl;
	  break;
	}
	default: {
	  out_stream << std::endl;
	}
    }
    if (source.gds_included) {
	out_stream << "  Grid Type: " << source.grid_type_ << "  Grid Length: " << source.grid_length_ << "  Dimensions: " << source.nx << "x" << source.ny;
	out_stream.precision(4);
	switch (source.grid_type_) {
	  case 0:
	  case 4: {
	    out_stream << "  Resolution: " << source.dist_x << "x" << source.dist_y << "  First Gridpoint: (" << source.flat << "," << source.flon << ")  Scan Direction: " << source.scan_dir;
	    break;
	  }
	  case 5: {
	    out_stream << "  Grid Spacing: " << source.dist_x << "x" << source.dist_y << "  First Gridpoint: (" << source.flat << "," << source.flon << ")  Pole Projection: " << source.pole_proj << "  Orientation Longitude: " << source.olon << "  Scan Direction: " << source.scan_dir;
	    break;
	  }
	  default: {
	    std::cerr << "Error: unrecognized grid type " << source.grid_type_ << std::endl;
	    exit(1);
	  }
	}
    }
  }
  else {
    out_stream << "Type=" << source.inventory_type_ << " Fmt=" << source.format_ << " DS=" << std::setfill('0') << std::setw(5) << source.dsnum << " FN=" << source.filename_ <<  std::setfill(' ');
    switch (source.inventory_type_) {
	case 1: {
	  out_stream << " Date=" << source.datetime[0].to_string("%Y%m%d%H%MM");
	  if (source.number_averaged_ > 0) {
	    out_stream << " NAvg=" << source.number_averaged_;
	  }
	  break;
	}
	case 2: {
	  std::cout << " DateRange=" << source.datetime[0].to_string("%Y%m%d%H%MM") << "-" << source.datetime[1].to_string("%Y%m%d%H%MM");
	  break;
	}
    }
    out_stream << " TRange=" << source.time_range_;
    switch (source.time_range_) {
	case 0:
	case 1:
	case 10: {
	  out_stream << " Pd=" << source.period[0];
	  break;
	}
	case 2:
	case 3:
	case 4:
	case 5:
	case 51:
	case 113:
	case 114:
	case 115:
	case 116:
	case 117:
	case 118:
	case 119:
	case 123:
	case 124: {
	  out_stream << " Pds=" << source.period[0] << "/" << source.period[1];
	  break;
	}
    }
    out_stream << "(" << GRIBGrid::TIME_UNITS[source.time_unit_] << ") Param=" << source.parameter_;
    out_stream << " Level=" << source.level_type_;
    if (source.level_type_ < 202)
	out_stream << "(" << GRIBGrid::LEVEL_TYPE_SHORT_NAME[source.level_type_] << ")";
    switch (source.level_type_) {
	case 20:
	case 100:
	case 103:
	case 105:
	case 107:
	case 109:
	case 111:
	case 113:
	case 115:
	case 117:
	case 119:
	case 125:
	case 160: {
	  out_stream << "/" << source.level_id[0];
	  break;
	}
	case 101:
	case 104:
	case 106:
	case 108:
	case 110:
	case 112:
	case 114:
	case 116:
	case 120:
	case 121:
	case 128:
	case 141: {
	  out_stream << "/" << source.level_id[0] << "," << source.level_id[1];
	  break;
	}
	case 254: {
	  out_stream << "(ISBY)/" << source.level_id[0] << "," << source.level_id[1];
	  break;
	}
    }
    if (source.gds_included) {
	out_stream << " GType=" << source.grid_type_ << " GLen=" << source.grid_length_ << " Dim=" << source.nx << "x" << source.ny;
	out_stream.precision(4);
	switch (source.grid_type_) {
	  case 0:
	  case 4: {
	    out_stream << " Res=" << source.dist_x << "x" << source.dist_y << " First=(" << source.flat << "," << source.flon << ") Scan=" << source.scan_dir;
	    break;
	  }
	  case 5: {
	    out_stream << " GSpace=" << source.dist_x << "x" << source.dist_y << " First=(" << source.flat << "," << source.flon << ") PoleProj=" << source.pole_proj << " OLon=" << source.olon << " Scan=" << source.scan_dir;
	    break;
	  }
	  default: {
	    std::cerr << "Error: unrecognized grid type " << source.grid_type_ << std::endl;
	    exit(1);
	  }
	}
    }
  }

  return out_stream;
}

bool operator!=(const GridInventory& source1,const GridInventory& source2)
{
  if (source1.datetime[0] != source2.datetime[0]) {
    return true;
  }
  if (source1.datetime[1] != source2.datetime[1]) {
    return true;
  }
  if (source1.inventory_type_ != source2.inventory_type_) {
    return true;
  }
  if (source1.time_range_ != source2.time_range_) {
    return true;
  }
  if (source1.period[0] != source2.period[0] || source1.period[1] != source2.period[1]) {
    return true;
  }
  if (source1.time_unit_ != source2.time_unit_) {
    return true;
  }
  if (source1.number_averaged_ != source2.number_averaged_) {
    return true;
  }
  if (source1.parameter_ != source2.parameter_) {
    return true;
  }
  if (source1.level_type_ != source2.level_type_) {
    return true;
  }
  if (source1.grid_type_ != source2.grid_type_) {
    return true;
  }
  if (source1.level_id[0] != source2.level_id[0] || source1.level_id[1] != source2.level_id[1]) {
    return true;
  }
  if (source1.nx != source2.nx) {
    return true;
  }
  if (source1.ny != source2.ny) {
    return true;
  }
  if (source1.scan_dir != source2.scan_dir) {
    return true;
  }
  if (source1.format_ != source2.format_) {
    return true;
  }
  if (!floatutils::myequalf(source1.dist_x,source2.dist_x)) {
    return true;
  }
  if (!floatutils::myequalf(source1.dist_y,source2.dist_y)) {
    return true;
  }
  if (!floatutils::myequalf(source1.flat,source2.flat)) {
    return true;
  }
  if (!floatutils::myequalf(source1.flon,source2.flon)) {
    return true;
  }
  if (!floatutils::myequalf(source1.dsnum,source2.dsnum)) {
    return true;
  }
  if (source1.pole_proj != source2.pole_proj) {
    return true;
  }
  if (!floatutils::myequalf(source1.olon,source2.olon)) {
    return true;
  }
  if (source1.gds_included != source2.gds_included) {
    return true;
  }
  if (source1.verbose != source2.verbose) {
    return true;
  }
  if (source1.filename_ != source2.filename_) {
    return true;
  }
  return false;
}

bool operator==(const GridInventory& source1,const GridInventory& source2)
{
  return !(source1 != source2);
}

bool operator<(const GridInventory& source1,const GridInventory& source2)
{
  size_t ltype1= (source1.level_type_ == 102) ? 0 : source1.level_type_;
  size_t ltype2= (source2.level_type_ == 102) ? 0 : source2.level_type_;
  if (source1.datetime[0] < source2.datetime[0]) {
    return true;
  }
  else if (source1.datetime[0] > source2.datetime[0]) {
    return false;
  }
  if (source1.datetime[1] < source2.datetime[1]) {
    return true;
  }
  else if (source1.datetime[1] > source2.datetime[1]) {
    return false;
  }
  if (source1.dsnum < source2.dsnum) {
    return true;
  }
  else if (source1.dsnum > source2.dsnum) {
    return false;
  }
  if (source1.inventory_type_ < source2.inventory_type_) {
    return true;
  }
  else if (source1.inventory_type_ > source2.inventory_type_) {
    return false;
  }
  if (source1.format_ < source2.format_) {
    return true;
  }
  else if (source1.format_ > source2.format_) {
    return false;
  }
  if (ltype1 < ltype2) {
    return true;
  }
  else if (ltype1 > ltype2) {
    return false;
  }
  switch (source1.level_type_) {
    case 100: {
	if (source1.level_id[0] > source2.level_id[0]) {
	  return true;
	}
	else if (source1.level_id[0] < source2.level_id[0]) {
	  return false;
	}
	break;
    }
    default: {
	if (source1.level_id[0] < source2.level_id[0] || source1.level_id[1] < source2.level_id[1]) {
	  return true;
	}
	else if (source1.level_id[0] > source2.level_id[0] || source1.level_id[1] > source2.level_id[1]) {
	  return false;
	}
    }
  }
  if (source1.parameter_ < source2.parameter_) {
    return true;
  }
  else if (source1.parameter_ > source2.parameter_) {
    return false;
  }
  if (source1.time_range_ < source2.time_range_) {
    return true;
  }
  else if (source1.time_range_ > source2.time_range_) {
    return false;
  }
  if (source1.period[0] < source2.period[0] || source1.period[1] < source2.period[1]) {
    return true;
  }
  else if (source1.period[0] > source2.period[0] || source1.period[1] > source2.period[1]) {
    return false;
  }
  if (source1.time_unit_ < source2.time_unit_) {
    return true;
  }
  else if (source1.time_unit_ > source2.time_unit_) {
    return false;
  }
  if (source1.number_averaged_ < source2.number_averaged_) {
    return true;
  }
  else if (source1.number_averaged_ > source2.number_averaged_) {
    return false;
  }
  if (source1.gds_included < source2.gds_included) {
    return true;
  }
  else if (source1.gds_included > source2.gds_included) {
    return false;
  }
  if (source1.grid_type_ < source2.grid_type_) {
    return true;
  }
  else if (source1.grid_type_ > source2.grid_type_) {
    return false;
  }
  if (source1.nx < source2.nx) {
    return true;
  }
  else if (source1.nx > source2.nx) {
    return false;
  }
  if (source1.ny < source2.ny) {
    return true;
  }
  else if (source1.ny > source2.ny) {
    return false;
  }
  if (source1.scan_dir < source2.scan_dir) {
    return true;
  }
  else if (source1.scan_dir > source2.scan_dir) {
    return false;
  }
  if (source1.dist_x < source2.dist_x) {
    return true;
  }
  else if (source1.dist_x > source2.dist_x) {
    return false;
  }
  if (source1.dist_y < source2.dist_y) {
    return true;
  }
  else if (source1.dist_y > source2.dist_y) {
    return false;
  }
  if (source1.flat < source2.flat) {
    return true;
  }
  else if (source1.flat > source2.flat) {
    return false;
  }
  if (source1.flon < source2.flon) {
    return true;
  }
  else if (source1.flon > source2.flon) {
    return false;
  }
  return false;
}

bool operator<=(const GridInventory& source1,const GridInventory& source2)
{
  return (source1 < source2 || source1 == source2);
}

bool operator>(const GridInventory& source1,const GridInventory& source2)
{
return false;
}

bool operator>=(const GridInventory& source1,const GridInventory& source2)
{
  return (source1 > source2 || source1 == source2);
}
