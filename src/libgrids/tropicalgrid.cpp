// FILE: octagonalgrid.cpp

#include <iomanip>
#include <complex>
#include <grid.hpp>
#include <utils.hpp>
#include <bits.hpp>

int InputTropicalGridStream::peek()
{
  if (icosstream != nullptr) {
    return icosstream->peek();
  }
  else {
    unsigned char buffer[2560];
    int bytes_read=0;
    bytes_read=read(buffer,2560);
    if (bytes_read != bfstream::error && bytes_read != bfstream::eof) {
	fs.seekg(-bytes_read,std::ios_base::cur);
	--num_read;
    }
    return bytes_read;
  }
}

int InputTropicalGridStream::read(unsigned char *buffer,size_t buffer_length)
{
  if (buffer_length < 2560) {
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
    fs.read(reinterpret_cast<char *>(buffer),2560);
    if (fs.eof()) {
	return bfstream::eof;
    }
    else {
	bytes_read=fs.gcount();
	if (bytes_read < 2560) {
	  bytes_read=bfstream::error;
	}
    }
  }
  ++num_read;
  return bytes_read;
}

TropicalGrid::TropicalGrid()
{
  dim.x=73;
  dim.y=23;
  dim.size=1679;
  def.type=Grid::mercatorType;
  def.slatitude=-48.09;
  def.slongitude=0.;
  def.elatitude=48.09;
  def.slongitude=360.;
  def.dx=5.;
  def.dy=4.37;
}

TropicalGrid::TropicalGrid(const TropicalGrid& source)
{
  *this=source;
}

TropicalGrid& TropicalGrid::operator=(const TropicalGrid& source)
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
	for (n=0; n < dim.y; n++)
	  gridpoints_[n]=new double[dim.x];
    }
    for (n=0; n < dim.y; n++) {
	for (m=0; m < dim.x; m++)
	  gridpoints_[n][m]=source.gridpoints_[n][m];
    }
  }
  return *this;
}

size_t TropicalGrid::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void TropicalGrid::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
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
  int lvl1;
  bits::get(stream_buffer,lvl1,27,10);
  grid.level1=1023.-lvl1;
  bits::get(stream_buffer,grid.param,37,9);
  bits::get(stream_buffer,grid.fcst_time,46,9);
  grid.fcst_time*=10000;
  valid_date_time_=reference_date_time_.time_added(grid.fcst_time);
  int lvl2;
  bits::get(stream_buffer,lvl2,55,10);
  grid.level2=1023.-lvl2;
  bits::get(stream_buffer,grid.src,65,6);
  if (dy == 0) {
    bits::get(stream_buffer,grid.nmean,100,8);
  }
  else {
    grid.nmean=0;
  }
  grid.filled=false;

// check the checksum for the grid
  long long sum;
  if (checksum(stream_buffer,341,60,sum) != 0) {
    std::cerr << "Warning: checksum error - Date: " << reference_date_time_.to_string() << "  Level: " << grid.level1 << "  Parameter: " << grid.param << std::endl;
  }
  int bias;
  bits::get(stream_buffer,bias,76,12);
  int scale;
  bits::get(stream_buffer,scale,88,12);
  scale-=bias;
  auto base=floatutils::cdcconv(stream_buffer,120,1);
// unpack the tropical grid gridpoints
  if (!fill_header_only) {
// if memory has not yet been allocated for the gridpoints, do it now
    if (gridpoints_ == nullptr) {
	gridpoints_=new double *[dim.y];
	for (int n=0; n < dim.y; ++n) {
	  gridpoints_[n]=new double[dim.x];
	}
    }
    auto pval=new int[dim.size];
    bits::get(stream_buffer,pval,216,12,0,dim.size);
    stats.max_val=-Grid::missing_value;
    stats.min_val=Grid::missing_value;
    stats.avg_val=0.;
    grid.num_missing=0;
    size_t avg_cnt=0;
    for (int n=0,l=0; n < dim.y; ++n) {
	for (int m=0; m < dim.x; ++m) {
	  if (m >= start[n] && m <= end[n]) {
	    gridpoints_[n][m]=base+(pval[l]-bias)*std::pow(2.,scale);
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
	  else {
	    gridpoints_[n][m]=Grid::missing_value;
	    grid.num_missing++;
	  }
	  ++l;
	}
    }
    if (avg_cnt > 0) {
	stats.avg_val/=static_cast<float>(avg_cnt);
    }
    grid.filled=true;
    delete[] pval;
  }
}

bool TropicalGrid::is_averaged_grid() const
{
  return (grid.nmean > 0);
}

void TropicalGrid::print(std::ostream& outs) const
{
  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (grid.filled) {
  }
}

void TropicalGrid::print(std::ostream& outs,float bottom_latitude,float top_latitude,float west_longitude,float east_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
}

void TropicalGrid::print_ascii(std::ostream& outs) const
{
  size_t precision,width;

  if (!grid.filled)
    return;

  outs.setf(std::ios::fixed);
  precision= (floatutils::precision(stats.min_val) > floatutils::precision(stats.avg_val)) ? floatutils::precision(stats.min_val) : floatutils::precision(stats.avg_val);
  precision= (precision > floatutils::precision(stats.max_val)) ? precision : floatutils::precision(stats.max_val);
  outs.precision(precision);
  width=7+precision;

  outs << reference_date_time_.to_string() << " " << std::setw(2) << grid.param << " " << std::setw(6) << grid.level1 << " " << std::setw(6) << grid.level2 << " " << std::setw(3) << dim.x << " " << std::setw(3) << dim.y << " " << std::setw(2) << width << " " << std::setw(2) << precision << std::endl;
}

void TropicalGrid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
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
      outs << "   Format: NCAR Tropical  Levels: " << std::setw(6) << grid.level1 << "mb " << std::setw(6) << grid.level2 << "mb  Parameter Code: " << std::setw(2) << grid.param << "  Source: " << std::setw(2) << grid.src;
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
      outs << "   Format: NCAR Tropical  Levels: " << std::setw(6) << grid.level1 << "mb " << std::setw(6) << grid.level2 << "mb  Parameter Code: " << std::setw(2) << grid.param << "  Source: " << std::setw(2) << grid.src;
    }
    if (grid.filled) {
	outs << "   Minimum Value: " << std::setw(8) << stats.min_val << " at (" << stats.min_i << "," << stats.min_j << ")  Maximum Value: " << std::setw(8) << stats.max_val << " at (" << stats.max_i << "," << stats.max_j << ")  Average Value: " << std::setw(8) << stats.avg_val << std::endl;
    }
  }
  else {
    outs << "  Type=" << grid.grid_type << " Date=" << reference_date_time_.to_string("%Y%m%d%H") << " Valid=" << valid_date_time_.to_string("%Y%m%d%H") << " NAvg=" << grid.nmean << " Src=" << grid.src << " Param=" << grid.param;
    if (floatutils::myequalf(grid.level2,0.)) {
	outs << " Level=" << grid.level1;
    }
    else {
	outs << " Levels=" << grid.level1 << "," << grid.level2;
    }
    if (grid.filled) {
	outs << " Min=" << stats.min_val << " Max=" << stats.max_val << " Avg=" << stats.avg_val << std::endl;
    }
    else {
	outs << std::endl;
    }
  }
}
