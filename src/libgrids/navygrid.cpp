#include <iomanip>
#include <grid.hpp>
#include <utils.hpp>
#include <bits.hpp>

int InputNavyGridStream::peek()
{
  if (icosstream != nullptr)
    return icosstream->peek();
  else {
    return bfstream::error;
  }
}

int InputNavyGridStream::read(unsigned char *buffer,size_t buffer_length)
{
  if (buffer_length < 31500) {
    std::cerr << "Error: buffer overflow" << std::endl;
    exit(1);
  }
  int bytes_read;
// read a grid from the stream
  if (icosstream != nullptr) {
    if ( (bytes_read=icosstream->read(buffer,buffer_length)) <= 0)
      return bytes_read;
  }
  else {
    fs.read(reinterpret_cast<char *>(buffer),1);
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (!fs.good()) {
	return bfstream::error;
    }
    size_t fmt;
    bits::get(buffer,fmt,0,6);
    switch (fmt) {
	case 3:
	case 4:
	{
	  fs.read(reinterpret_cast<char *>(&buffer[1]),7972);
	  bytes_read=fs.gcount();
	  break;
	}
	default:
	{
	  return bfstream::error;
	}
    }
  }
  ++num_read;
  return bytes_read;
}

void NavyGrid::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  long long sum;
  size_t num_words;
  short yr,mo,dy,hr;
  int bias,scale,dum;
  GridDimensions old_dim;
  int n,m,cnt=0,avg_cnt=0,*pval;

  old_dim=dim;
  bits::get(stream_buffer,grid.grid_type,0,6);
  switch (grid.grid_type) {
    case 3:
    case 4:
	dim.x=dim.y=63;
	def.type=Grid::polarStereographicType;
	def.dx=def.dy=381.;
	if (grid.grid_type == 3) {
	  def.slatitude=-19.116;
	  def.slongitude=-125.;
	  def.llatitude=60.;
	  def.olongitude=-80.;
	  def.projection_flag=0;
	}
	else {
	  def.slatitude=19.116;
	  def.slongitude=145.;
	  def.llatitude=-60.;
	  def.olongitude=100.;
	  def.projection_flag=1;
	}
	break;
    default:
	std::cerr << "Error: grid type " << grid.grid_type << " not recognized" << std::endl;
	exit(1);
  }
  dim.size=dim.x*dim.y;
  bits::get(stream_buffer,yr,6,7);
  bits::get(stream_buffer,mo,13,4);
  bits::get(stream_buffer,dy,17,5);
  bits::get(stream_buffer,hr,22,5);
  reference_date_time_.set(1900+yr,mo,dy,hr*10000);
  bits::get(stream_buffer,dum,27,10);
  grid.level1=1023.-dum;
  bits::get(stream_buffer,grid.param,37,9);
  bits::get(stream_buffer,grid.fcst_time,46,9);
  grid.fcst_time*=10000;
  valid_date_time_=reference_date_time_.time_added(grid.fcst_time);
  bits::get(stream_buffer,grid.src,67,6);
  bits::get(stream_buffer,status,73,5);
  if (dy > 0) {
    grid.nmean=0;
  }
  else {
    bits::get(stream_buffer,grid.nmean,110,10);
  }
  grid.filled=false;
// check the checksum for the grid
  num_words=(dim.size*16+59)/60+4;
  if (checksum(stream_buffer,num_words,60,sum) != 0) {
    std::cerr << "Warning: checksum error - Date: " << reference_date_time_.to_string() << "  Level: " << grid.level1 << "  Parameter: " << grid.param << std::endl;
  }
  bits::get(stream_buffer,bias,78,16);
  bits::get(stream_buffer,scale,94,16);
  scale-=bias;
  auto base=floatutils::cdcconv(stream_buffer,120,1);
  if (!fill_header_only) {
// allocate gridpoint memory, if necessary
    if (gridpoints_ != nullptr && dim.size > old_dim.size) {
	for (n=0; n < old_dim.y; n++)
	  delete[] gridpoints_[n];
	delete[] gridpoints_;
	gridpoints_=nullptr;
    }
    if (gridpoints_ == nullptr) {
	gridpoints_=new double *[dim.y];
	for (n=0; n < dim.y; ++n) {
	  gridpoints_[n]=new double[dim.x];
	}
    }
    pval=new int[dim.size];
    bits::get(stream_buffer,pval,180,16,0,dim.size);
    stats.max_val=-Grid::missing_value;
    stats.min_val=Grid::missing_value;
    stats.avg_val=0.;
    grid.num_missing=0;
    for (n=0; n < dim.y; n++) {
	for (m=0; m < dim.x; m++) {
	  if (pval[cnt] > 0) {
	    gridpoints_[n][m]=base+(pval[cnt]-bias)*pow(2.,scale);
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
	  cnt++;
	}
    }
    if (avg_cnt > 0)
	stats.avg_val/=static_cast<float>(avg_cnt);
    switch (grid.grid_type) {
	case 10:
	case 11:
	  grid.pole=Grid::missing_value;
	  break;
	default:
	  grid.pole=gridpoints_[dim.y/2][dim.x/2];
    }
    grid.filled=true;
    delete[] pval;
  }
  else {
    switch (grid.grid_type) {
	case 10:
	case 11:
	  grid.pole=Grid::missing_value;
	  break;
	default:
	  pval=new int[1];
	  n=dim.y/2;
	  bits::get(stream_buffer,pval[0],180+(dim.x*n+n)*16,16);
	  if (pval[0] > 0) {
	    grid.pole=base+(pval[0]-bias)*pow(2.,scale);
	  }
	  else {
	    grid.pole=Grid::missing_value;
	  }
	  delete[] pval;
    }
  }
}

size_t NavyGrid::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void NavyGrid::print(std::ostream& outs) const
{
  int n,m,l,max_m;

  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (grid.filled) {
    for (m=0; m < dim.x; m+=16) {
	max_m=m+16;
	if (max_m > dim.x) {
	  max_m=dim.x;
	}
	outs << "\n   \\ X";
	for (l=m; l < max_m; ++l) {
	  outs << std::setw(8) << l+1;
	}
	outs << "\n  Y +-";
	for (l=m; l < max_m; ++l) {
	  outs << "--------";
	}
	outs << std::endl;
	for (n=dim.y-1; n >= 0; --n) {
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

void NavyGrid::print(std::ostream& outs,float start_latitude,float end_latitude,float start_longitude,float end_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
}

void NavyGrid::print_ascii(std::ostream& outs) const
{
}

void NavyGrid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (verbose) {
    if (reference_date_time_.day() > 0) {
	outs << " DAILY GRID -- Time: " << reference_date_time_.to_string() << "  Valid Time: " << valid_date_time_.to_string() << std::endl;
    }
    else {
	outs << " MONTHLY GRID -- Time: " << reference_date_time_.to_string() << "  Valid Time: " << valid_date_time_.to_string() << "  Grids in Average: " << std::setw(3) << grid.nmean << std::endl;
    }
    outs << "  Format: NCAR Navy  Level: " << std::setw(6) << grid.level1 << "mb  Parameter: " << std::setw(2) << grid.param << "  Source: " << std::setw(2) << grid.src;
    if (!floatutils::myequalf(grid.pole,Grid::missing_value)) {
	outs << "  Pole: " << std::setw(8) << grid.pole << std::endl;
    }
    else {
	outs << "  Pole: N/A" << std::endl;
    }
    outs << "  Grid Type: " << std::setw(2) << grid.grid_type << "  Dimensions: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  Number of Points: " << std::setw(5) << dim.size << std::endl;
    if (grid.filled) {
	outs << "  Minimum Value: " << std::setw(8) << stats.min_val << " at (" << stats.min_i << "," << stats.min_j << ")  Maximum Value: " << std::setw(8) << stats.max_val << " at (" << stats.max_i << "," << stats.max_j << ")  Average Value: " << std::setw(8) << stats.avg_val << std::endl;
    }
  }
  else {
    outs << " Time=" << reference_date_time_.to_string("%Y%m%d%H") << " ValidTime=" << valid_date_time_.to_string("%Y%m%d%H") << " Param=" << grid.param << " Level=" << grid.level1 << " Pole=";
    if (!floatutils::myequalf(grid.pole,Grid::missing_value)) {
	outs << grid.pole;
    }
    else {
	outs << "N/A";
    }
    outs << " Fmt=" << grid.grid_type << " Dim=" << dim.x << "x" << dim.y << " NumPoints=" << dim.size;
    if (grid.filled) {
	outs << " Min=" << stats.min_val << " Max=" << stats.max_val << " Avg=" <<  stats.avg_val;
    }
    outs << std::endl;
  }
}
