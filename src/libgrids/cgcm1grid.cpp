// FILE: cgcm1grid.cpp

#include <iomanip>
#include <grid.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>

int InputCGCM1GridStream::peek()
{
return bfstream::error;
}

int InputCGCM1GridStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read,len,idum;
  size_t num_x,num_y,pack_dens;

  if (ics != nullptr) {
std::cerr << "Error: unable to read a COS-blocked CGCM1Grid stream" << std::endl;
exit(1);
  }

// read a grid from the stream
  fs.read(reinterpret_cast<char *>(buffer),71);
  if (fs.eof()) {
    return bfstream::eof;
  }
  else if (!fs.good()) {
    return bfstream::error;
  }
  bytes_read=fs.gcount();
  if (bytes_read < 71) {
    return bfstream::error;
  }
  else {
    if (num_read == 0) {
	bits::get(buffer,idum,0,32);
	if (idum == 0x40) {
	  is_binary=true;
	}
	else {
	  is_binary=false;
	}
    }
    if (is_binary) {
	bits::get(buffer,idum,32,32);
	if (idum != 0x47524944) {
	  std::cerr << "Error: not a CGCM1 record" << std::endl;
	  exit(1);
	}
	bits::get(buffer,num_x,320,32);
	bits::get(buffer,num_y,384,32);
	bits::get(buffer,pack_dens,512,32);
	pack_dens=64/pack_dens;
	len=(num_x*num_y*pack_dens+7)/8;
	len+=96;
	if (len > static_cast<int>(buffer_length)) {
	  std::cerr << "Error: buffer overflow" << std::endl;
	  exit(1);
	}
	fs.read(reinterpret_cast<char *>(&buffer[71]),len-71);
	bytes_read+=fs.gcount();
	if (bytes_read < len) {
	  bytes_read=bfstream::error;
	}
    }
    else {
	if (buffer[1] != 0x47 || buffer[2] != 0x52 || buffer[3] != 0x49 ||
          buffer[4] != 0x44 || buffer[70] != 0xa) {
	  std::cerr << "Error: not a CGCM1 record" << std::endl;
	  exit(1);
	}
	if (buffer_length < 56719) {
	  std::cerr << "Error: buffer overflow" << std::endl;
	  exit(1);
	}
	fs.read(reinterpret_cast<char *>(&buffer[71]),56648);
	bytes_read+=fs.gcount();
	if (bytes_read < 56719) {
	  bytes_read=bfstream::error;
	}
    }
  }
  ++num_read;
  return bytes_read;
}

CGCM1Grid::CGCM1Grid() : cgcm1()
{
  def.type=Grid::Type::gaussianLatitudeLongitude;
  def.slatitude=87.159;
  def.slongitude=0.;
  def.elatitude=-87.159;
  def.elongitude=356.25;
  def.num_circles=24;
  def.loincrement=3.75;
}

size_t CGCM1Grid::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void CGCM1Grid::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  char *cbuf=reinterpret_cast<char *>(const_cast<unsigned char *>(stream_buffer));
  union {
    int idum;
    float fdum;
  };
  if (stream_buffer == nullptr) {
    std::cerr << "Error: empty file stream" << std::endl;
    exit(1);
  }
  if (cbuf[4] == 'G' && cbuf[5] == 'R' && cbuf[6] == 'I' && cbuf[7] == 'D') {
    bits::get(stream_buffer,cgcm1.tstep,128,32);
    bits::get(stream_buffer,cgcm1.var_name,160,8,0,4);
    cgcm1.var_name[4]='\0';
    bits::get(stream_buffer,idum,256,32);
    grid.level1=idum;
    bits::get(stream_buffer,dim.x,336,16);
    bits::get(stream_buffer,dim.y,400,16);
    dim.size=dim.x*dim.y;
    bits::get(stream_buffer,grid.grid_type,464,16);
    bits::get(stream_buffer,cgcm1.pack_dens,528,16);
    bits::get(stream_buffer,idum,608,32);
    stats.min_val=fdum;
    bits::get(stream_buffer,idum,672,32);
    stats.max_val=fdum;
    if (!fill_header_only) {
	if (m_gridpoints != nullptr && dim.size > cgcm1.capacity) {
	  for (short n=0; n < dim.y; ++n) {
	    delete[] m_gridpoints[n];
	  }
	  delete[] m_gridpoints;
	  m_gridpoints=nullptr;
	}
	if (m_gridpoints == nullptr) {
	  m_gridpoints=new double *[dim.y];
	  for (short n=0; n < dim.y; ++n) {
	    m_gridpoints[n]=new double[dim.x];
	  }
	  cgcm1.capacity=dim.size;
	}
	int nbits=64/cgcm1.pack_dens;
	size_t *pval;
	if (nbits <= 32) {
	  pval=new size_t[dim.size];
	}
	else {
	  std::cerr << "Error: unable to unpack data field > 32 bits" << std::endl;
	  exit(1);
	}
	bits::get(stream_buffer,pval,736,nbits,0,dim.size);
	auto range=stats.max_val-stats.min_val;
	auto max=pow(2.,nbits)-1.;
	auto scale=range/max;
	auto avg_cnt=0;
	stats.avg_val=0.;
	auto cnt=0;
	for (short n=0; n < dim.y; ++n) {
	  for (short m=0; m < dim.x; ++m) {
	    m_gridpoints[n][m]=pval[cnt]*scale+stats.min_val;
	    stats.avg_val+=m_gridpoints[n][m];
	    ++avg_cnt;
	    ++cnt;
	  }
	}
	if (avg_cnt > 0) {
	  stats.avg_val/=static_cast<float>(avg_cnt);
	}
	grid.filled=true;
	delete[] pval;
    }
  }
  else {
    short yr,mo,dy,hr;
    if (cbuf[5] == ' ') {
	strutils::strget(&cbuf[9],yr,4);
	strutils::strget(&cbuf[13],mo,2);
	m_reference_date_time.set(yr,mo,0,0);
    }
    else {
	strutils::strget(&cbuf[5],yr,4);
	strutils::strget(&cbuf[9],mo,2);
	strutils::strget(&cbuf[11],dy,2);
	strutils::strget(&cbuf[13],hr,2);
	m_reference_date_time.set(yr,mo,dy,hr*10000);
    }
    m_valid_date_time=m_reference_date_time;
    std::copy(&cbuf[16],&cbuf[20],cgcm1.var_name);
    cgcm1.var_name[4]='\0';
    strutils::strget(&cbuf[20],idum,10);
    grid.level1=idum;
    strutils::strget(&cbuf[30],dim.x,10);
    strutils::strget(&cbuf[40],dim.y,10);
    if (dim.x == 97) {
	def.elongitude=360.;
    }
    strutils::strget(&cbuf[50],grid.grid_type,10);
    strutils::strget(&cbuf[60],cgcm1.pack_dens,10);
    dim.size=dim.x*dim.y;
    if (!fill_header_only) {
	if (m_gridpoints != nullptr && dim.size > cgcm1.capacity) {
	  for (short n=0; n < dim.y; ++n) {
	    delete[] m_gridpoints[n];
	  }
	  delete[] m_gridpoints;
	  m_gridpoints=nullptr;
	}
	if (m_gridpoints == nullptr) {
	  m_gridpoints=new double *[dim.y];
	  for (short n=0; n < dim.y; ++n) {
	    m_gridpoints[n]=new double[dim.x];
	  }
	  cgcm1.capacity=dim.size;
	}
	auto next=71;
	stats.min_val=Grid::MISSING_VALUE;
	stats.max_val=-Grid::MISSING_VALUE;
	auto avg_cnt=0;
	stats.avg_val=0.;
	for (short n=0; n < dim.y; ++n) {
	  for (short m=0; m < dim.x; ++m) {
	    if (cbuf[next] == 0xa) {
		++next;
	    }
	    strutils::strget(&cbuf[next],m_gridpoints[n][m],12);
	    if (m_gridpoints[n][m] > stats.max_val) {
		stats.max_val=m_gridpoints[n][m];
	    }
	    if (m_gridpoints[n][m] < stats.min_val) {
		stats.min_val=m_gridpoints[n][m];
	    }
	    stats.avg_val+=m_gridpoints[n][m];
	    ++avg_cnt;
	    next+=12;
	  }
	}
	if (avg_cnt > 0) {
	  stats.avg_val/=static_cast<float>(avg_cnt);
	}
	grid.filled=true;
    }
  }
}

void CGCM1Grid::print(std::ostream& outs) const
{
  if (grid.filled) {
    auto scientific=false;
    if (!floatutils::myequalf(stats.avg_val,0.) && fabs(stats.avg_val) < 0.01) {
	scientific=true;
    }
    outs.setf(std::ios::fixed);
    outs.precision(2);
    for (short m=0; m < dim.x; m+=15) {
	auto start=m+1;
	auto stop=m+15;
	if (stop > dim.x) stop=dim.x;
	outs << "\n \\ X  ";
	for (short l=start; l <= stop; ++l) {
	  outs << l;
	}
	outs << "\nY \\ +-";
	for (short l=start; l <= stop; ++l) {
	  outs << "---------";
	}
	outs << std::endl;
	for (short n=dim.y-1; n >= 0; --n) {
	  outs << n+1;
	  for (short l=start-1; l < stop; l++) {
	    if (floatutils::myequalf(m_gridpoints[n][l],Grid::MISSING_VALUE)) {
		outs << "         ";
	    }
	    else {
		if (scientific) {
		  outs.unsetf(std::ios::fixed);
		  outs.setf(std::ios::scientific);
		  outs.precision(3);
		}
		else {
		  outs.precision(2);
		}
		outs << std::setw(10) << m_gridpoints[n][l];
	    }
	  }
	  outs << std::endl;
	}
	outs << std::endl;
    }
    outs << std::endl;
  }
}

void CGCM1Grid::print(std::ostream& outs,float start_latitude,float end_latitude,float start_longitude,float end_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
}

void CGCM1Grid::print_ascii(std::ostream& outs) const
{
}

void CGCM1Grid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{

  outs.setf(std::ios::fixed);
  outs.precision(2);
  auto scientific=false;
  if (!floatutils::myequalf(stats.avg_val,0.) && fabs(stats.avg_val) < 0.01) {
    scientific=true;
  }
  if (verbose) {
    outs << "  Date: " << m_reference_date_time.to_string() << "  Level: " << std::setw(5) << grid.level1 << "  Variable " << cgcm1.var_name << std::endl;
    outs << "  Grid Type: " << std::setw(2) << grid.grid_type << "  Grid Dimensions: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  Points: " << std::setw(4) << dim.size;
    if (grid.filled) {
	if (scientific) {
	  outs.precision(3);
	  outs << "  Min Value=" << stats.min_val << " Max Value=" << stats.max_val << " Avg Value=" << stats.avg_val;
	  outs.precision(2);
	}
	else {
	  outs << "  Min Value=" << stats.min_val << " Max Value=" << stats.max_val << " Avg Value=" << stats.avg_val;
	}
    }
    else
	outs << std::endl;
  }
  else {
    outs << " Time=" << m_reference_date_time.to_string("%Y%m%d%H") << " Level=" << grid.level1 << " VarName=" << cgcm1.var_name << " Type=" << grid.grid_type << " NumX=" << dim.x << " NumY=" << dim.y << " NumPoints=" << dim.size;
    if (grid.filled) {
	if (scientific) {
	  outs.precision(3);
	  outs << " MinVal=" << stats.min_val << " MaxVal=" << stats.max_val << " AvgVal=" << stats.avg_val << std::endl;
	  outs.precision(2);
	}
	else {
	  outs << " MinVal=" << stats.min_val << " MaxVal=" << stats.max_val << " AvgVal=" << stats.avg_val << std::endl;
	}
    }
    else {
	outs << std::endl;
    }
  }
}
