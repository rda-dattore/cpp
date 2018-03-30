#include <iomanip>
#include <grid.hpp>
#include <utils.hpp>
#include <bits.hpp>

int InputUSSRSLPGridStream::peek()
{
return bfstream::error;
}

int InputUSSRSLPGridStream::read(unsigned char *buffer,size_t buffer_length)
{
  if (buffer_length < 1158) {
    std::cerr << "Error: buffer overflow" << std::endl;
    exit(1);
  }
  if (icosstream != nullptr) {
    if (ptr_to_lrec == 2316) {
	int status;
	while ( (status=icosstream->read(block,2316)) == craystream::eof);
	if (status == craystream::eod) {
	  return bfstream::eof;
	}
	else if (status != 2316) {
	  return bfstream::error;
	}
	ptr_to_lrec=0;
    }
    std::copy(&block[ptr_to_lrec],&block[ptr_to_lrec+1158],buffer);
    ptr_to_lrec+=1158;
  }
  else {
    fs.read(reinterpret_cast<char *>(buffer),1158);
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (fs.gcount() != 1158) {
	return bfstream::error;
    }
  }
  ++num_read;
  return 1158;
}

USSRSLPGrid::USSRSLPGrid()
{
  dim.x=36;
  dim.y=16;
  dim.size=576;
  def.type=Grid::latitudeLongitudeType;
  def.slatitude=90.;
  def.elatitude=15.;
  def.laincrement=5.;
  def.slongitude=0.;
  def.elongitude=350.;
  def.loincrement=10.;
}

size_t USSRSLPGrid::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void USSRSLPGrid::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  short yr,mo,dy;
  int *pval,n,m;
  size_t cnt=0;
  size_t avg_cnt=0;

  if (stream_buffer == nullptr) {
    std::cerr << "Error: empty file stream" << std::endl;
    exit(1);
  }

  bits::get(stream_buffer,yr,0,16);
  bits::get(stream_buffer,mo,16,16);
  bits::get(stream_buffer,dy,32,16);
  if (yr < 1973) {
    reference_date_time_.set(yr,mo,dy,120000);
  }
  else {
    reference_date_time_.set(yr,mo,dy,0);
  }
  grid.filled=false;
  valid_date_time_=reference_date_time_;
// unpack the gridpoints
  if (!fill_header_only) {
// if memory has not yet been allocated for the gridpoints, do it now
    if (gridpoints_ == nullptr) {
	gridpoints_=new double *[dim.y];
	for (n=0; n < dim.y; n++)
	  gridpoints_[n]=new double[dim.x];
    }
    pval=new int[dim.size];
    bits::get(stream_buffer,pval,48,16,0,dim.size);
    stats.max_val=-Grid::missing_value;
    stats.min_val=Grid::missing_value;
    stats.avg_val=0.;
    for (n=0; n < dim.y; n++) {
	for (m=0; m < dim.x; m++,cnt++) {
	  if (pval[cnt] > 0x7fff)
	    pval[cnt]-=0x10000;
	  if (pval[cnt] != 0x7fff) {
	    gridpoints_[n][m]=(pval[cnt]+10000)*0.1;
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
	  else
	    gridpoints_[n][m]=Grid::missing_value;
	}
    }

    if (avg_cnt > 0)
	stats.avg_val/=static_cast<float>(avg_cnt);
    else {
	stats.avg_val=Grid::missing_value;
	stats.max_val=-stats.max_val;
    }
    grid.filled=true;
  }
}

void USSRSLPGrid::print(std::ostream& outs) const
{
  size_t n,m,l,start,stop;

  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (grid.filled) {
    for (n=0; n < 36; n+=18) {
      start=n;
      stop=n+18;
      outs << "\n    \\ LON";
      outs << std::setw(4) << start*10 << "E";
      for (l=start+1; l < stop; l++)
        outs << std::setw(7) << l*10 << "E";
      outs << std::endl;
      outs << " LAT +------------------------------------------------------------------------------------------------------------------------------------------------" << std::endl;
      for (m=0; m < 15; m++) {
        outs << std::setw(3) << 90-(m*5) << "N |";
        for (l=start; l < stop; l++) {
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

void USSRSLPGrid::print(std::ostream& outs,float bottom_latitude,float top_latitude,float west_longitude,float east_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
}

void USSRSLPGrid::print_ascii(std::ostream& outs) const
{
}

void USSRSLPGrid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
  if (verbose) {
    outs << " Date: " << reference_date_time_.to_string() << std::endl;
  }
  else {
    outs << " Date=" << reference_date_time_.to_string("%Y%m%d%H") << std::endl;
  }
}
