#include <iomanip>
#include <grid.hpp>
#include <utils.hpp>
#include <bits.hpp>

int InputUKSLPGridStream::peek()
{
return bfstream::error;
}

int InputUKSLPGridStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read;

  if (ics != nullptr) {
    if ( (bytes_read=ics->read(buffer,buffer_length)) <= 0)
	return bytes_read;
  }
  else {
    std::cerr << "Error: file is not COS-blocked" << std::endl;
    exit(1);
  }

  num_read++;
  return bytes_read;
}

UKSLPGrid::UKSLPGrid()
{
  dim.x=36;
  dim.y=15;
  dim.size=540;
  def.type=Grid::Type::latitudeLongitude;
  def.slatitude=85.;
  def.elatitude=15.;
  def.laincrement=5.;
  def.slongitude=0.;
  def.elongitude=350.;
  def.loincrement=10.;
}

size_t UKSLPGrid::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void UKSLPGrid::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  long long sum,ssum;
  short yr,mo,dy;
  int *pval;
  size_t cnt=0,avg_cnt=0;
  int n,m,l;

  if (stream_buffer == nullptr) {
    std::cerr << "Error: empty file stream" << std::endl;
    exit(1);
  }

// check the checksum for the grid
  if (checksum(stream_buffer,159,60,sum) != 0) {
    bits::get(stream_buffer,ssum,9480,60);
    std::cerr << "Warning: checksum error - computed checksum: " << std::hex << sum << "  stored checksum: " << ssum << std::dec << std::endl;
  }

  bits::get(stream_buffer,dy,32,16);
  dy/=10;
  bits::get(stream_buffer,mo,48,16);
  mo/=10;
  bits::get(stream_buffer,yr,64,16);
  yr/=10;
  m_reference_date_time.set(1900+yr,mo,dy,310000);
  grid.filled=false;

// unpack the gridpoints
  if (!fill_header_only) {
    grid.level1=1013.;
    pval=new int[dim.size+dim.x];
    bits::get(stream_buffer,pval[0],240,16);
    if (pval[0] > 0x7fff)
	pval[0]-=0x10000;
    if (pval[0] != 32768)
	grid.pole=1000.+(pval[0]/10.);
    else
	grid.pole=Grid::MISSING_VALUE;
// if memory has not yet been allocated for the gridpoints, do it now
    if (m_gridpoints == nullptr) {
	m_gridpoints=new double *[dim.y];
	for (n=0; n < dim.y; n++)
	  m_gridpoints[n]=new double[dim.x];
    }
    bits::get(stream_buffer,pval,256,16,0,dim.size+dim.x);
    stats.max_val=-Grid::MISSING_VALUE;
    stats.min_val=Grid::MISSING_VALUE;
    stats.avg_val=0.;
    for (l=0; l < dim.x; l++) {
	m=pval[cnt++];
	if (m > 0x7fff)
	  m-=0x10000;
	m/=10;
	m= (m <= 0) ? -m : (36-m);
	for (n=0; n < dim.y; n++) {
	  if (pval[cnt] > 0x7fff)
	    pval[cnt]-=0x10000;
	  if (pval[cnt] != -32768) {
	    m_gridpoints[n][m]=1000.+(pval[cnt]/10.);
	    if (m_gridpoints[n][m] > stats.max_val) {
		stats.max_val=m_gridpoints[n][m];
		stats.max_i=m+1;
		stats.max_j=n+1;
	    }
	    if (m_gridpoints[n][m] < stats.min_val) {
		stats.min_val=m_gridpoints[n][m];
		stats.min_i=m+1;
		stats.min_j=n+1;
	    }
	    stats.avg_val+=m_gridpoints[n][m];
	    avg_cnt++;
	  }
	  else
	    m_gridpoints[n][m]=Grid::MISSING_VALUE;
	  cnt++;
	}
    }

    if (avg_cnt > 0)
	stats.avg_val/=static_cast<float>(avg_cnt);
    else {
	stats.avg_val=Grid::MISSING_VALUE;
	stats.max_val=-stats.max_val;
    }
    delete[] pval;
    grid.filled=true;
  }
}

void UKSLPGrid::print(std::ostream& outs) const
{
  int n;
  size_t m,l,start,stop;

  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (grid.filled) {
    for (m=0; m < 36; m+=18) {
      start=m;
      stop=m+18;
      outs << "\n    \\ LON";
      outs << std::setw(4) << start*10 << "E";
      for (l=start+1; l < stop; l++)
        outs << std::setw(7) << l*10 << "E";
      outs << std::endl;
      outs << " LAT +------------------------------------------------------------------------------------------------------------------------------------------------" << std::endl;
      for (n=14; n >= 0; n--) {
        outs << std::setw(3) << 85-(n*5) << "N |";
        for (l=start; l < stop; l++) {
          if (floatutils::myequalf(m_gridpoints[n][l],Grid::MISSING_VALUE)) {
            outs << "        ";
	  }
          else {
            outs << std::setw(8) << m_gridpoints[n][l];
	  }
        }
        outs << std::endl;
      }
    }
    outs << std::endl;
  }
}

void UKSLPGrid::print(std::ostream& outs,float bottom_latitude,float top_latitude,float west_longitude,float east_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
}

void UKSLPGrid::print_ascii(std::ostream& outs) const
{
}

void UKSLPGrid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (verbose) {
    outs << " Date: " << m_reference_date_time.to_string() << std::endl;
    outs << "  Format: U.K. Sea-Level Pressure  Level: " << grid.level1 << "mb  Parameter: Sea-Level Pressure  Pole: ";
    if (floatutils::myequalf(grid.pole,Grid::MISSING_VALUE)) {
	outs << "    N/A" << std::endl;
    }
    else {
	outs << std::setw(7) << grid.pole << "mb" << std::endl;
    }
  }
  else {
    outs << " Date=" << m_reference_date_time.to_string("%Y%m%d%H") << " Pole=";
    if (floatutils::myequalf(grid.pole,Grid::MISSING_VALUE)) {
	outs << "    N/A";
    }
    else {
	outs << grid.pole;
    }
    if (grid.filled)
	outs << " Min=" << stats.min_val << " Max=" << stats.max_val << " Avg=" << stats.avg_val;
    outs << std::endl;
  }
}
