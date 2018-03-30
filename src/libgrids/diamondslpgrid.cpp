// FILE: diamondslpgrid.cpp

#include <iomanip>
#include <grid.hpp>
#include <utils.hpp>
#include <bits.hpp>

int InputDiamondSLPGridStream::peek()
{
return bfstream::error;
}

int InputDiamondSLPGridStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read;

  if (buffer_length < 2183) {
    std::cerr << "Error: buffer overflow" << std::endl;
    exit(1);
  }

// read a grid from the stream
  if (icosstream != nullptr) {
    if ( (bytes_read=icosstream->read(buffer,buffer_length)) <= 0) {
	return bytes_read;
    }
  }
  else {
    fs.read(reinterpret_cast<char *>(buffer),2183);
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (!fs.good()) {
	return bfstream::error;
    }
    bytes_read=fs.gcount();
    if (bytes_read < 2183) {
	bytes_read=bfstream::error;
    }
  }
  ++num_read;
  return bytes_read;
}

DiamondSLPGrid::DiamondSLPGrid()
{
  dim.y=16;
  dim.x=72;
  dim.size=1152;
  def.slatitude=10.;
  def.elatitude=85.;
  def.laincrement=5.;
  def.slongitude=0.;
  def.elongitude=355.;
  def.loincrement=5.;
}

DiamondSLPGrid::DiamondSLPGrid(const DiamondSLPGrid& source)
{
  *this=source;
}

DiamondSLPGrid& DiamondSLPGrid::operator=(const DiamondSLPGrid& source)
{
  int n,m,cnt=0;

  if (this == &source) {
    return *this;
  }
  reference_date_time_=source.reference_date_time_;
  dim=source.dim;
  def=source.def;
  grid=source.grid;
  stats.min_val=Grid::missing_value;
  stats.max_val=-Grid::missing_value;
  stats.avg_val=0.;
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
	  if (!floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	    if (gridpoints_[n][m] > stats.max_val) {
		stats.max_val=gridpoints_[n][m];
	    }
	    if (gridpoints_[n][m] < stats.min_val) {
		stats.min_val=gridpoints_[n][m];
	    }
	    stats.avg_val+=gridpoints_[n][m];
	    ++cnt;
	  }
	}
    }
  }
  if (cnt > 0) {
    stats.avg_val/=static_cast<float>(cnt);
  }
  return *this;
}

size_t DiamondSLPGrid::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
  if (!grid.filled) {
    return 0;
  }
  bits::set(output_buffer,grid.grid_type,0,6);
  bits::set(output_buffer,reference_date_time_.year(),6,11);
  bits::set(output_buffer,reference_date_time_.month(),17,4);
  bits::set(output_buffer,reference_date_time_.day(),21,5);
  auto hr=reference_date_time_.time()/10000;
  if ( (reference_date_time_.time() % 10000) >= 3000) {
    ++hr;
  }
  bits::set(output_buffer,hr,26,5);
  size_t level1=grid.level1;
  bits::set(output_buffer,level1,31,10);
  bits::set(output_buffer,grid.param,41,9);
  bits::set(output_buffer,grid.src,50,6);
  bits::set(output_buffer,1,56,4);
  if (!floatutils::myequalf(grid.pole,Grid::missing_value)) {
    bits::set(output_buffer,static_cast<int>(grid.pole*10.),60,15);
  }
  else {
    bits::set(output_buffer,0,60,15);
  }
  bits::set(output_buffer,grid.nmean,75,6);
  auto packed=new int[dim.size];
  auto cnt=0;
  for (short n=0; n < dim.y; ++n) {
    for (short m=0; m < dim.x; ++m) {
	if (gridpoints_[n][m] > 0.) {
	  packed[cnt]=lround(gridpoints_[n][m]*10.);
	}
	else {
	  packed[cnt]=0;
	}
	++cnt;
    }
  }
  bits::set(output_buffer,packed,120,15,0,dim.size);
  delete[] packed;
// pack the checksum
  long long sum;
  checksum(output_buffer,273,60,sum);
  bits::set(output_buffer,sum,16320,60);
  return 2048;
}

void DiamondSLPGrid::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  if (stream_buffer == nullptr) {
    std::cerr << "Error: empty file stream" << std::endl;
    exit(1);
  }
// check the checksum for the grid
  long long sum;
  if (checksum(stream_buffer,291,60,sum) != 0) {
    std::cerr << "Warning: checksum error" << std::endl;
  }
// unpack the SLP grid header
  bits::get(stream_buffer,grid.grid_type,0,6);
  short yr,mo,dy,time;
  bits::get(stream_buffer,yr,6,11);
  bits::get(stream_buffer,mo,17,4);
  bits::get(stream_buffer,dy,21,5);
  bits::get(stream_buffer,time,26,5);
  reference_date_time_.set(yr,mo,dy,time*10000);
  size_t level1;
  bits::get(stream_buffer,level1,31,10);
  grid.level1=level1;
  bits::get(stream_buffer,grid.param,41,9);
  bits::get(stream_buffer,grid.src,50,6);
  int ipole;
  bits::get(stream_buffer,ipole,60,15);
  if (ipole > 0 && ipole != 0x4000) {
    grid.pole=ipole;
  }
  else {
    grid.pole=Grid::missing_value;
  }
  bits::get(stream_buffer,grid.nmean,75,6);
  if (grid.nmean == 0 && dy == 0) {
    grid.nmean=999;
  }
  grid.filled=false;
// unpack the SLP grid gridpoints
  if (!fill_header_only) {
// if memory has not yet been allocated for the gridpoints, do it now
    if (gridpoints_ == nullptr) {
	gridpoints_=new double *[dim.y];
	for (short n=0; n < dim.y; ++n) {
	  gridpoints_[n]=new double[dim.x];
	}
    }
    auto pval=new int[dim.size];
    bits::get(stream_buffer,pval,120,15,0,dim.size);
    stats.max_val=-Grid::missing_value;
    stats.min_val=Grid::missing_value;
    stats.avg_val=0.;
    auto cnt=0,avg_cnt=0;
    for (short n=0; n < dim.y; ++n) {
	for (short m=0; m < dim.x; ++m,++cnt) {
	  if (pval[cnt] >= 16384) {
	    pval[cnt]-=16384;
	  }
	  if (pval[cnt] > 0) {
	    gridpoints_[n][m]=pval[cnt];
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
	    ++avg_cnt;
	  }
	  else
	    gridpoints_[n][m]=Grid::missing_value;
	}
    }
    stats.avg_val/=static_cast<float>(avg_cnt);
    grid.filled=true;
    delete[] pval;
  }
}

void DiamondSLPGrid::operator+=(const DiamondSLPGrid& source)
{
  int n,m;

  if (gridpoints_ == nullptr) {
    *this=source;
    reference_date_time_.set_day(0);
    grid.nmean=1;
  }
  else {
    if (reference_date_time_.time() != 310000) {
	if (reference_date_time_.time() != source.reference_date_time_.time()) {
	  reference_date_time_.set_time(310000);
	}
    }
    if (floatutils::myequalf(grid.pole,Grid::missing_value) || floatutils::myequalf(source.grid.pole,Grid::missing_value)) {
	std::cerr << "Warning: pole value contains missing data" << std::endl;
    }
    grid.pole+=source.grid.pole;
    for (n=0; n < dim.y; ++n) {
	for (m=0; m < dim.x; ++m) {
	  if (floatutils::myequalf(gridpoints_[n][m],Grid::missing_value) || floatutils::myequalf(source.gridpoints_[n][m],Grid::missing_value)) {
	    std::cerr << "Warning: gridpoint at " << n+1 << "," << m+1 << "contains " << "missing data - Grid: " << source.reference_date_time_.to_string() << std::endl;
	  }
	  gridpoints_[n][m]+=source.gridpoints_[n][m];
	}
    }
    ++grid.nmean;
  }
}

DiamondSLPGrid& DiamondSLPGrid::operator=(const GRIBGrid& source)
{
  int n,m,i,j;

  grid.filled=false;
// patch when NCEP leaves off the grid type
  if ((source.type() == 0 && source.definition().slatitude < 0.) || (source.type() != 0 && source.type() != 29)) {
    return *this;
  }
  if (source.source() != 7 || source.parameter() != 2 || source.first_level_type() != 102) {
    return *this;
  }
  grid.grid_type=10;
  reference_date_time_=source.reference_date_time();
  grid.level1=1013.;
  grid.param=6;
  grid.src=28;
  if (source.scan_mode() == 0x40) {
    grid.pole=source.gridpoint(144,36)*0.01;
  }
  else {
    grid.pole=source.gridpoint(0,0)*0.01;
  }
  if (source.is_averaged_grid()) {
    grid.nmean=source.number_averaged();
  }
  else {
    grid.nmean=0;
  }
  if (gridpoints_ == nullptr) {
    gridpoints_=new double *[dim.y];
    for (n=0; n < dim.y; ++n) {
	gridpoints_[n]=new double[dim.x];
    }
  }
// SLP grid goes from 15N to 85N
  for (n=6,j=0; n < 36; n+=2,++j) {
// SLP grid goes from 0E to 355E
    for (m=0,i=0; m < 144; m+=2,++i) {
	if (floatutils::myequalf(source.gridpoint(m,n),Grid::missing_value)) {
	  gridpoints_[j][i]=0.;
	}
	else {
	  gridpoints_[j][i]=source.gridpoint(m,n)*0.01;
	}
    }
  }
  grid.filled=true;
  return *this;
}

void DiamondSLPGrid::print(std::ostream& outs) const
{
  size_t n,l;
  int m;
  size_t start,stop;

  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (grid.filled) {
    for (n=0; n < 72; n+=18) {
	start=n;
	stop=n+18;
	outs << "\n    \\ LON";
	outs << std::setw(4) << start*5 << "E";
	for (l=start+1; l < stop; l++)
	  outs << std::setw(7) << l*5 << "E";
	outs << std::endl;
	outs << " LAT +------------------------------------------------------------------------------------------------------------------------------------------------" << std::endl;
	for (m=15; m >= 0; m--) {
	  outs << std::setw(3) << (m*5)+10 << "N |";
	  for (l=start; l < stop; ++l) {
	    if (floatutils::myequalf(gridpoints_[m][l],Grid::missing_value)) {
		outs << "        ";
	    }
	    else {
		outs << std::setw(8) << gridpoints_[m][l];
	    }
	  }
	  outs << std::endl;
	}
    }
    outs << std::endl;
  }
}

void DiamondSLPGrid::print(std::ostream& outs,float bottom_latitude,float top_latitude,float west_longitude,float east_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
  int bottom_lat,top_lat;
  int west_lon,east_lon;
  int n,m,l,nn=1,mm=1,ll;
  int start,stop;

  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (grid.filled) {
    if ( (bottom_lat=latitude_index_of(bottom_latitude,nullptr)) < 0) {
	bottom_lat=latitude_index_south_of(bottom_latitude,nullptr);
    }
    if ( (top_lat=latitude_index_of(top_latitude,nullptr)) < 0) {
	top_lat=latitude_index_north_of(top_latitude,nullptr);
    }
    if ( (west_lon=longitude_index_of(west_longitude)) < 0) {
	west_lon=longitude_index_west_of(west_longitude);
    }
    if ( (east_lon=longitude_index_of(east_longitude)) < 0) {
	east_lon=longitude_index_east_of(east_longitude);
    }
    if ((west_lon == east_lon && full_longitude_strip) || west_lon > east_lon) {
	east_lon+=72;
    }
    if (floatutils::myequalf(lat_increment,0.)) {
	lat_increment=def.laincrement;
    }
    if (floatutils::myequalf(lon_increment,0.)) {
	lon_increment=def.loincrement;
    }
    if (lon_increment > 0) {
	nn=lroundf(lon_increment/def.loincrement);
    }
    if (lat_increment > 0) {
	mm=lroundf(lat_increment/def.laincrement);
    }
    for (n=west_lon; n <= east_lon; n+=18*nn) {
	start=n;
	stop=n+17*nn;
	if (stop > east_lon) {
	  stop=east_lon;
	}
	outs << "\n    \\ LON";
	if (start < 72) {
	  if (start <= 36) {
	    outs << std::setw(4) << start*5 << "E";
	  }
	  else {
	    outs << std::setw(4) << 360-start*5 << "W";
	  }
	}
	else {
	  ll=start-72;
	  if (ll <= 36) {
	    outs << std::setw(4) << ll*5 << "E";
	  }
	  else {
	    outs << std::setw(4) << 360-ll*5 << "W";
	  }
	}
	for (l=ll=start+nn; l <= stop; l+=nn,ll+=nn) {
	  if (ll >= 72) {
	    ll-=72;
	  }
	  if (ll <= 36) {
	    outs << std::setw(7) << ll*5 << "E";
	  }
	  else {
	    outs << std::setw(7) << 360-ll*5 << "W";
	  }
	}
	outs << std::endl;
	outs << " LAT +";
	for (l=start; l <= stop; l+=nn) {
	  outs << "--------";
	}
	outs << std::endl;
	for (m=top_lat; m >= bottom_lat; m-=mm) {
	  outs << std::setw(3) << (m*5)+15 << "N |";
	  if (m == 15) {
	    for (l=start; l <= stop; l+=nn) {
		if (floatutils::myequalf(grid.pole,Grid::missing_value)) {
		  outs << "        ";
		}
		else {
		  outs << std::setw(8) << grid.pole;
		}
	    }
	  }
	  else {
	    for (l=ll=start; l <= stop; l+=nn,ll+=nn) {
		if (ll >= 72) {
		  ll-=72;
		}
		if (floatutils::myequalf(gridpoints_[m][ll],Grid::missing_value)) {
		  outs << "        ";
		}
		else {
		  outs << std::setw(8) << gridpoints_[m][ll];
		}
	    }
	  }
	  outs << std::endl;
	}
    }
    outs << std::endl;
  }
}

void DiamondSLPGrid::print_ascii(std::ostream& outs) const
{
}

void DiamondSLPGrid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (verbose) {
    if (reference_date_time_.day() > 0) {
	outs << " DAILY GRID -- Date: " << reference_date_time_.to_string() << std::endl;
	outs << "   Format: NCAR Diamond Sea-Level Pressure  Level: " << std::setw(4) << grid.level1 << "mb  Parameter: Sea-Level Pressure  Source: " << std::setw(2) << grid.src << "  Pole: ";
    }
    else {
	outs << " MONTHLY GRID -- Date: " << reference_date_time_.to_string() << "  Grids in Average: " << std::setw(3) << grid.nmean << std::endl;
	outs << "   Format: NCAR Sea-Level Pressure  Level: " << std::setw(4) << grid.level1 << "mb  Parameter: Sea-Level Pressure  Source: " << std::setw(2) << grid.src << "  Pole: ";
    }
    if (floatutils::myequalf(grid.pole,Grid::missing_value)) {
	outs << "    N/A" << std::endl;
    }
    else {
	outs << std::setw(7) << grid.pole << "mb" << std::endl;
    }
  }
  else {
    outs << " Type=" << grid.grid_type << " Date=" << reference_date_time_.to_string("%Y%m%d%H") << " NAvg=" << grid.nmean << " Src=" << grid.src << " Param=" << grid.param << " Level=" << grid.level1 << " Pole=";
    if (!floatutils::myequalf(grid.pole,Grid::missing_value)) {
	outs << grid.pole;
    }
    else {
	outs << "N/A";
    }
    if (grid.filled) {
	outs << " Min=" << stats.min_val << " Max=" << stats.max_val << " Avg=" << stats.avg_val;
    }
    outs << std::endl;
  }
}
