// FILE: latlongrid.cpp

#include <iomanip>
#include <grid.hpp>
#include <utils.hpp>
#include <bits.hpp>

int InputOldLatLonGridStream::peek()
{
  if (icosstream != nullptr) {
    return icosstream->peek();
  }
  else {
    unsigned char buffer[1725];
    int bytes_read=read(buffer,1725);
    if (bytes_read != bfstream::error && bytes_read != bfstream::eof) {
	fs.seekg(-bytes_read,std::ios_base::cur);
	--num_read;
    }
    return bytes_read;
  }
}

int InputOldLatLonGridStream::read(unsigned char *buffer,size_t buffer_length)
{
  if (buffer_length < 1725) {
    std::cerr << "Error: buffer overflow\n" << std::endl;
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
    fs.read(reinterpret_cast<char *>(buffer),1725);
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (!fs.good()) {
	return bfstream::error;
    }
    bytes_read=fs.gcount();
    if (bytes_read < 1725) {
	bytes_read=bfstream::error;
    }
  }
  ++num_read;
  return bytes_read;
}

OldLatLonGrid::OldLatLonGrid()
{
  def.laincrement=def.loincrement=5.;
  def.slatitude=15.;
  def.elatitude=90.;
  def.slongitude=0.;
  def.elongitude=355.;
  dim.x=72;
  dim.y=15;
  dim.size=1080;
  grid.level1=500.;
  grid.level2=0.;
}

OldLatLonGrid::OldLatLonGrid(const OldLatLonGrid& source)
{
  *this=source;
}

OldLatLonGrid& OldLatLonGrid::operator=(const OldLatLonGrid& source)
{
  int n,m;

  if (this == &source) {
    return *this;
  }
  reference_date_time_=source.reference_date_time_;
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
    if (source.num_in_sum == nullptr) {
	if (num_in_sum != nullptr) {
	  for (n=0; n < dim.y; ++n) {
	    delete[] num_in_sum[n];
	  }
	  delete[] num_in_sum;
	  num_in_sum=nullptr;
	}
    }
    else {
	if (num_in_sum == nullptr) {
	  num_in_sum=new size_t *[dim.y];
	  for (n=0; n < dim.y; ++n) {
	    num_in_sum[n]=new size_t[dim.x];
	  }
	}
	for (n=0; n < dim.y; ++n) {
	  for (m=0; m < dim.x; ++m) {
	    num_in_sum[n][m]=source.num_in_sum[n][m];
	  }
	}
    }
  }
  return *this;
}

size_t OldLatLonGrid::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void OldLatLonGrid::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  int n,m,l,avg_cnt=0;
  long long sum;
  int bias,scale,dum,*pval,off;
  short yr,mo,dy,time;

  if (stream_buffer == nullptr) {
    std::cerr << "Error: empty file stream\n" << std::endl;
    exit(1);
  }

// check the checksum for the grid
  if (checksum(stream_buffer,220,60,sum) != 0)
    std::cerr << "Warning: checksum error" << std::endl;

// unpack the grid header
  bits::get(stream_buffer,grid.grid_type,0,6);
  bits::get(stream_buffer,yr,6,7);
  bits::get(stream_buffer,mo,13,4);
  bits::get(stream_buffer,dy,17,5);
  bits::get(stream_buffer,time,22,5);
  reference_date_time_.set(1900+yr,mo,dy,time*10000);
  bits::get(stream_buffer,dum,27,10);
  grid.level1=1023.-dum;
  grid.level2=0.;
  bits::get(stream_buffer,grid.param,37,9);

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
    bits::get(stream_buffer,pval,180,12,0,1080);
    stats.max_val=-Grid::missing_value;
    stats.min_val=Grid::missing_value;
    stats.avg_val=0.;
    for (n=0,l=0; n < dim.y; n++) {
	for (m=0; m < dim.x; m++) {
	  if (pval[l] == 0)
	    gridpoints_[n][m]=Grid::missing_value;
	  else {
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
	  l++;
	}
    }
    grid.filled=true;
// use the pole value at 280E - agrees with pole value of Octagonal grid
// oriented 280E
    grid.pole=gridpoints_[14][56];
    if (avg_cnt > 0)
	stats.avg_val/=static_cast<float>(avg_cnt);

    delete[] pval;
  }
  else {
    off=12768;
    bits::get(stream_buffer,dum,180+off,12);
    if (dum == 0) {
	grid.pole=Grid::missing_value;
    }
    else {
	grid.pole=base+(dum-bias)*pow(2.,scale);
    }
  }
}

void OldLatLonGrid::print(std::ostream& outs) const
{
  int n,m,l;
  double start,stop,mm;

  if (!floatutils::myequalf(stats.avg_val,0.) && fabs(stats.avg_val) < 0.01 && fabs(stats.min_val) < 0.01 && fabs(stats.max_val) < 0.01) {
    outs.setf(std::ios::scientific);
  }
  else {
    outs.setf(std::ios::fixed);
  }
  outs.precision(1);
  if (grid.filled) {
    outs << std::endl;
    for (l=0; l < 72; l+=18) {
	start=l*5.;
	stop=start+85.;
	if (stop > def.elongitude) stop=def.elongitude;
	outs << "     \\ LON";
	for (mm=start; mm <= stop; mm+=5.) {
	  outs << std::setw(7) << mm << "E";
	}
	outs << std::endl;
	outs << "  LAT +---";
	for (mm=start; mm <= stop; mm+=5.) {
	  outs << "--------";
	}
	outs << std::endl;
	for (n=14; n >= 0; n--) {
	  outs << std::setw(4) << n*5.+20. << "N |   ";
	  for (m=l; m < l+18; m++) {
	    if (!floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
		outs << std::setw(8) << gridpoints_[n][m];
	    }
	    else {
		outs << "        ";
	    }
	  }
	  outs << std::endl;
	}
	outs << std::endl;
    }
    outs << std::endl;
  }
}

void OldLatLonGrid::print(std::ostream& outs,float start_latitude,float end_latitude,float start_longitude,float end_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
}

void OldLatLonGrid::print_ascii(std::ostream& outs) const
{
}

void OldLatLonGrid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{

  outs.setf(std::ios::fixed);
  outs.precision(1);
  bool scientific=false;
  if (!floatutils::myequalf(stats.avg_val,0.) && fabs(stats.avg_val) < 0.01) {
    scientific=true;
  }
  if (verbose) {
    outs << " DAILY GRID -- Date: " << reference_date_time_.to_string() << std::endl;
    outs << "   Format: NCAR 500mb Latitude/Longitude  Level: " << std::setw(4) << grid.level1 << "mb  Parameter Code: " << std::setw(2) << grid.param << "  Level: 500mb  Pole: ";
    if (floatutils::myequalf(grid.pole,Grid::missing_value)) {
	outs << "    N/A" << std::endl;
    }
    else {
	if (scientific) {
	  outs.unsetf(std::ios::fixed);
	  outs.setf(std::ios::scientific);
	  outs << grid.pole << std::endl;
	  outs.unsetf(std::ios::scientific);
	  outs.setf(std::ios::fixed);
	}
	else {
	  outs << std::setw(7) << grid.pole << std::endl;
	}
    }
    outs << "   Grid Definition: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  LonRange: " << std::setw(5) << def.slongitude << " to " << std::setw(5) << def.elongitude << " by " << std::setw(4) << def.loincrement << "  LatRange: " << std::setw(5) << def.slatitude << " to " << std::setw(5) << def.elatitude << " by " << std::setw(4) << def.laincrement << std::endl;
  }
  else {
    outs << " Type=" << grid.grid_type << " Date=" << reference_date_time_.to_string("%Y%m%d%H") << " Param=" << grid.param << " Level=500mb Pole=";
    if (!floatutils::myequalf(grid.pole,Grid::missing_value)) {
	if (scientific) {
	  outs.unsetf(std::ios::fixed);
	  outs.setf(std::ios::scientific);
	  outs << grid.pole;
	  if (grid.filled) {
	    outs << " Min=" << stats.min_val << " Max=" << stats.max_val << " Avg=" << stats.avg_val;
	  }
	  outs.unsetf(std::ios::scientific);
	  outs.setf(std::ios::fixed);
	}
	else {
	  outs << grid.pole;
	  if (grid.filled) {
	    outs << " Min=" << stats.min_val << " Max=" << stats.max_val << " Avg=" << stats.avg_val;
	  }
	}
    }
    else {
	outs << "N/A";
    }
    outs << std::endl;
  }
}
