#include <iomanip>
#include <grid.hpp>
#include <utils.hpp>
#include <bits.hpp>

int InputLatLonGridStream::peek()
{
  size_t rec_len;
  unsigned char *buffer;
  int bytes_read;

  if (icosstream != NULL) {
    return icosstream->peek();
  }
  else {
    if (floatutils::myequalf(res,2.5)) {
	rec_len=10024;
    }
    else if (floatutils::myequalf(res,5.)) {
	rec_len=2600;
    }
    else {
	std::cerr << "Error: invalid resolution " << res << " specified" << std::endl;
	exit(1);
    }
    buffer=new unsigned char[rec_len];
    bytes_read=read(buffer,rec_len);
    if (bytes_read != bfstream::error && bytes_read != bfstream::eof) {
	fs.seekg(-bytes_read,std::ios_base::cur);
	--num_read;
    }
    return bytes_read;
  }
}

int InputLatLonGridStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read;
  size_t rec_len;

  if (floatutils::myequalf(res,2.5)) {
    rec_len=10024;
  }
  else if (floatutils::myequalf(res,5.)) {
    rec_len=2600;
  }
  else {
    std::cerr << "Error: invalid resolution " << res << " specified" << std::endl;
    exit(1);
  }

  if (buffer_length < rec_len) {
    std::cerr << "Error: buffer overflow\n" << std::endl;
    exit(1);
  }

// read a grid from the stream
  if (icosstream != NULL) {
    if ( (bytes_read=icosstream->read(buffer,buffer_length)) <= 0)
	return bytes_read;
  }
  else {
    fs.read(reinterpret_cast<char *>(buffer),rec_len);
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (!fs.good()) {
	return bfstream::error;
    }
    bytes_read=fs.gcount();
    if (static_cast<size_t>(bytes_read) < rec_len) {
	bytes_read=bfstream::error;
    }
  }
  ++num_read;
  return bytes_read;
}

LatLonGrid::LatLonGrid(float resolution)
{
  def.type=Grid::latitudeLongitudeType;
  def.laincrement=def.loincrement=resolution;
  def.slatitude=0.;
  def.elatitude=90.;
  def.slongitude=0.;
  def.elongitude=360.-def.loincrement;
  dim.x=(def.elongitude-def.slongitude)/def.loincrement+1;
  dim.y=(def.elatitude-def.slatitude)/def.loincrement+1;
  dim.size=dim.x*dim.y;
}

LatLonGrid::LatLonGrid(const LatLonGrid& source)
{
  *this=source;
}

LatLonGrid& LatLonGrid::operator=(const LatLonGrid& source)
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
    if (gridpoints_ == NULL) {
	gridpoints_=new double *[dim.y];
	for (n=0; n < dim.y; n++)
	  gridpoints_[n]=new double[dim.x];
    }
    for (n=0; n < dim.y; ++n) {
	for (m=0; m < dim.x; ++m) {
	  gridpoints_[n][m]=source.gridpoints_[n][m];
	}
    }
    if (source.num_in_sum == NULL) {
	if (num_in_sum != NULL) {
	  for (n=0; n < dim.y; ++n) {
	    delete[] num_in_sum[n];
	  }
	  delete[] num_in_sum;
	  num_in_sum=NULL;
	}
    }
    else {
	if (num_in_sum == NULL) {
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

void LatLonGrid::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  int n,m,l,avg_cnt=0;
  long long sum;
  int bias,scale,dum,*pval,off,num_words=(180+dim.size*15+59)/60;
  short yr,mo,dy,time;

  if (stream_buffer == NULL) {
    std::cerr << "Error: empty file stream\n" << std::endl;
    exit(1);
  }

// check the checksum for the grid
  if (checksum(stream_buffer,num_words+1,60,sum) != 0)
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
  bits::get(stream_buffer,grid.param,37,9);
  bits::get(stream_buffer,grid.fcst_time,46,9);
  grid.fcst_time*=10000;
  if (grid.fcst_time > 0) {
    valid_date_time_=reference_date_time_.time_added(grid.fcst_time);
  }
  else {
    valid_date_time_=reference_date_time_;
  }
  bits::get(stream_buffer,dum,55,10);
  grid.level2=1023.-dum;
  bits::get(stream_buffer,grid.src,65,6);
  if (dy == 0) {
    bits::get(stream_buffer,grid.nmean,106,8);
  }
  else {
    grid.nmean=0;
  }
  grid.filled=false;
  bits::get(stream_buffer,bias,76,15);
  bits::get(stream_buffer,scale,91,15);
  scale-=bias;
  auto base=floatutils::cdcconv(stream_buffer,120,1);
// unpack the octagonal grid gridpoints
  if (!fill_header_only) {
// if memory has not yet been allocated for the gridpoints, do it now
    if (gridpoints_ == NULL) {
	gridpoints_=new double *[dim.y];
	for (n=0; n < dim.y; n++)
	  gridpoints_[n]=new double[dim.x];
    }
    pval=new int[dim.size];
    bits::get(stream_buffer,pval,180,15,0,dim.size);
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
    grid.pole=gridpoints_[dim.y-1][int(280./def.loincrement)];
    if (avg_cnt > 0) {
	stats.avg_val/=static_cast<float>(avg_cnt);
    }
    delete[] pval;
  }
  else {
    off=((dim.y-1)*dim.x)+(280./def.loincrement);
    off*=15;
    bits::get(stream_buffer,dum,180+off,15);
    if (dum == 0) {
	grid.pole=Grid::missing_value;
    }
    else {
	grid.pole=base+(dum-bias)*pow(2.,scale);
    }
  }
}

void LatLonGrid::operator+=(const LatLonGrid& source)
{
  int n,m;

  if (gridpoints_ == NULL) {
    *this=source;
    if (num_in_sum == NULL) {
	num_in_sum=new size_t *[dim.y];
	for (n=0; n < dim.y; ++n) {
	  num_in_sum[n]=new size_t[dim.x];
	}
    }
    for (n=0; n < dim.y; n++) {
	for (m=0; m < dim.x; m++) {
	  if (!floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	    num_in_sum[n][m]=1;
	  }
	  else {
	    num_in_sum[n][m]=0;
	  }
	}
    }
    grid.num_in_pole_sum=0;
    reference_date_time_.set_day(0);
    grid.nmean=1;
  }
  else {
    if (reference_date_time_.time() != 310000) {
	if (reference_date_time_.time() != source.reference_date_time_.time())
	  reference_date_time_.set_time(310000);
    }
    if (!floatutils::myequalf(source.grid.pole,Grid::missing_value)) {
	grid.pole+=source.grid.pole;
	grid.num_in_pole_sum++;
    }
    stats.max_val=-Grid::missing_value;
    stats.min_val=Grid::missing_value;
    for (n=0; n < dim.y; ++n) {
	for (m=0; m < dim.x; ++m) {
	  if (!floatutils::myequalf(source.gridpoints_[n][m],Grid::missing_value)) {
	    gridpoints_[n][m]+=source.gridpoints_[n][m];
	    ++num_in_sum[n][m];
	    if (gridpoints_[n][m] > stats.max_val) {
		stats.max_val=gridpoints_[n][m];
		stats.max_i=m+1;
		stats.max_j=n+1;
	    }
	    else if (gridpoints_[n][m] < stats.min_val) {
		stats.min_val=gridpoints_[n][m];
		stats.min_i=m+1;
		stats.min_j=n+1;
	    }
	  }
	}
    }
    grid.nmean++;
  }
}

LatLonGrid& LatLonGrid::operator=(const GRIBGrid& source)
{
  int n,m,l,latinc,loninc;

  grid.filled=false;
  if (floatutils::myequalf(source.definition().laincrement,0.) || source.definition().type == Grid::gaussianLatitudeLongitudeType) {
    return *this;
  }
  switch (source.first_level_type()) {
    case 1:
    case 100:
    case 101:
    case 102:
    case 105:
	break;
    default:
	return *this;
  }
  if (gridpoints_ == NULL) {
    gridpoints_=new double *[dim.y];
    for (n=0; n < dim.y; ++n) {
	gridpoints_[n]=new double[dim.x];
    }
  }
  switch (source.parameter()) {
    case 2:
    {
	if (source.first_level_type() == 102) {
	  grid.param=6;
	  grid.level1=1013.;
	}
	else {
	  return *this;
	}
	break;
    }
    case 7:
    {
	grid.param=1;
	if (source.first_level_type() == 1) {
	  grid.level1=1001.;
	}
	else {
	  grid.level1=source.first_level_value();
	}
	break;
    }
    case 11:
    {
	if (source.first_level_type() == 1) {
	  grid.param=57;
	  grid.level1=1001.;
	}
	else if (source.first_level_type() == 105 && floatutils::myequalf(source.first_level_value(),2.)) {
	  grid.param=10;
	  grid.level1=1001.;
	}
	else {
	  grid.param=10;
	  grid.level1=source.first_level_value();
	}
	break;
    }
    case 33:
    {
	grid.param=30;
	if (source.first_level_type() == 105) {
	  grid.level1=1001.;
	}
	else {
	  grid.level1=source.first_level_value();
	}
	break;
    }
    case 34:
    {
	grid.param=31;
	if (source.first_level_type() == 105) {
	  grid.level1=1001.;
	}
	else {
	  grid.level1=source.first_level_value();
	}
	break;
    }
    case 39:
    {
	grid.param=5;
	grid.level1=source.first_level_value();
	break;
    }
    case 52:
    {
	grid.param=44;
	if (source.first_level_type() == 105) {
	  grid.level1=1001.;
	}
	else {
	  grid.level1=source.first_level_value();
	}
	break;
    }
    case 80:
    {
	grid.param=57;
	if (source.first_level_type() == 1) {
	  grid.level1=1001.;
	}
	else {
	  grid.level1=source.first_level_value();
	}
	break;
    }
    default:
    {
	return *this;
    }
  }
  stats.max_val=-Grid::missing_value;
  stats.min_val=Grid::missing_value;
  latinc=lround(5./source.definition().laincrement);
  loninc=lround(5./source.definition().loincrement);
  switch (source.source()) {
    case 7:
    {
	switch (source.scan_mode()) {
	  case 0x0:
	  {
	    grid.pole=source.gridpoint(0,0);
	    for (n=18,l=0; n >= 0; n--,l++) {
		for (m=0; m < 72; m++) {
		  gridpoints_[n][m]=source.gridpoint(m*loninc,l*latinc);
		  if (!floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
		    if (grid.param == 5 || grid.param == 6) {
			gridpoints_[n][m]*=0.01;
		    }
		    else if (grid.param == 10 || grid.param == 11 || grid.param == 57) {
			gridpoints_[n][m]-=273.15;
		    }
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
		  }
		}
	    }
	    break;
	  }
	  case 0x40:
	  {
	    grid.pole=source.gridpoint(144,36);
	    for (n=0; n < 19; n++) {
		for (m=0; m < 72; m++) {
		  gridpoints_[n][m]=source.gridpoint(m*loninc,n*latinc);
		  if (!floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
		    if (grid.param == 5 || grid.param == 6) {
			gridpoints_[n][m]*=0.01;
		    }
		    else if (grid.param == 10 || grid.param == 11 || grid.param == 57) {
			gridpoints_[n][m]-=273.15;
		    }
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
		  }
		}
	    }
	    break;
	  }
	  default:
	  {
	    return *this;
	  }
	}
	grid.filled=true;
	grid.src=1;
	break;
    }
    default:
    {
	return *this;
    }
  }
  grid.grid_type=1;
  reference_date_time_=source.reference_date_time();
  valid_date_time_=source.valid_date_time();
  grid.fcst_time=source.forecast_time();
  grid.level2=source.second_level_value();
  grid.nmean=source.number_averaged();
  return *this;
}

LatLonGrid& LatLonGrid::operator=(const OctagonalGrid& source)
{
  const float deg_rad=3.141592654/180.,pi_4=3.141592654*0.25;
  int n,m,x,y,avg_cnt=0;
  double lat,r,lon,dx,dy;
  double dxx,dyy,a,b,c,d;
  double **ogp=NULL;
  GridDimensions gdim;

  grid.grid_type=source.type();
  reference_date_time_=source.reference_date_time();
  valid_date_time_=source.valid_date_time();
  grid.level1=source.first_level_value();
  grid.param=source.parameter();
  if (grid.param == 28) {
    grid.param=6;
  }
  else if (grid.param == 47) {
    grid.param=57;
  }
  grid.fcst_time=source.forecast_time();
  grid.level2=source.second_level_value();
  grid.src=source.source();
  grid.nmean=source.number_averaged();
  gdim=source.dimensions();
  if (gridpoints_ == NULL) {
    gridpoints_=new double *[dim.y];
    for (n=0; n < dim.y; ++n) {
	gridpoints_[n]=new double[dim.x];
    }
  }
  for (n=0; n < int(20./def.laincrement); n++) {
    for (m=0; m < dim.x; ++m) {
	gridpoints_[n][m]=Grid::missing_value;
    }
  }
  if (source.number_missing() > 0) {
    ogp=new double *[gdim.y];
    for (n=0; n < gdim.y; ++n) {
	ogp[n]=new double[gdim.x];
	for (m=0; m < gdim.x; ++m) {
	  ogp[n][m]=source.gridpoint(m,n);
	  if (floatutils::myequalf(ogp[n][m],Grid::missing_value)) {
	    ogp[n][m]=0.;
	  }
	}
    }
  }
  stats.max_val=-Grid::missing_value;
  stats.min_val=Grid::missing_value;
  stats.avg_val=0.;
  for (n=int(20./def.laincrement); n < dim.y; n++) {
    lat=deg_rad*(n*def.laincrement);
    r=31.204359052*tan(pi_4-(lat*0.5));
    for (m=0; m < dim.x; m++) {
	lon=deg_rad*(m*def.loincrement-10.);
	dx=r*cos(lon)+23.;
	dy=r*sin(lon)+25.;
	x=dx;
	y=dy;
	dx-=x;
	dy-=y;
// 16-point bessel interpolation
	dxx=(dx-1.)*0.25;
	dyy=(dy-1.)*0.25;
	if (ogp != NULL) {
	  a=ogp[y-1][x]+dx*(ogp[y-1][x+1]-ogp[y-1][x]+dxx*(ogp[y-1][x+2]-
          ogp[y-1][x+1]+ogp[y-1][x-1]-ogp[y-1][x]));
	  b=ogp[y][x]+dx*(ogp[y][x+1]-ogp[y][x]+dxx*(ogp[y][x+2]-ogp[y][x+1]+
          ogp[y][x-1]-ogp[y][x]));
	  c=ogp[y+1][x]+dx*(ogp[y+1][x+1]-ogp[y+1][x]+dxx*(ogp[y+1][x+2]-
          ogp[y+1][x+1]+ogp[y+1][x-1]-ogp[y+1][x]));
	  d=ogp[y+2][x]+dx*(ogp[y+2][x+1]-ogp[y+2][x]+dxx*(ogp[y+2][x+2]-
          ogp[y+2][x+1]+ogp[y+2][x-1]-ogp[y+2][x]));
	}
	else {
	  a=source.gridpoint(x,y-1)+dx*(source.gridpoint(x+1,y-1)-
          source.gridpoint(x,y-1)+dxx*(source.gridpoint(x+2,y-1)-
          source.gridpoint(x+1,y-1)+source.gridpoint(x-1,y-1)-
          source.gridpoint(x,y-1)));
	  b=source.gridpoint(x,y)+dx*(source.gridpoint(x+1,y)-
          source.gridpoint(x,y)+dxx*(source.gridpoint(x+2,y)-
          source.gridpoint(x+1,y)+source.gridpoint(x-1,y)-
          source.gridpoint(x,y)));
	  c=source.gridpoint(x,y+1)+dx*(source.gridpoint(x+1,y+1)-
          source.gridpoint(x,y+1)+dxx*(source.gridpoint(x+2,y+1)-
          source.gridpoint(x+1,y+1)+source.gridpoint(x-1,y+1)-
          source.gridpoint(x,y+1)));
	  d=source.gridpoint(x,y+2)+dx*(source.gridpoint(x+1,y+2)-
          source.gridpoint(x,y+2)+dxx*(source.gridpoint(x+2,y+2)-
          source.gridpoint(x+1,y+2)+source.gridpoint(x-1,y+2)-
          source.gridpoint(x,y+2)));
	}
	gridpoints_[n][m]=b+dy*(c-b+dyy*(d-c+a-b));
	if (grid.param == 44) {
	  if (gridpoints_[n][m] > 100.)
	    gridpoints_[n][m]=100.;
	  else if (gridpoints_[n][m] < 0.)
	    gridpoints_[n][m]=0.;
	}
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
  if (avg_cnt > 0)
    stats.avg_val/=static_cast<float>(avg_cnt);
  grid.filled=true;
  if (ogp != NULL) {
    for (n=0; n < gdim.y; ++n) {
	delete[] ogp[n];
    }
    delete[] ogp;
  }
  return *this;
}

LatLonGrid& LatLonGrid::operator=(const ON84Grid& source)
{
  int n,m,nn,mm,avg_cnt=0;

  if (source.type() != 29) {
    std::cerr << "Error: not a NH grid or lat/lon" << std::endl;
    exit(1);
  }

  grid.grid_type=1;
  reference_date_time_=source.reference_date_time();
  valid_date_time_=source.valid_date_time();
  grid.level1=source.first_level_value();
  grid.level2=source.second_level_value();
  switch (source.parameter()) {
    case 1:
    {
	grid.param=1;
	break;
    }
    case 8:
    {
	if (floatutils::myequalf(grid.level1,0.)) {
	  grid.param=6;
	  if (source.first_level_type() == 128) {
	    grid.level1=1013.;
	  }
	  else if (source.first_level_type() == 129) {
	    grid.level1=1001.;
	  }
	  else {
	    std::cerr << "Error: unknown pressure" << std::endl;
	    exit(1);
	  }
	}
	else {
	  std::cerr << "Error: unknown pressure" << std::endl;
	  exit(1);
	}
	break;
    }
    case 16:
    {
	grid.param=10;
	if (floatutils::myequalf(grid.level1,0.)) {
	  if (source.first_level_type() == 129) {
	    grid.level1=1001.;
	  }
	  else if (source.first_level_type() == 130) {
	    grid.level1=980.;
	  }
	  else if (source.first_level_type() == 145) {
	    grid.level1=1013.;
	    grid.level2=980.;
	  }
	  else {
	    std::cerr << "Error: unknown temperature level " << source.first_level_type() << std::endl;
	    exit(1);
	  }
	}
	else {
	  if (floatutils::myequalf(grid.level1,1001.) || floatutils::myequalf(grid.level1,850.) || floatutils::myequalf(grid.level1,700.) || floatutils::myequalf(grid.level1,500.) || floatutils::myequalf(grid.level1,400.) || floatutils::myequalf(grid.level1,300.))
	    grid.param=11;
	}
	break;
    }
    case 48:
    {
	grid.param=30;
	break;
    }
    case 49:
    {
	grid.param=31;
	break;
    }
    case 384:
    {
	grid.param=57;
	grid.level1=1001.;
	break;
    }
  }
  grid.fcst_time=source.forecast_time();
  grid.src=1;
  grid.nmean=source.number_averaged();
  if (gridpoints_ == NULL) {
    gridpoints_=new double *[dim.y];
    for (n=0; n < dim.y; ++n) {
	gridpoints_[n]=new double[dim.x];
    }
  }
  stats.max_val=-Grid::missing_value;
  stats.min_val=Grid::missing_value;
  stats.avg_val=0.;
  for (nn=0,n=0; nn < 37; ++nn) {
    if ( (nn % 2) == 0) {
	for (mm=0,m=0; mm < 143; ++mm) {
	  if ( (mm % 2) == 0) {
	    gridpoints_[n][m]=source.gridpoint(mm,nn);
	    if (!floatutils::myequalf(gridpoints_[n][m],Grid::missing_value) && (grid.param == 10 || grid.param == 57)) {
		gridpoints_[n][m]-=273.15;
	    }
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
	    ++m;
	  }
	}
	++n;
    }
  }
  if (avg_cnt > 0) {
    stats.avg_val/=static_cast<float>(avg_cnt);
  }
  grid.filled=true;
  return *this;
}

void LatLonGrid::print(std::ostream& outs) const
{
  int n,m,l;
  float start,stop,mm;

  if (!floatutils::myequalf(stats.avg_val,0.) && fabs(stats.avg_val) < 0.01 && fabs(stats.min_val) < 0.01 && fabs(stats.max_val) < 0.01) {
    outs.setf(std::ios::scientific);
  }
  else {
    outs.setf(std::ios::fixed);
  }
  outs.precision(1);
  if (grid.filled) {
    outs << std::endl;
    for (l=0; l < dim.x; l+=18) {
	start=def.slongitude+l*def.loincrement;
	stop=start+17*def.loincrement;
	if (stop > def.elongitude) stop=def.elongitude;
	outs << "     \\ LON";
	for (mm=start; mm <= stop; mm+=def.loincrement) {
	  outs << std::setw(7) << mm << "E";
	}
	outs << std::endl;
	outs << "  LAT +---";
	for (mm=start; mm <= stop; mm+=def.loincrement) {
	  outs << "--------";
	}
	outs << std::endl;
	for (n=dim.y-1; n >= 0; --n) {
	  outs << std::setw(4) << n*def.laincrement << "N |   ";
	  for (m=l; m < l+18; ++m) {
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

void LatLonGrid::print(std::ostream& outs,float bottom_latitude,float top_latitude,float west_longitude,float east_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
}

void LatLonGrid::print_ascii(std::ostream& outs) const
{
  int n,m;
  size_t precision,width;

  if (!grid.filled) {
    return;
  }
  outs.setf(std::ios::fixed);
  precision= (floatutils::precision(stats.min_val) > floatutils::precision(stats.avg_val)) ? floatutils::precision(stats.min_val) : floatutils::precision(stats.avg_val);
  precision= (precision > floatutils::precision(stats.max_val)) ? precision : floatutils::precision(stats.max_val);
  outs.precision(precision);
  width=7+precision;
  outs << reference_date_time_.to_string("%Y%m%d%H") << " " << std::setw(2) << grid.param << " " << std::setw(6) << grid.level1 << " " << std::setw(6) << grid.level2 << " " << std::setw(3) << dim.x << " " << std::setw(3) << dim.y << " " << std::setw(3) << grid.nmean << " " << std::setw(2) << width << " " << std::setw(2) << precision << std::endl;
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
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

void LatLonGrid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
  bool scientific=false;

  outs.setf(std::ios::fixed);
  outs.precision(1);
  if (!floatutils::myequalf(stats.avg_val,0.) && fabs(stats.avg_val) < 0.01) {
    scientific=true;
  }
  if (verbose) {
    if (reference_date_time_.day() > 0) {
	outs << " DAILY GRID -- Date: " << reference_date_time_.to_string() << "  Valid: " << valid_date_time_.to_string() << std::endl;
	outs << "   Format: NCAR 5-degree Latitude/Longitude  Level: " << std::setw(6) << grid.level1 << "mb  Parameter Code: " << std::setw(2) << grid.param << "  Source: " << std::setw(2) << grid.src << "  Pole: ";
    }
    else {
	outs << " MONTHLY GRID -- Date: " << reference_date_time_.to_string() << "  Grids in Average: " << std::setw(3) << grid.nmean << std::endl;
	outs << "   Format: NCAR 5-degree Latitude/Longitude  Level: " << std::setw(6) << grid.level1 << "mb  Parameter Code: " << std::setw(2) << grid.param << "  Source: " << grid.src << "  Pole: ";
    }
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
    outs << " Type=" << grid.grid_type << " Date=" << reference_date_time_.to_string("%Y%m%d%H") << " Valid=" << valid_date_time_.to_string("%Y%m%d%H") << " NAvg=" << grid.nmean << " Src=" << grid.src << " Param=" << grid.param;
    if (floatutils::myequalf(grid.level2,0.))
	outs << " Level=" << grid.level1 << " Pole=";
    else
	outs << " Levels=" << grid.level1 << "," << grid.level2 << " Pole=";
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

size_t LatLonGrid::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
  static const size_t bias=16384;
  int n,m;
  size_t cnt=0;
  long long sum;
  int scale,*packed,num_words=(180+dim.size*15+59)/60,dum;
  double dscale,dbase;
  short hr;

  if (!grid.filled) {
    return 0;
  }
  if (floatutils::myequalf(stats.max_val,-Grid::missing_value) || floatutils::myequalf(stats.min_val,Grid::missing_value)) {
    return 0;
  }
  bits::set(output_buffer,grid.grid_type,0,6);
  bits::set(output_buffer,reference_date_time_.year()-1900,6,7);
  bits::set(output_buffer,reference_date_time_.month(),13,4);
  bits::set(output_buffer,reference_date_time_.day(),17,5);
  hr=reference_date_time_.time()/10000;
  if ( (reference_date_time_.time() % 10000) >= 3000)
    hr++;
  bits::set(output_buffer,hr,22,5);
  dum=1023.-grid.level1;
  bits::set(output_buffer,dum,27,10);
  bits::set(output_buffer,grid.param,37,9);
  bits::set(output_buffer,grid.fcst_time/10000,46,9);
  dum=1023.-grid.level2;
  bits::set(output_buffer,dum,55,10);
  bits::set(output_buffer,grid.src,65,6);
  bits::set(output_buffer,0,71,5);
  bits::set(output_buffer,bias,76,15);
  bits::set(output_buffer,grid.nmean,106,8);
  bits::set(output_buffer,0,114,6);
// do gridpoint scaling
  dscale=log((stats.max_val-stats.min_val)/static_cast<float>(0x7fff))/log(2.);
  scale=dscale;
  if (dscale >= 0.) {
    ++scale;
  }
  bits::set(output_buffer,scale+bias,91,15);
  dbase=(stats.max_val+stats.min_val)*0.5;
  auto base=floatutils::cdcconv(dbase,1);
  bits::set(output_buffer,base,120,60);
// pack the gridpoints
  packed=new int[dim.size];
  for (n=0; n < dim.y; ++n) {
    for (m=0; m < dim.x; ++m) {
	if (!floatutils::myequalf(gridpoints_[n][m],Grid::missing_value)) {
	  packed[cnt++]=lround((gridpoints_[n][m]-dbase)*pow(2.,-scale))+bias;
	}
	else {
	  packed[cnt++]=0;
	}
    }
  }
  if (cnt != dim.size) {
    std::cerr << "Error: attempting to pack wrong number of datapoints" << std::endl;
    exit(1);
  }
  bits::set(output_buffer,packed,180,15,0,dim.size);
  delete[] packed;
// pack the checksum
  checksum(output_buffer,num_words+1,60,sum);
  bits::set(output_buffer,sum,num_words*60,60);
  num_words=((num_words+1)*60+63)/64;
  return num_words*8;
}

void convert_ij_wind_to_xy(LatLonGrid& ucomp,LatLonGrid& vcomp)
{
  size_t n,m,cnt=0;
  double deg_rad=3.141592654/180.,ang,temp;

  if (ucomp.dim.x != 72 || ucomp.dim.y != 19 || vcomp.dim.x != 72 || vcomp.dim.y != 19 || !ucomp.grid.filled || !vcomp.grid.filled) {
    return;
  }
  ucomp.stats.max_val=vcomp.stats.max_val=-Grid::missing_value;
  ucomp.stats.min_val=vcomp.stats.min_val=Grid::missing_value;
  ucomp.stats.avg_val=vcomp.stats.avg_val=0.;
  for (m=0; m < 72; ++m) {
    ang=deg_rad*(m*5.-10.);
    for (n=0; n < 19; ++n) {
	if (!floatutils::myequalf(ucomp.gridpoints_[n][m],ucomp.Grid::missing_value) && !floatutils::myequalf(vcomp.gridpoints_[n][m],vcomp.Grid::missing_value)) {
	  temp=vcomp.gridpoints_[n][m];
	  vcomp.gridpoints_[n][m]=-temp*sin(ang)-ucomp.gridpoints_[n][m]*cos(ang);
	  if (vcomp.gridpoints_[n][m] > vcomp.stats.max_val) {
	    vcomp.stats.max_val=vcomp.gridpoints_[n][m];
	  }
	  if (vcomp.gridpoints_[n][m] < vcomp.stats.min_val) {
	    vcomp.stats.min_val=vcomp.gridpoints_[n][m];
	  }
	  vcomp.stats.avg_val+=vcomp.gridpoints_[n][m];
	  ucomp.gridpoints_[n][m]=temp*cos(ang)-ucomp.gridpoints_[n][m]*sin(ang);
	  if (ucomp.gridpoints_[n][m] > ucomp.stats.max_val) {
	    ucomp.stats.max_val=ucomp.gridpoints_[n][m];
	  }
	  if (ucomp.gridpoints_[n][m] < ucomp.stats.min_val) {
	    ucomp.stats.min_val=ucomp.gridpoints_[n][m];
	  }
	  ucomp.stats.avg_val+=ucomp.gridpoints_[n][m];
	  ++cnt;
	}
    }
  }
  if (cnt > 0) {
    ucomp.stats.avg_val/=static_cast<float>(cnt);
    vcomp.stats.avg_val/=static_cast<float>(cnt);
  }
}

void convert_xy_wind_to_ij(LatLonGrid& ucomp,LatLonGrid& vcomp)
{
  size_t n,m,cnt=0;
  double deg_rad=3.141592654/180.,ang,temp;

  if (ucomp.dim.x != 72 || ucomp.dim.y != 19 || vcomp.dim.x != 72 || vcomp.dim.y != 19 || !ucomp.grid.filled || !vcomp.grid.filled) {
    return;
  }
  ucomp.stats.max_val=vcomp.stats.max_val=-Grid::missing_value;
  ucomp.stats.min_val=vcomp.stats.min_val=Grid::missing_value;
  ucomp.stats.avg_val=vcomp.stats.avg_val=0.;
  for (m=0; m < 72; ++m) {
    ang=deg_rad*(m*5.-10.);
    for (n=0; n < 19; ++n) {
	if (!floatutils::myequalf(ucomp.gridpoints_[n][m],ucomp.Grid::missing_value) && !floatutils::myequalf(vcomp.gridpoints_[n][m],vcomp.Grid::missing_value)) {
	  temp=vcomp.gridpoints_[n][m];
	  vcomp.gridpoints_[n][m]=(-temp*sin(ang)+ucomp.gridpoints_[n][m]*cos(ang))/
          (cos(ang)*cos(ang)+sin(ang)*sin(ang));
	  if (vcomp.gridpoints_[n][m] > vcomp.stats.max_val) {
	    vcomp.stats.max_val=vcomp.gridpoints_[n][m];
	  }
	  if (vcomp.gridpoints_[n][m] < vcomp.stats.min_val) {
	    vcomp.stats.min_val=vcomp.gridpoints_[n][m];
	  }
	  vcomp.stats.avg_val+=vcomp.gridpoints_[n][m];
	  ucomp.gridpoints_[n][m]=(-temp*cos(ang)-ucomp.gridpoints_[n][m]*sin(ang))/
          (cos(ang)*cos(ang)+sin(ang)*sin(ang));
	  if (ucomp.gridpoints_[n][m] > ucomp.stats.max_val) {
	    ucomp.stats.max_val=ucomp.gridpoints_[n][m];
	  }
	  if (ucomp.gridpoints_[n][m] < ucomp.stats.min_val) {
	    ucomp.stats.min_val=ucomp.gridpoints_[n][m];
	  }
	  ucomp.stats.avg_val+=ucomp.gridpoints_[n][m];
	  ++cnt;
	}
    }
  }
  if (cnt > 0) {
    ucomp.stats.avg_val/=static_cast<float>(cnt);
    vcomp.stats.avg_val/=static_cast<float>(cnt);
  }
}
