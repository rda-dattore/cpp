// FILE: octagonalgrid.cpp

#include <iomanip>
#include <grid.hpp>
#include <utils.hpp>
#include <bits.hpp>

int InputOctagonalGridStream::peek()
{
  if (icosstream != nullptr) {
    return icosstream->peek();
  }
  else {
    unsigned char buffer[3000];
    int bytes_read=read(buffer,3000);
    if (bytes_read != bfstream::error && bytes_read != bfstream::eof) {
	fs.seekg(-bytes_read,std::ios_base::cur);
	--num_read;
    }
    return bytes_read;
  }
}

int InputOctagonalGridStream::read(unsigned char *buffer,size_t buffer_length)
{
  if (buffer_length < 3000) {
    std::cerr << "Error: buffer overflow" << std::endl;
    exit(1);
  }
  int bytes_read;
// read a grid from the stream
  if (icosstream != nullptr) {
    if ( (bytes_read=icosstream->read(buffer,buffer_length)) <= 0) {
	return bytes_read;
    }
  }
  else {
    fs.read(reinterpret_cast<char *>(buffer),3000);
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (!fs.good()) {
	return bfstream::error;
    }
    bytes_read=fs.gcount();
    if (bytes_read < 3000) {
	bytes_read=bfstream::error;
    }
  }
  ++num_read;
  return bytes_read;
}

OctagonalGrid::OctagonalGrid()
{
  size_t n;

  dim.y=51;
  dim.x=47;
  dim.size=2397;
  def.type=Grid::polarStereographicType;
  def.slatitude=-4.86;
  def.slongitude=-122.61;
  def.llatitude=60.;
  def.olongitude=-80.;
  def.dx=def.dy=381.;
  def.projection_flag=0;
  for (n=0; n < 14; n++) {
    start[n]=14-n;
    end[n]=n+32;
  }
  for (n=14; n < 37; n++) {
    start[n]=0;
    end[n]=46;
  }
  for (n=37; n < 51; n++) {
    start[n]=n-36;
    end[n]=82-n;
  }
}

OctagonalGrid::OctagonalGrid(const OctagonalGrid& source)
{
  *this=source;
}

OctagonalGrid& OctagonalGrid::operator=(const OctagonalGrid& source)
{
  int n,m;

  if (this == &source) {
    return *this;
  }
  reference_date_time_=source.reference_date_time_;
  valid_date_time_=source.valid_date_time_;
  dim=source.dim;
  def=source.def;
  stats=source.stats;
  grid=source.grid;
  if (source.grid.filled) {
    if (gridpoints_ == nullptr) {
	gridpoints_=new double *[dim.y];
	for (n=0; n < dim.y; ++n) {
	  gridpoints_[n]=new double[dim.x];
	}
    }
    for (n=0; n < dim.y; ++n) {
	for (m=0; m < dim.x; ++m) {
	  gridpoints_[n][m]=source.gridpoints_[n][m];
	}
    }
  }
  return *this;
}

size_t OctagonalGrid::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
  static const size_t bias=2048;
  size_t n,m,cnt=0;
  long long sum;
  int scale,*packed,dum;
  double dscale,dbase;
  short hr;

  if (!grid.filled)
    return 0;
  if (floatutils::myequalf(stats.max_val,-Grid::missing_value) || floatutils::myequalf(stats.min_val,Grid::missing_value)) {
    return 0;
  }
  for (n=0; n < 27; ++n) {
    output_buffer[n]=0;
  }
  bits::set(output_buffer,grid.grid_type,0,6);
  bits::set(output_buffer,reference_date_time_.year()-1900,6,7);
  bits::set(output_buffer,reference_date_time_.month(),13,4);
  bits::set(output_buffer,reference_date_time_.day(),17,5);
  hr=reference_date_time_.time()/10000;
  if ( (reference_date_time_.time() % 10000) >= 3000) {
    ++hr;
  }
  bits::set(output_buffer,hr,22,5);
  dum=1023.-grid.level1;
  bits::set(output_buffer,dum,27,10);
  bits::set(output_buffer,grid.param,37,9);
  bits::set(output_buffer,grid.fcst_time/10000,46,9);
  dum=1023.-grid.level2;
  bits::set(output_buffer,dum,55,10);
  bits::set(output_buffer,grid.src,65,6);
  bits::set(output_buffer,0,71,5);
  bits::set(output_buffer,bias,76,12);
  bits::set(output_buffer,grid.nmean,100,8);
  bits::set(output_buffer,0,108,12);
// do gridpoint scaling
  dscale=log((stats.max_val-stats.min_val)/static_cast<float>(0xfff))/log(2.);
  scale=dscale;
  if (dscale >= 0.) scale++;
  bits::set(output_buffer,scale+bias,88,12);
  dbase=(stats.max_val+stats.min_val)*0.5;
  auto base=floatutils::cdcconv(dbase,1);
  bits::set(output_buffer,base,120,60);
// pack the gridpoints_
  packed=new int[1977];
  for (n=0; n < 51; ++n) {
    for (m=0; m < 47; ++m) {
	if (m >= start[n] && m <= end[n]) {
	  if (!floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	    packed[cnt++]=lround((gridpoints_[n][m]-dbase)*pow(2.,-scale))+bias;
	  }
	  else {
	    packed[cnt++]=0;
	  }
	}
    }
  }
  if (cnt != 1977) {
    std::cerr << "Error: attempt to pack more than 1977 points" << std::endl;
    exit(1);
  }
  bits::set(output_buffer,packed,216,12,0,1977);
  delete[] packed;
// pack the checksum
  checksum(output_buffer,400,60,sum);
  bits::set(output_buffer,sum,23940,60);
  return 3000;
}

OctagonalGrid& OctagonalGrid::operator=(const LatLonGrid& source)
{
  int n;

  grid.grid_type=source.type();
  reference_date_time_=source.reference_date_time();
  valid_date_time_=source.valid_date_time();
  grid.level1=source.first_level_value();
  grid.param=source.parameter();
  switch (grid.param) {
// convert latlon SLP 6 to octagonal SLP 28
    case 6:
	grid.param=28;
	break;
// convert latlon SST 57 to octagonal SST 47
    case 57:
	grid.param=47;
	break;
  }
  grid.fcst_time=source.forecast_time();
  grid.level2=source.second_level_value();
  grid.src=source.source();
  grid.nmean=source.number_averaged();
  if (gridpoints_ == nullptr) {
    gridpoints_=new double *[dim.y];
    for (n=0; n < dim.y; ++n) {
	gridpoints_[n]=new double[dim.x];
    }
  }
  bessel_interpolation(reinterpret_cast<Grid *>(const_cast<LatLonGrid *>(&source)));
  grid.pole=gridpoints_[25][23];
  grid.filled=true;
  return *this;
}

OctagonalGrid& OctagonalGrid::operator=(const NavyGrid& source)
{
  int n,m,nn,mm,avg_cnt=0;

  if (source.type() != 3) {
    std::cerr << "Error converting Navy grid type " << source.type() << std::endl;
    exit(1);
  }
  grid.grid_type=1;
  reference_date_time_=source.reference_date_time();
  valid_date_time_=source.valid_date_time();
  grid.level1=source.first_level_value();
  grid.param=source.parameter();
  switch (grid.param) {
// convert navy SLP 6 to octagonal SLP 28
    case 6:
	grid.param=28;
	break;
// convert navy SST 57 to octagonal SST 47
    case 57:
	grid.param=47;
	break;
  }
  grid.fcst_time=source.forecast_time();
  grid.level2=source.second_level_value();
  grid.src=source.source();
  grid.nmean=source.number_averaged();

  if (gridpoints_ == nullptr) {
    gridpoints_=new double *[dim.y];
    for (n=0; n < dim.y; n++)
	gridpoints_[n]=new double[dim.x];
  }
  stats.max_val=-Grid::missing_value;
  stats.min_val=Grid::missing_value;
  stats.avg_val=0.;
  for (nn=6,n=0; nn < 57; nn++,n++) {
    for (mm=8,m=0; mm < 55; mm++,m++) {
	gridpoints_[n][m]=source.gridpoint(mm,nn);
	if (m >= start[n] && m <= end[n]) {
	  if (gridpoints_[n][m] > stats.max_val) {
	    stats.max_val=gridpoints_[n][m];
	    stats.max_i=m+1;
	    stats.max_j=n+1;
	  }
	  if (gridpoints_[n][m] < stats.min_val) {
	    stats.min_val=gridpoints_[n][m];
	    stats.min_i=m+1;
	    stats.min_j=n+1;
	  }
	  stats.avg_val+=gridpoints_[n][m];
	  avg_cnt++;
	}
    }
  }
  if (avg_cnt > 0)
    stats.avg_val/=static_cast<float>(avg_cnt);
  grid.pole=gridpoints_[25][23];
  grid.filled=true;
  return *this;
}

void OctagonalGrid::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  int n,m,l,avg_cnt=0;
  long long sum;
  int bias,scale,dum,*pval;

  if (stream_buffer == nullptr) {
    std::cerr << "Error: empty file stream" << std::endl;
    exit(1);
  }
// unpack the grid header
  bits::get(stream_buffer,grid.grid_type,0,6);
  short yr,mo,dy,time;
  bits::get(stream_buffer,yr,6,7);
  bits::get(stream_buffer,mo,13,4);
  bits::get(stream_buffer,dy,17,5);
  bits::get(stream_buffer,time,22,5);
  reference_date_time_.set(1900+yr,mo,dy,time*10000);
  bits::get(stream_buffer,dum,27,10);
  grid.level1=1023.-dum;
  bits::get(stream_buffer,grid.param,37,9);
  bits::get(stream_buffer,grid.fcst_time,46,9);
  grid.fcst_time*=10000;
  valid_date_time_=reference_date_time_.time_added(grid.fcst_time);
  bits::get(stream_buffer,dum,55,10);
  grid.level2=1023.-dum;
  bits::get(stream_buffer,grid.src,65,6);
  if (dy == 0) {
    bits::get(stream_buffer,grid.nmean,100,8);
  }
  else {
    grid.nmean=0;
  }
  grid.filled=false;
// check the checksum for the grid
  if (checksum(stream_buffer,400,60,sum) != 0) {
    std::cerr << "Warning: checksum error - Date: " << reference_date_time_.to_string() << "  Level: " << grid.level1 << "  Parameter: " << grid.param << std::endl;
  }
  bits::get(stream_buffer,bias,76,12);
  bits::get(stream_buffer,scale,88,12);
  scale-=bias;
  auto base=floatutils::cdcconv(stream_buffer,120,1);
// unpack the octagonal grid gridpoints
  if (!fill_header_only) {
// if memory has not yet been allocated for the gridpoints, do it now
    if (gridpoints_ == nullptr) {
	gridpoints_=new double *[dim.y];
	for (n=0; n < dim.y; n++)
	  gridpoints_[n]=new double[dim.x];
    }
    pval=new int[dim.size];
    bits::get(stream_buffer,pval,216,12,0,dim.size);
    stats.max_val=-Grid::missing_value;
    stats.min_val=Grid::missing_value;
    stats.avg_val=0.;
    grid.num_missing=0;
    for (n=0,l=0; n < dim.y; ++n) {
	for (m=0; m < dim.x; ++m) {
	  if (m >= start[n] && m <= end[n]) {
	    if (pval[l] > 0) {
		gridpoints_[n][m]=base+(pval[l]-bias)*pow(2.,scale);
		if (gridpoints_[n][m] > stats.max_val) {
		  stats.max_val=gridpoints_[n][m];
		  stats.max_i=m+1;
		  stats.max_j=n+1;
		}
		if (gridpoints_[n][m] < stats.min_val) {
		  stats.min_val=gridpoints_[n][m];
		  stats.min_i=m+1;
		  stats.min_j=n+1;
		}
		stats.avg_val+=gridpoints_[n][m];
		avg_cnt++;
	    }
	    else {
		gridpoints_[n][m]=Grid::missing_value;
		grid.num_missing++;
	    }
	    ++l;
	  }
	  else
	    gridpoints_[n][m]=Grid::missing_value;
	}
    }
    grid.pole=gridpoints_[25][23];
    if (avg_cnt > 0) {
	stats.avg_val/=static_cast<float>(avg_cnt);
    }
    grid.filled=true;
    delete[] pval;
  }
  else {
    bits::get(stream_buffer,dum,12072,12);
    if (dum == 0) {
	grid.pole=Grid::missing_value;
    }
    else {
	grid.pole=base+(dum-bias)*pow(2.,scale);
    }
  }
}

bool OctagonalGrid::is_averaged_grid() const
{
  return (grid.nmean > 0);
}

void OctagonalGrid::print(std::ostream& outs) const
{
  int n,m,l,max_m;

  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (grid.filled) {
    for (m=0; m < dim.x; m+=16) {
	max_m=m+16;
	if (max_m > dim.x) max_m=dim.x;
	outs << "\n   \\ X";
	for (l=m; l < max_m; ++l) {
	  outs << std::setw(8) << l+1;
	}
	outs << "\n  Y +-";
	for (l=m; l < max_m; ++l) {
	  outs << "--------";
	}
	outs << std::endl;
	for (n=dim.y-1; n >= 0; n--) {
	  outs << std::setw(3) << n+1 << " | ";
	  for (l=m; l < max_m; ++l) {
	    if (floatutils::myequalf(gridpoints_[n][l],Grid::missing_value)) {
		outs << "        ";
	    }
	    else {
		outs << std::setw(8) << gridpoints_[n][l];
	    }
	  }
	  outs << std::endl;
	}
	outs << std::endl;
    }
    outs << std::endl;
  }
}

void OctagonalGrid::print(std::ostream& outs,float bottom_latitude,float top_latitude,float west_longitude,float east_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
}

void OctagonalGrid::print_ascii(std::ostream& outs) const
{
  if (!grid.filled) {
    return;
  }
  outs.setf(std::ios::fixed);
  auto precision= (floatutils::precision(stats.min_val) > floatutils::precision(stats.avg_val)) ? floatutils::precision(stats.min_val) : floatutils::precision(stats.avg_val);
  precision= (precision > floatutils::precision(stats.max_val)) ? precision : floatutils::precision(stats.max_val);
  outs.precision(precision);
  auto width=7+precision;
  outs << reference_date_time_.to_string() << " " << std::setw(2) << grid.param << " " << std::setw(6) << grid.level1 << " " << std::setw(6) << grid.level2 << " " << std::setw(3) << dim.x << " " << std::setw(3) << dim.y << " " << std::setw(3) << grid.nmean << " " << std::setw(2) << width << " " << std::setw(2) << precision << std::endl;
  for (short n=0; n < dim.y; ++n) {
    for (short m=0; m < dim.x; ++m) {
	outs << std::setw(width);
	if (floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	  outs << -999.9;
	}
	else {
	  outs << gridpoints_[n][m];
	}
    }
    outs << std::endl;
  }
}

void OctagonalGrid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (verbose) {
    if (reference_date_time_.day() > 0) {
	outs << " DAILY GRID -- Date: " << reference_date_time_.to_string();
	if (grid.fcst_time == 0) {
	  outs << "  Analysis Grid" << std::endl;
	}
	else {
	  outs << "  Valid Time: " << valid_date_time_.to_string() << std::endl;
	}
      outs << "   Format: NCAR Octagonal  Levels: " << std::setw(6) << grid.level1 << "mb " << std::setw(6) << grid.level2 << "mb  Parameter Code: " << std::setw(2) << grid.param << "  Source: " << std::setw(2) << grid.src;
    }
    else {
	outs << " MONTHLY GRID -- Date: " << reference_date_time_.to_string();
	if (grid.fcst_time == 0) {
	  outs << "  Analysis Grid";
	}
	else {
	  outs << "  Valid Time: " << valid_date_time_.to_string();
	}
	outs << "  Grids in Average: " << std::setw(3) << grid.nmean << std::endl;
      outs << "   Format: NCAR Octagonal  Levels: " << std::setw(6) << grid.level1 << "mb " << std::setw(6) << grid.level2 << "mb  Parameter Code: " << std::setw(2) << grid.param << "  Source: " << std::setw(2) << grid.src;
    }
    if (!floatutils::myequalf(grid.pole,Grid::missing_value)) {
	outs << "  Pole: " << std::setw(8) << grid.pole << std::endl;
    }
    else {
	outs << "  Pole: N/A" << std::endl;
    }
    if (grid.filled) {
	outs << "   Minimum Value: " << std::setw(8) << stats.min_val << " at (" << stats.min_i << "," << stats.min_j << ")  Maximum Value: " << std::setw(8) << stats.max_val << " at (" << stats.max_i << "," << stats.max_j << ")  Average Value: " << std::setw(8) << stats.avg_val << std::endl;
    }
  }
  else {
    outs << "  Type=" << grid.grid_type << " Date=" << reference_date_time_.to_string("%Y%m%d%H") << " Valid=" << valid_date_time_.to_string("%Y%m%d%H") << " NAvg=" << grid.nmean << " Src=" << grid.src << " Param=" << grid.param;
    if (floatutils::myequalf(grid.level2,0.)) {
	outs << " Level=" << grid.level1 << " Pole=";
    }
    else {
	outs << " Levels=" << grid.level1 << "," << grid.level2 << " Pole=";
    }
    if (!floatutils::myequalf(grid.pole,Grid::missing_value)) {
	outs << grid.pole;
    }
    else {
	outs << "N/A";
    }
    if (grid.filled) {
	outs << " Min=" << stats.min_val << " Max=" << stats.max_val << " Avg=" << stats.avg_val << std::endl;
    }
    else {
	outs << std::endl;
    }
  }
}

void OctagonalGrid::bessel_interpolation(Grid *source)
{
  const float pi=3.141592654,rad_deg=180./pi,zk2=973.712023868;
  int n,m,i,j,ii,jj,avg_cnt=0;
  int ix,jy;
  double x,y,r2,lat,lon,dx,dy,dxx,dyy;
  double val[4][4],a,b,c,d;

  stats.max_val=-Grid::missing_value;
  stats.min_val=Grid::missing_value;
  stats.avg_val=0.;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
	if (m >= start[n] && m <= end[n]) {
	  x=m-23.;
	  y=n-25.;
	  r2=x*x+y*y;
	  lat=rad_deg*asin((zk2-r2)/(zk2+r2));
// because of the imprecision of floating point arithmetic, make sure that
// gridpoints that fall exactly on a 72x19 latitude are represented that way
	  if (fabs(round(lat)-lat) < 0.0001) {
	    lat=round(lat);
	  }
	  if (floatutils::myequalf(x,0.)) {
	    lon= (y <= 0.) ? 270. : 90.;
	  }
	  else {
	    lon=atan(y/x)*rad_deg;
// because of the imprecision of floating point arithmetic, make sure that
// gridpoints that fall exactly on a 72x19 longitude are represented that way
	    if (fabs(round(lon)-lon) < 0.0001) {
		lon=round(lon);
	    }
	    if (x < 0.) {
		lon=180.+lon;
	    }
	    else {
		if (y < 0.) {
		  lon=360.+lon;
		}
	    }
	  }
	  lon+=10.;
	  dx=lon/source->definition().loincrement;
	  dy=lat/source->definition().laincrement;
	  i=dx;
	  j=dy;
	  dx-=i;
// adjust for interpolation in direction of pole-to-equator
	  if (floatutils::myequalf(dy,0.)) {
	    --j;
	  }
	  else {
	    dy=j+1-dy;
	  }
// 16-point bessel interpolation
	  dxx=(dx-1.)*0.25;
	  dyy=(dy-1.)*0.25;
	  jy=j-2;
	  for (jj=0; jj < 4; ++jj) {
	    ++jy;
	    ix=i-2;
	    for (ii=0; ii < 4; ++ii) {
		++ix;
		if (jy == 19) {
		  jy=17;
		  ix+=36;
		}
		if (ix > 71) {
		  ix-=72;
		}
		else if (ix < 0) {
		  ix+=72;
		}
		val[jj][ii]=source->gridpoint(ix,jy);
	    }
	  }
	  a=val[3][1]+dx*(val[3][2]-val[3][1]+dxx*(val[3][3]-val[3][2]+val[3][0]-val[3][1]));
	  b=val[2][1]+dx*(val[2][2]-val[2][1]+dxx*(val[2][3]-val[2][2]+val[2][0]-val[2][1]));
	  c=val[1][1]+dx*(val[1][2]-val[1][1]+dxx*(val[1][3]-val[1][2]+val[1][0]-val[1][1]));
	  d=val[0][1]+dx*(val[0][2]-val[0][1]+dxx*(val[0][3]-val[0][2]+val[0][0]-val[0][1]));
	  gridpoints_[n][m]=b+dy*(c-b+dyy*(d-c+a-b));
	  if (gridpoints_[n][m] > stats.max_val) {
	    stats.max_val=gridpoints_[n][m];
	    stats.max_i=m+1;
	    stats.max_j=n+1;
	  }
	  if (gridpoints_[n][m] < stats.min_val) {
	    stats.min_val=gridpoints_[n][m];
	    stats.min_i=m+1;
	    stats.min_j=n+1;
	  }
	  stats.avg_val+=gridpoints_[n][m];
	  avg_cnt++;
	}
	else {
	  gridpoints_[n][m]=Grid::missing_value;
	}
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<float>(avg_cnt);
  }
}
