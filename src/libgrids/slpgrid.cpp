// FILE: slpgrid.cpp

#include <iomanip>
#include <grid.hpp>
#include <utils.hpp>
#include <bits.hpp>

int InputSLPGridStream::peek()
{
  if (ics != nullptr) {
    return ics->peek();
  }
  else {
    unsigned char buffer[2048];
    int bytes_read=read(buffer,2048);
    if (bytes_read != bfstream::error && bytes_read != bfstream::eof) {
	fs.seekg(-bytes_read,std::ios_base::cur);
	--num_read;
    }
    return bytes_read;
  }
}

int InputSLPGridStream::read(unsigned char *buffer,size_t buffer_length)
{
  if (buffer_length < 2048) {
    std::cerr << "Error: buffer overflow" << std::endl;
    exit(1);
  }
  int bytes_read;
// read a grid from the stream
  if (ics != nullptr) {
    if ( (bytes_read=ics->read(buffer,buffer_length)) <= 0) {
	return bytes_read;
    }
  }
  else {
    fs.read(reinterpret_cast<char *>(buffer),2048);
    bytes_read=fs.gcount();
    if (bytes_read == 0) {
	return bfstream::eof;
    }
    else if (bytes_read < 2048) {
	bytes_read=bfstream::error;
    }
  }
  ++num_read;
  return bytes_read;
}

SLPGrid::SLPGrid()
{
  dim.y=16;
  dim.x=72;
  dim.size=1080;
  def.type=Grid::Type::latitudeLongitude;
  def.slatitude=15.;
  def.elatitude=90.;
  def.laincrement=5.;
  def.slongitude=0.;
  def.elongitude=355.;
  def.loincrement=5.;
}

SLPGrid::SLPGrid(const SLPGrid& source)
{
  *this=source;
}

SLPGrid& SLPGrid::operator=(const SLPGrid& source)
{
  int n,m,cnt=0;
  int dimy=dim.y-1;

  if (this == &source) {
    return *this;
  }
  m_reference_date_time=source.m_reference_date_time;
  m_valid_date_time=source.m_valid_date_time;
  dim=source.dim;
  def=source.def;
  grid=source.grid;
  stats.min_val=Grid::MISSING_VALUE;
  stats.max_val=-Grid::MISSING_VALUE;
  stats.avg_val=0.;
  if (source.grid.filled) {
    if (m_gridpoints == nullptr) {
	m_gridpoints=new double *[dimy];
	for (n=0; n < dimy; ++n) {
	  m_gridpoints[n]=new double[dim.x];
	}
    }
    for (n=0; n < dimy; ++n) {
	for (m=0; m < dim.x; ++m) {
	  m_gridpoints[n][m]=source.m_gridpoints[n][m];
	  if (!floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE)) {
	    if (m_gridpoints[n][m] > stats.max_val)
            stats.max_val=m_gridpoints[n][m];
	    if (m_gridpoints[n][m] < stats.min_val)
            stats.min_val=m_gridpoints[n][m];
	    stats.avg_val+=m_gridpoints[n][m];
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

void SLPGrid::compute_mean()
{
  dim.y--;
  Grid::compute_mean();
  dim.y++;
}

size_t SLPGrid::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
  int n,m,cnt=0;
  long long sum;
  short hr;
  int *packed;
  int dum;
  int dimy=dim.y-1;

  if (!grid.filled)
    return 0;
  bits::set(output_buffer,grid.grid_type,0,6);
  bits::set(output_buffer,m_reference_date_time.year(),6,11);
  bits::set(output_buffer,m_reference_date_time.month(),17,4);
  bits::set(output_buffer,m_reference_date_time.day(),21,5);
  hr=m_reference_date_time.time()/10000;
  if ( (m_reference_date_time.time() % 10000) >= 3000)
    hr++;
  bits::set(output_buffer,hr,26,5);
  dum=grid.level1;
  bits::set(output_buffer,dum,31,10);
  bits::set(output_buffer,grid.param,41,9);
  bits::set(output_buffer,grid.src,50,6);
  bits::set(output_buffer,1,56,4);
  if (!floatutils::myequalf(grid.pole,Grid::MISSING_VALUE)) {
    bits::set(output_buffer,lround(grid.pole*10.),60,15);
  }
  else {
    bits::set(output_buffer,0,60,15);
  }
  bits::set(output_buffer,grid.nmean,75,6);
  packed=new int[dim.size];
  for (n=0; n < dimy; n++) {
    for (m=0; m < dim.x; m++) {
	if (floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE)) {
	  packed[cnt]=0x4000;
	}
	else {
	  packed[cnt]=lround(m_gridpoints[n][m]*10.);
	  if (imap[n][m] == 1) {
	    packed[cnt]|=0x4000;
	  }
	}
	++cnt;
    }
  }
  bits::set(output_buffer,packed,120,15,0,dim.size);
  delete[] packed;
// pack the checksum
  checksum(output_buffer,273,60,sum);
  bits::set(output_buffer,sum,16320,60);
  return 2048;
}

void SLPGrid::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  int n,m,ipole,cnt=0,avg_cnt=0;
  long long sum;
  short yr,mo,dy,time;
  int *pval;
  int dum;
  int dimy=dim.y-1;

  if (stream_buffer == nullptr) {
    std::cerr << "Error: empty file stream" << std::endl;
    exit(1);
  }
// check the checksum for the grid
  if (checksum(stream_buffer,273,60,sum) != 0)
    std::cerr << "Warning: checksum error" << std::endl;
// unpack the SLP grid header
  bits::get(stream_buffer,grid.grid_type,0,6);
  bits::get(stream_buffer,yr,6,11);
  bits::get(stream_buffer,mo,17,4);
  bits::get(stream_buffer,dy,21,5);
  bits::get(stream_buffer,time,26,5);
  m_reference_date_time.set(yr,mo,dy,time*10000);
  m_valid_date_time=m_reference_date_time;
  bits::get(stream_buffer,dum,31,10);
  grid.level1=dum;
  bits::get(stream_buffer,grid.param,41,9);
  bits::get(stream_buffer,grid.src,50,6);
  if (grid.src == 15 || grid.src == 16) {
// pre-IGY and IGY NOTOS grids over the S. Hemisphere
    def.slatitude=-15.;
    def.elatitude=-90.;
  }
  bits::get(stream_buffer,ipole,60,15);
  if (ipole > 0 && ipole != 0x4000)
    grid.pole=ipole*0.1;
  else
    grid.pole=Grid::MISSING_VALUE;
  bits::get(stream_buffer,grid.nmean,75,6);
  if (grid.nmean == 0 && dy == 0)
    grid.nmean=999;
  grid.filled=false;
// unpack the SLP grid gridpoints
  if (!fill_header_only) {
// if memory has not yet been allocated for the gridpoints, do it now
    if (m_gridpoints == nullptr) {
	m_gridpoints=new double *[dimy];
	for (n=0; n < dimy; n++)
	  m_gridpoints[n]=new double[dim.x];
    }
    pval=new int[dim.size];
    bits::get(stream_buffer,pval,120,15,0,dim.size);
    stats.max_val=-Grid::MISSING_VALUE;
    stats.min_val=Grid::MISSING_VALUE;
    stats.avg_val=0.;
    for (n=0; n < dimy; n++) {
	for (m=0; m < dim.x; m++,cnt++) {
	  imap[n][m]=0;
	  if (pval[cnt] >= 16384) {
	    pval[cnt]-=16384;
	    imap[n][m]=1;
	  }
	  if (pval[cnt] > 0) {
	    m_gridpoints[n][m]=pval[cnt]*0.1;
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
	  else {
	    m_gridpoints[n][m]=Grid::MISSING_VALUE;
	    imap[n][m]=1;
	  }
	}
    }

    stats.avg_val/=static_cast<float>(avg_cnt);
    grid.filled=true;

    delete[] pval;
  }
}

void SLPGrid::operator+=(const SLPGrid& source)
{
  int n,m;
  int dimy=dim.y-1;

  if (m_gridpoints == nullptr) {
    *this=source;
    m_reference_date_time.set_day(0);
    m_valid_date_time=m_reference_date_time;
    grid.nmean=1;
    num_in_sum=new size_t *[dimy];
    for (n=0; n < dimy; n++) {
	num_in_sum[n]=new size_t[dim.x];
	for (m=0; m < dim.x; m++) {
	  imap[n][m]=1;
	  if (floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE)) {
	    num_in_sum[n][m]=0;
	  }
	  else {
	    num_in_sum[n][m]=1;
	  }
	}
    }
    if (floatutils::myequalf(grid.pole,Grid::MISSING_VALUE)) {
	grid.num_in_pole_sum=0;
    }
    else {
	grid.num_in_pole_sum=1;
    }
  }
  else {
    if (m_reference_date_time.time() != 310000) {
	if (m_reference_date_time.time() != source.m_reference_date_time.time()) {
	  m_reference_date_time.set_time(310000);
	  m_valid_date_time=m_reference_date_time;
	}
    }
    if (!floatutils::myequalf(source.grid.pole,Grid::MISSING_VALUE)) {
	if (floatutils::myequalf(grid.pole,Grid::MISSING_VALUE)) {
	  grid.pole=source.grid.pole;
	}
	else {
	  grid.pole+=source.grid.pole;
	}
	++grid.num_in_pole_sum;
    }
    for (n=0; n < dimy; n++) {
	for (m=0; m < dim.x; m++) {
	  if (!floatutils::myequalf(source.m_gridpoints[n][m],Grid::MISSING_VALUE)) {
	    if (floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE)) {
		m_gridpoints[n][m]=source.m_gridpoints[n][m];
	    }
	    else {
		m_gridpoints[n][m]+=source.m_gridpoints[n][m];
	    }
	    num_in_sum[n][m]++;
	    if (num_in_sum[n][m] >= 10) {
		imap[n][m]=0;
	    }
	  }
	}
    }
    ++grid.nmean;
  }
}

SLPGrid& SLPGrid::operator=(const GRIBGrid& source)
{
  int n,m,i,j,latinc,loninc;
  int dimy=dim.y-1;

  grid.filled=false;
// patch when NCEP leaves off the grid type
  if ((source.type() == 0 && source.definition().slatitude < 0.) || (source.type() != 0 && source.type() != 29)) {
    return *this;
  }
  if (source.source() != 7 || source.parameter() != 2 || source.first_level_type() != 102) {
    return *this;
  }
  grid.grid_type=10;
  m_reference_date_time=source.reference_date_time();
  m_valid_date_time=source.valid_date_time();
  grid.level1=1013.;
  grid.param=6;
  grid.src=28;
  if (m_gridpoints == nullptr) {
    m_gridpoints=new double *[dimy];
    for (n=0; n < dimy; ++n) {
	m_gridpoints[n]=new double[dim.x];
    }
  }
  latinc=lround(5./source.definition().laincrement);
  loninc=lround(5./source.definition().loincrement);
  if (source.scan_mode() == 0x40) {
    grid.pole=source.gridpoint(source.dimensions().x-1,source.dimensions().y-1)*0.01;
// SLP grid goes from 15N to 85N
    n=lround((15.-source.definition().slatitude)/source.definition().laincrement);
    for (j=0; n < source.dimensions().y-1; n+=latinc,j++) {
// SLP grid goes from 0E to 355E
	for (m=0,i=0; m < source.dimensions().x-1; m+=loninc,i++) {
	  if (floatutils::myequalf(source.gridpoint(m,n),Grid::MISSING_VALUE)) {
	    m_gridpoints[j][i]=0.;
	    imap[j][i]=1;
	  }
	  else {
	    m_gridpoints[j][i]=source.gridpoint(m,n)*0.01;
	    imap[j][i]=0;
	  }
	}
    }
  }
  else {
    grid.pole=source.gridpoint(0,0)*0.01;
// SLP grid goes from 15N to 85N
    n=lround((source.definition().slatitude-15.)/source.definition().laincrement);
    for (j=0; n > 0; n-=latinc,++j) {
// SLP grid goes from 0E to 355E
	for (m=0,i=0; m < source.dimensions().x-1; m+=loninc,++i) {
	  if (floatutils::myequalf(source.gridpoint(m,n),Grid::MISSING_VALUE)) {
	    m_gridpoints[j][i]=0.;
	    imap[j][i]=1;
	  }
	  else {
	    m_gridpoints[j][i]=source.gridpoint(m,n)*0.01;
	    imap[j][i]=0;
	  }
	}
    }
  }
  if (source.is_averaged_grid()) {
    grid.nmean=source.number_averaged();
  }
  else {
    grid.nmean=0;
  }
  grid.filled=true;
  return *this;
}

SLPGrid& SLPGrid::operator=(const GRIB2Grid& source)
{
  int n,m,i,j,latinc,loninc;
  int dimy=dim.y-1;

  grid.filled=false;
  if (source.source() != 7 || source.discipline() != 0 || source.parameter_category() != 3 || source.parameter() != 1 || source.first_level_type() != 101) {
    return *this;
  }
  grid.grid_type=10;
  m_reference_date_time=source.reference_date_time();
  m_valid_date_time=source.valid_date_time();
  grid.level1=1013.;
  grid.param=6;
  grid.src=28;
  if (m_gridpoints == nullptr) {
    m_gridpoints=new double *[dimy];
    for (n=0; n < dimy; ++n) {
	m_gridpoints[n]=new double[dim.x];
    }
  }
  latinc=lround(5./source.definition().laincrement);
  loninc=lround(5./source.definition().loincrement);
  if (source.scan_mode() == 0) {
    grid.pole=source.gridpoint(0,0)*0.01;
// SLP grid goes from 15N to 85N
    n=lround((source.definition().slatitude-15.)/source.definition().laincrement);
    for (j=0; n > 0; n-=latinc,++j) {
// SLP grid goes from 0E to 355E
	for (m=0,i=0; m < source.dimensions().x-1; m+=loninc,i++) {
	  if (floatutils::myequalf(source.gridpoint(m,n),Grid::MISSING_VALUE)) {
	    m_gridpoints[j][i]=0.;
	    imap[j][i]=1;
	  }
	  else {
	    m_gridpoints[j][i]=source.gridpoint(m,n)*0.01;
	    imap[j][i]=0;
	  }
	}
    }
  }
  else {
    std::cerr << "Error: unable to convert grid with scan mode " << source.scan_mode() << std::endl;
    exit(1);
  }
  if (source.is_averaged_grid()) {
    grid.nmean=source.number_averaged();
  }
  else {
    grid.nmean=0;
  }
  grid.filled=true;
  return *this;
}

SLPGrid& SLPGrid::operator=(const UKSLPGrid& source)
{
  int n,m;
  int l,k,ll,kk;
  size_t avg_cnt=0;
  double f=1./16.,f9=f*9.;
  int dimy=dim.y-1;

  grid.grid_type=10;
  m_reference_date_time=source.reference_date_time();
  m_valid_date_time=source.valid_date_time();
  grid.level1=1013.;
  grid.param=6;
  grid.src=12;
  if (source.is_averaged_grid()) {
    grid.nmean=source.number_averaged();
  }
  else {
    grid.nmean=0;
  }
  grid.pole=source.pole_value();
  if (m_gridpoints == nullptr) {
    m_gridpoints=new double *[dimy];
    for (n=0; n < dimy; ++n) {
	m_gridpoints[n]=new double[dim.x];
    }
  }

  stats.min_val=Grid::MISSING_VALUE;
  stats.max_val=-Grid::MISSING_VALUE;
  stats.avg_val=0.;
  for (n=0,k=dimy-1; n < dimy; ++n,--k) {
    for (m=0,l=0; m < dim.x; ++m) {
	if ( (m % 2) == 0) {
	  m_gridpoints[n][m]=source.gridpoint(l,k);
	  if (!floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE)) {
	    imap[n][m]=0;
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
	    ++avg_cnt;
	  }
	  else {
	    imap[n][m]=1;
	  }
	  ++l;
	}
	else {
	  m_gridpoints[n][m]=Grid::MISSING_VALUE;
	  imap[n][m]=1;
	}
    }
  }
// fill in missing points on a latitude line that have non-missing data on
//   either side of the missing point - do this only for original data points
  for (n=0; n < dimy; ++n) {
    for (m=0; m < dim.x; m+=2) {
	if (floatutils::myequalf(m_gridpoints[n][m],Grid::MISSING_VALUE)) {
	  k=m-2;
	  if (k < 0) {
	    k+=72;
	  }
	  if (!floatutils::myequalf(m_gridpoints[n][k],Grid::MISSING_VALUE)) {
	    l=m+2;
	    if (l >= dim.x) {
		l-=dim.x;
	    }
	    if (!floatutils::myequalf(m_gridpoints[n][l],Grid::MISSING_VALUE)) {
		m_gridpoints[n][m]=(m_gridpoints[n][k]+m_gridpoints[n][l])*0.5;
	    }
	  }
	}
    }
  }
// interpolate the non-original data points
  for (n=0; n < dimy; ++n) {
    for (m=1; m < dim.x; m+=2) {
	k=m-1;
	if (!floatutils::myequalf(m_gridpoints[n][k],Grid::MISSING_VALUE)) {
	  l=m+1;
	  if (l >= dim.x) {
	    l-=dim.x;
	  }
	  if (!floatutils::myequalf(m_gridpoints[n][l],Grid::MISSING_VALUE)) {
	    kk=m-3;
	    if (kk < 0) {
		kk+=72;
	    }
	    ll=m+3;
	    if (ll >= dim.x) {
		ll-=dim.x;
	    }
	    if (!floatutils::myequalf(m_gridpoints[n][kk],Grid::MISSING_VALUE) && !floatutils::myequalf(m_gridpoints[n][ll],Grid::MISSING_VALUE)) {
		m_gridpoints[n][m]=f9*(m_gridpoints[n][k]+m_gridpoints[n][l])-f*(m_gridpoints[n][kk]+m_gridpoints[n][ll]);
	    }
	    else {
		m_gridpoints[n][m]=(m_gridpoints[n][k]+m_gridpoints[n][l])*0.5;
	    }
	  }
	}
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<float>(avg_cnt);
  }
  grid.filled=true;
  return *this;
}

void SLPGrid::print(std::ostream& outs) const
{
  size_t n,l;
  int m;
  size_t start,stop;

  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (grid.filled) {
    outs << "i = the data point was interpolated from surrounding data" << std::endl;
    for (n=0; n < 72; n+=18) {
	start=n;
	stop=n+18;
	outs << "\n    \\ LON";
	outs << std::setw(4) << start*5 << "E";
	for (l=start+1; l < stop; l++)
	  outs << std::setw(7) << l*5 << "E";
	outs << std::endl;
	outs << " LAT +------------------------------------------------------------------------------------------------------------------------------------------------" << std::endl;
	for (m=14; m >= 0; m--) {
	  outs << std::setw(3) << (m*5)+15 << "N |";
	  for (l=start; l < stop; ++l) {
	    if (floatutils::myequalf(m_gridpoints[m][l],Grid::MISSING_VALUE)) {
		outs << "        ";
	    }
	    else {
		if (imap[m][l] == 0) {
		  outs << std::setw(8) << m_gridpoints[m][l];
		}
		else {
		  outs << " i" << std::setw(6) << m_gridpoints[m][l];
		}
	    }
	  }
	  outs << std::endl;
	}
    }
    outs << std::endl;
  }
}

void SLPGrid::print(std::ostream& outs,float bottom_latitude,float top_latitude,float west_longitude,float east_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
  int bottom_lat,top_lat;
  int west_lon,east_lon;
  int n,m,l;
  int nn=1,mm=1,ll;
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
		if (floatutils::myequalf(grid.pole,Grid::MISSING_VALUE)) {
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
		if (floatutils::myequalf(m_gridpoints[m][ll],Grid::MISSING_VALUE)) {
		  outs << "        ";
		}
		else {
		  outs << std::setw(8) << m_gridpoints[m][ll];
		}
	    }
	  }
	  outs << std::endl;
	}
    }
    outs << std::endl;
  }
}

void SLPGrid::print_ascii(std::ostream& outs) const
{
}

void SLPGrid::print_csv(std::ostream& outs) const
{
  size_t n;
  int m;
  size_t dimy=dim.y-1;

  outs.setf(std::ios::fixed);
  outs.precision(1);
  outs << m_reference_date_time.to_string("%Y%m%d") << "," << m_reference_date_time.to_string("%H%MM") << "," << grid.level1 << "," << grid.src << ",90";
  if (!floatutils::myequalf(grid.pole,Grid::MISSING_VALUE)) {
    for (m=0; m < dim.x; ++m) {
	outs << "," << grid.pole;
    }
    outs << std::endl;
  }
  else {
    for (m=0; m < dim.x; ++m) {
	outs << ",M";
    }
    outs << std::endl;
  }
  for (n=dimy; n > 0; n--) {
    outs << m_reference_date_time.to_string("%Y%m%d") << "," << m_reference_date_time.to_string("%H%MM") << "," << grid.level1 << "," << grid.src << "," << 85-(15-n)*5;
    for (m=0; m < dim.x; ++m) {
	outs << ",";
	if (!floatutils::myequalf(m_gridpoints[n-1][m],Grid::MISSING_VALUE)) {
	  if (imap[n-1][m] > 0) {
	    outs << "i";
	  }
	  outs << m_gridpoints[n-1][m];
	}
	else {
	  outs << "M";
	}
    }
    outs << std::endl;
  }
}

void SLPGrid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
  outs.setf(std::ios::fixed);
  outs.precision(1);

  if (verbose) {
    if (m_reference_date_time.day() > 0) {
	outs << " DAILY GRID -- Date: " << m_reference_date_time.to_string("%Y-%m-%d %H %Z") << std::endl;
	outs << "   Format: NCAR Sea-Level Pressure  Level: " << std::setw(4) << grid.level1 << "mb  Parameter: Sea-Level Pressure  Source: " << std::setw(2) << grid.src << "  Pole: ";
    }
    else {
	outs << " MONTHLY GRID -- Date: " << m_reference_date_time.to_string("%Y-%m-%d %H %Z") << "  Grids in Average: " << std::setw(3) << grid.nmean << std::endl;
	outs << "   Format: NCAR Sea-Level Pressure  Level: " << std::setw(4) << grid.level1 << "mb  Parameter: Sea-Level Pressure  Source: " << std::setw(2) << grid.src << "  Pole: ";
    }
    if (floatutils::myequalf(grid.pole,Grid::MISSING_VALUE)) {
	outs << "    N/A" << std::endl;
    }
    else {
	outs << std::setw(7) << grid.pole << "mb" << std::endl;
    }
  }
  else {
    outs << " Type=" << grid.grid_type << " Time=" << m_reference_date_time.to_string("%Y%m%d%H") << " NAvg=" << grid.nmean << " Src=" << grid.src << " Param=" << grid.param << " Level=" << grid.level1 << " Pole=";
    if (!floatutils::myequalf(grid.pole,Grid::MISSING_VALUE)) {
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
