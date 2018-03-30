// FILE: on84grid.cpp

#include <iomanip>
#include <grid.hpp>
#include <utils.hpp>
#include <bits.hpp>

const char *ON84Grid::parameter_short_name[ON84Grid::PARAMETER_SIZE]={
"","-HGT--","-P-ALT","","","",
"-DIST-","-DEPTH","-PRES-","-PTEND","","","","","","","-TMP--","-DPT--",
"-DEPR-","-POT--","-T-MAX","-T-MIN","-TSOIL","","","","","","","","","","","",
"","","","","","","-V-VEL","-NETVD","-DZDT-","-OROW-","-FRCVV","","","",
"-U-GRD","-V-GRD","-WIND-","-T-WND","-VW-SH","-U-DIV","-V-DIV","-WDIR-",
"-WWND-","-SWND-","-RATS-","-VECW-","-SFAC-","-GUST-","D-DUDT","D-DVDT","","",
"","","", "","","","-ABS-V","-REL-V","-DIV--","","","","","","-STRM-","-V-POT",
"-U-STR","-V-STR","-TUVRD","-TVVRD","XGWSTR","YGWSTR","-R-H--","-P-WAT",
"-A-PCP","-P-O-P","-P-O-Z","-SNO-D","-ACPCP","-SPF-H","-L-H20","-RRATE",
"-TSTM-","-CSVR-","-CTDR-","-MIXR-","-PSVR-","-MCONV","-VAPP-","-NCPCP",
"-ICEAC","-NPRAT","-CPRAT","-TQDEP","-TQSHL","-TQVDF","-LFT-X","-TOTOS",
"-K-X--","-C-INS","-4LFTX","-A-EVP","","","-L-WAV","-S-WAV","","","","","","",
"-MSL--","-SFC--","-TRO--","-MWSL-","-PLYR-","-A-LEV","-T-AIL","-B-AIL","","",
"","","","","","","-BDY--","-TRS--","-STS--","-QCP--","-SIG--","","","","","",
"","","","","","","-DRAG-","-LAND-","-KFACT","-10TSL","-7TSL-","-RCPOP",
"-RCMT-","-RCMP-","-ORTHP","-ALBDO","-ENFLX","-TTHTG","-ENRGY","-TOTHF",
"-SPEHF","-SORAD","-LAT--","-LON--","-RADIC","------","------","------",
"------","------","-PROB-","-CPROB","-USTAR","-TSTAR","-MIXHT","-MIXLY",
"-DLRFL","-ULRFL","-DSRFL","-USRFL","-UTHFL","-UTWFL","-TTLWR","-TTSWR",
"-TTRAD","-MSTAV","-RDNCE","-BRTMP","-TCOZ-","-OZMR-","-SWABS","-TTLRG",
"-TTSHL","-TTDEP","-TTVDF","-STCOF","-CDLYR","-CDCON","-PBCLY","-PTCLY",
"-PBCON","-PTCON","-SFEXC","-ZSTAR","-STDZG","","", "","","","","","","","",
"","","","","","","","","","","","","","","","","","","","","","","","","",
"","","","","","","","","","","","","","", "","","","","","","","", "","","","",
"","","","", "","","","","","","","", "","","","","","","","", "","","",
"-UOGRD","-VOGRD","","","", "","","","","","","","", "","","","","","","","","",
"","","","","","","", "","","","","","","","", "","","","","","","","", "","",
"","","","","","", "","","","","","","","", "","","","","","","","", "","","",
"","","","","", "","","","","-WTMP-","-WVHGT","-SWELL","-WVSWL","-WVPER",
"-WVDIR","-SWPER","-SWDIR","-ICWAT","","","","","","","","-HTSGW","-PERPW",
"-DIRPW","-PERSW","-DIRSW","-WCAPS","","","","","","U*","V*","H","LE","Ts","w","snow","LFDS","LFUS","OLR","OSL","SFUS","SFDS","CLFH","CLFM","CLFL","CHTB","CMTB","CLTB","TMPH","TMPM","TMPL","RAIN","CVPR","GFLX","U10","V10","T2","q2","Ps",
"","","","","","","","","","","","","","","","","","","","","","","RAIN","","",
"","","T2","q2",""};
const char *ON84Grid::parameter_description[ON84Grid::PARAMETER_SIZE]={
"","Geopotential",
"Pressure altitude","","","","Geometric distance above",
"Geometric distance below","Atmospheric pressure","Pressure tendency","","","",
"","","","Atmospheric temperature","Dewpoint temperature","Dewpoint depression",
"Potential temperature","Maximum temperature","Minimum temperature",
"Soil temperature","","","","","","","","","","","","","","","","","",
"Vertical velocity dp/dt","Net vertical displacement","Vertical velocity dz/dt",
"Orographic component dz/dt","Frictional component dz/dt","","","",
"U comp. of wind wrt grid","V comp. of wind wrt grid","Wind speed",
"Thermal wind speed","Vertical speed shear","Divergent u comp wrt grid",
"Divergent v comp wrt grid","Direction from which wind is blowing (wrt North)",
"Westerly comp. of wind","Southerly comp. of wind","Ratio of speeds",
"Vertical wind (spectral)","Steadiness factor","Wind gustiness",
"Diffusive u-comp. accel.","Diffusive v-comp.accel.","","","","","","","","",
"Absolute vorticity","Relative vorticity","Divergence","","","","","",
"Stream function","Velocity potential","Westerly comp. of wind stress",
"Southerly comp. of wind stress",
"Westerly wind comp. acceleration by vertical diffusion",
"Southerly wind comp. acceleration by vertical diffusion",
"x-component of gravity wave drag","y-component of gravity wave drag",
"Relative humidity","Precipitable water","Accumulated total precip",
"Probability of precipitation","Prob. of frozen precipitation","Snow depth",
"Accumulated convective precip","Specific humidity","Liquid water",
"Rainfall rate","Probability of thunderstorm",
"Conditional probability of severe local storm",
"Conditional probability of major tornado outbreak","Mixing ratio",
"Unconditional probability of severe local storm","Moisture convergence",
"Vapor pressure","Accumulated non-convective precipitation",
"Ice accretion rate","Non-convective precip rate",
"Convective precipitation rate","Deep conv. moisture tndcy.",
"Shallow conv. moisture tndcy.","Vertical diffusion moisture tendency",
"Lifted index","Totals index","K-index","Convective instability",
"4-layer lifted index","Accumulated evaporation","","",
"Long wave component of geopotential","Short wave component of geopotential",
"","","","","","","Mean sea level","Earth's surface (base of atmosphere)",
"Tropopause","Maximum wind speed level","Oceanographic primary layer",
"Anemometer level","Top of Aircraft Icing Layer",
"Bottom of Aircraft Icing Layer","","","","","","","","","Boundary",
"Troposphere","Stratosphere","Quiet cap","Entire atmosphere","","","","","","",
"","","","","","Drag coefficient","Land/sea flag","K factors",
"Conversion consts","Sea level pressure specification from 700mb heights",
"Regression coefficients for probability of precip.",
"Regression coefficients for mean temperature",
"Regression coefficients for mean precipitation","Orthogonal pressure function",
"Albedo","Energy flux","Temperature tendency from heating","Energy statistics",
"Total heat flux downward","Sensible + evaporative heat flux upward",
"Solar heat flux downward","Latitude","Longitude","Radar intensity",
"Ceiling height","Visibility","Liquid precip.","Freezing Precip.",
"Frozen precip.","Probability","Conditional probability",
"Surface friction velocity","Surface friction temperature","Mixing height",
"Number of mixed layers next to the surface",
"Downward flux of long-wave radiation","Upward flux of long-wave radiation",
"Downward flux of short-wave radiation","Upward flux of short-wave radiation",
"Upward turbulent flux of sensible heat","Upward turbulent flux of water",
"Temperature tendency from long-wave radiation",
"Temperature tendency from short-wave radiation",
"Temperature tendency from all radiation","Moisture availability","Radiance",
"Brightness temperature","Total column ozone","Ozone mixing ratio",
"Rate of absorption of short-wave radiation",
"Temperature tendency from large scale precipitation",
"Temperature tendency from shallow convection",
"Temperature tendency from deep convection",
"Temperature tendency from vertical diffusion","Soil thermal coefficient",
"Amount of non-convective cloud","Amount of convective cloud",
"Pressure at the base of a non-convective cloud",
"Pressure at the top of a non-convective cloud",
"Pressure at the base of a convective cloud",
"Pressure at the top of a convective cloud","Exchange coefficient at surface",
"Surface roughness length","Standard deviation of ground height","","","","","",
"","","","","","","","","","","","","","","","","","","","","","","","","","",
"","","","","","","","","","","","","","","","","","","","","","","","","","",
"","","","","","","","","","","","","","","","","","","","","","","","","","",
"","","U comp. of current wrt grid","V comp. of current wrt grid","","","","",
"","","","","","","","","","","","","","","","","","","","","","","","","","",
"","","","","","","","","","","","","","","","","","","","","","","","","","",
"","","","","","","","","","","","","","","","","","","","","","",
"Water temperature","Height of wind-driven waves","Height of sea swells",
"Combined height of waves and swell","Period of wind-driven waves",
"Direction from which waves are moving (wrt North)","Period of sea swells",
"Direction from which swells are moving (wrt North)","Ice-free water surface",
"","","","","","","","Significant wave height","Primary wave period",
"Direction from which primary waves are moving (wrt North)",
"Secondary wave period",
"Direction from which secondary waves are moving (wrt North)",
"White cap coverage","","","","","",
"Surface stress in zonal direction","Surface stress in meridional direction",
"Surface sensible heat flux","Surface latent heat flux","Skin temperature",
"Soil wetness","Snow depth","Downward longwave flux at surface",
"Upward longwave flux at surface","Upward longwave flux at top of model",
"Upward shortwave flux at top of model","Upward shortwave flux at surface",
"Downward shortwave flux at surface","High cloud fractions",
"Mid cloud fractions","Low cloud fractions",
"High cloud top and bottom sigma layer number",
"Mid cloud top and bottom sigma layer number",
"Low cloud top and bottom sigma layer number",
"High cloud top temperature","Mid cloud top temperature",
"Low cloud top temperature","12-hour accumulated rain amount",
"Accumulated convective rain","Ground heat flux","10-meter zonal wind",
"10-meter meridional wind","Sigma level 1 temperature",
"Sigma level 1 specific humidity","Surface pressure",
"","","","","","","","","","","","","","","","","","","","","","",
"6-hour accumulated rain amount","","","","","2-meter temperature",
"2-meter specific humidity",""};
const char *ON84Grid::parameter_units[ON84Grid::PARAMETER_SIZE]={"","gpm","gpm","","","","m","m","mbars",
"mbars/sec","","","","","","","degK","degK","degK","degK","degK","degK","degK",
"","","","","","","","","","","","","","","","","","mbars/sec","mbars","m/sec",
"m/sec","m/sec","","","","m/sec","m/sec","m/sec","m/sec","1/sec","m/sec",
"m/sec","degrees","m/sec","m/sec","non-dimensional","m/sec","percent","m/sec",
"m/sec^2","m/sec^2","","","","","","","","","1/sec","1/sec","1/sec","","","","",
"","m^2/sec","m^2/sec","N/m^2","N/m^2","N/m^2","N/m^2","N/m^2","N/m^2",
"percent","kg/m^2","m","percent","percent","m","m","kg/kg","kg/kg","kg/m^2/sec",
"percent","percent","percent","kg/kg","percent","kg/kg/sec","mbars","m","m/sec",
"kg/m^2/sec","kg/m^2/sec","kg/kg/sec","kg/kg/sec","kg/kg/sec","degK","degK",
"degK","degK","degK","m","","","gpm","gpm","","","","","","","","","","","","",
"","","","","","","","","","","sigma","sigma","sigma","sigma","sigma","","","",
"","","","","","","","","non-dimensional","non-dimensional","non-dimensional",
"mbars/m","mbars/m","percent/m","degK/m","m/m","mbars","non-dimensional",
"W/m^2","degK/sec","(various)","W/m^2","W/m^2","W/m^2","degrees_N","degrees_W",
"non-dimensional","m","m","binary","binary","binary","percent","percent",
"m/sec","degK","m","integer","W/m^2","W/m^2","W/m^2","W/m^2","W/m^2",
"kg/m^2/sec","degK/sec","degK/sec","degK/sec","non-dimensional","W/m^2/sr/m",
"degK","kg/m^2","kg/kg","W/m^2","degK/sec","degK/sec","degK/sec","degK/sec",
"J/m^2/degree","non-dimensional","non-dimensional","mbars","mbars","mbars",
"mbars","kg/m^2/sec","m","m","","","","","","","","","","","","","","","","","",
"","","","","","","","","","","","","","","","","","","","","","","","","","",
"","","","","","","","","","","","","","","","","","","","","","","","","","",
"","","","","","","","","","","","","","","","m/sec","m/sec","","","","","","",
"","","","","","","","","","","","","","","","","","","","","","","","","","",
"","","","","","","","","","","","","","","","","","","","","","","","","","",
"","","","","","","","","","","","","","","","","","","","","degK","m","m","m",
"sec","degrees","sec","degrees","percent","","","","","","","","m","sec",
"degrees","sec","degrees","percent","","","","","","N/m^2","N/m^2","W/m^2",
"W/m^2","degK","mm","mm","W/m^2","W/m^2","W/m^2","W/m^2","W/m^2","W/m^2","","","","","","","degK","degK","degK","m","m","W/m^2","m/s","m/s","degK","gm/gm","mb",
"","","","","","","","","","","","","","","","","","","","","","","m","","","",
"","degK","degK",""};

int InputON84GridStream::peek()
{
  if (icosstream != nullptr) {
    return icosstream->peek();
  }
  else if (ivstream != nullptr) {
    return ivstream->peek();
  }
  else {
    unsigned char buffer[32];
    fs.read(reinterpret_cast<char *>(buffer),32);
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (!fs.good()) {
	return bfstream::error;
    }
    int n=fs.gcount();
    bits::get(buffer,n,240,16);
    fs.seekg(-32,std::ios_base::cur);
    return (n*2+48);
  }
}

int InputON84GridStream::read(unsigned char *buffer,size_t buffer_length)
{
  int bytes_read,num_points,length,test;
  bool got_a_good_record=false;

// read a grid from the stream
  if (icosstream != nullptr) {
    while (!got_a_good_record) {
	if ( (bytes_read=icosstream->read(buffer,buffer_length)) <= 0) {
	  return bytes_read;
	}
	if (static_cast<size_t>(bytes_read) == buffer_length) {
	  std::cerr << "Error: buffer overflow" << std::endl;
	  exit(1);
	}
	else if (bytes_read > 48) {
	  bits::get(buffer,num_points,240,16);
	  bits::get(buffer,length,256,16);
	  test=num_points*2+48;
	  if (test == length || test == bytes_read) {
	    got_a_good_record=true;
	  }
	  else {
// special case for TDL radar data
	    if (lround(num_points*4./8.)+48 == length) {
		got_a_good_record=true;
	    }
	  }
	}
    }
  }
  else if (ivstream != nullptr) {
    while (!got_a_good_record) {
	if ( (bytes_read=ivstream->read(buffer,buffer_length)) <= 0) {
	  return bytes_read;
	}
	if (static_cast<size_t>(bytes_read) == buffer_length) {
	  std::cerr << "Error: buffer overflow" << std::endl;
	  exit(1);
	}
	bits::get(buffer,num_points,240,16);
	bits::get(buffer,length,256,16);
	if (num_points*2+48 == length) {
	  got_a_good_record=true;
	}
    }
  }
  else {
    fs.read(reinterpret_cast<char *>(buffer),32);
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (!fs.good()) {
	return bfstream::error;
    }
    bytes_read=fs.gcount();
    bits::get(buffer,num_points,240,16);
    length=num_points*2+48;
    if (static_cast<size_t>(length) > buffer_length) {
	std::cerr << "Error: buffer overflow" << std::endl;
	exit(1);
    }
    fs.read(reinterpret_cast<char *>(&buffer[32]),length-32);
    bytes_read+=fs.gcount();
    if (bytes_read != length) {
	bytes_read=bfstream::error;
    }
  }
  ++num_read;
  return bytes_read;
}

ON84Grid& ON84Grid::operator=(const ON84Grid& source)
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
  if (gridpoints_ != nullptr && on84.capacity < source.on84.capacity) {
    for (n=0; n < dim.y; ++n) {
	delete[] gridpoints_[n];
    }
    delete[] gridpoints_;
    gridpoints_=nullptr;
  }
  if (source.grid.filled) {
    if (gridpoints_ == nullptr) {
	gridpoints_=new double *[dim.y];
	for (n=0; n < dim.y; ++n) {
	  gridpoints_[n]=new double[dim.x];
	}
	on84.capacity=dim.size;
    }
    for (n=0; n < dim.y; ++n) {
	for (m=0; m < dim.x; ++m) {
	  gridpoints_[n][m]=source.gridpoints_[n][m];
	}
    }
  }
  on84=source.on84;
  return *this;
}

size_t ON84Grid::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}

void ON84Grid::create_global_grid(const ON84Grid& NHGrid,const ON84Grid& SHGrid)
{
  int n;

  if (NHGrid.grid.grid_type != 29 || SHGrid.grid.grid_type != 30) {
    std::cerr << "Error: bad grid type(s) -- NH: " << NHGrid.grid.grid_type << "  SH: " << SHGrid.grid.grid_type << std::endl;
    exit(1);
  }
  if (NHGrid.reference_date_time_ != SHGrid.reference_date_time_) {
    std::cerr << "Error: bad date match -- NH: " << NHGrid.reference_date_time_.to_string() << "  SH: " << SHGrid.reference_date_time_.to_string() << std::endl;
    exit(1);
  }

  reference_date_time_=NHGrid.reference_date_time_;
  dim.x=NHGrid.dim.x;
  dim.y=NHGrid.dim.x+NHGrid.dim.y;
  dim.size=NHGrid.dim.size+SHGrid.dim.size;
  def.slatitude=SHGrid.def.slatitude;
  def.elatitude=NHGrid.def.elatitude;
  def.slongitude=NHGrid.def.slongitude;
  def.elongitude=NHGrid.def.elongitude;
  def.laincrement=NHGrid.def.laincrement;
  def.loincrement=NHGrid.def.loincrement;
  on84=NHGrid.on84;

  if (gridpoints_ != nullptr && on84.capacity < dim.size) {
    for (n=0; n < dim.y; n++)
	delete[] gridpoints_[n];
    delete[] gridpoints_;
    gridpoints_=nullptr;
  }
  if (gridpoints_ == nullptr) {
    gridpoints_=new double *[dim.y];
    for (n=0; n < dim.y; n++)
	gridpoints_[n]=new double[dim.x];
    on84.capacity=dim.size;
  }

// copy SH grid
// copy NH grid
}

void ON84Grid::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  int n,m,cnt=0,avg_cnt=0;
  int c,exp,scale,*packed,end;
  short yr,mo,dy,time;

  if (stream_buffer == nullptr) {
    std::cerr << "Error: empty file stream" << std::endl;
    exit(1);
  }
  grid.filled=false;

  bits::get(stream_buffer,grid.param,0,12);
  bits::get(stream_buffer,grid.level1_type,12,12);
  bits::get(stream_buffer,c,36,20);
  if (c > 0x80000) c=0x80000-c;
  bits::get(stream_buffer,exp,56,8);
  if (exp > 0x08) exp=0x80-exp;
  grid.level1=c*pow(10.,exp);
  bits::get(stream_buffer,grid.level2_type,76,12);
  bits::get(stream_buffer,c,100,20);
  if (c >= 0x80000) c=0x80000-c;
  bits::get(stream_buffer,exp,120,8);
  if (exp >= 0x08) exp=0x80-exp;
  grid.level2=c*pow(10.,exp);
  bits::get(stream_buffer,grid.grid_type,152,8);
  bits::get(stream_buffer,dim.size,240,16);
  if (gridpoints_ != nullptr && dim.size != on84.capacity) {
    for (n=0; n < dim.y; n++)
	delete[] gridpoints_[n];
    delete[] gridpoints_;
    gridpoints_=nullptr;
  }
  switch (grid.grid_type) {
    case 0:
	dim.x=47;
	dim.y=51;
	def.type=Grid::polarStereographicType;
	def.slatitude=-4.86;
	def.slongitude=-122.61;
	def.dx=def.dy=381.;
	def.llatitude=60.;
	def.projection_flag=0;
fill_header_only=true;
	break;
    case 1:
	dim.x=73;
	dim.y=23;
	def.type=Grid::mercatorType;
	def.slatitude=-48.09;
	def.slongitude=0.;
	def.elatitude=48.09;
	def.elongitude=0.;
	def.loincrement=5.;
	break;
    case 5:
	dim.x=53;
	dim.y=57;
	def.type=Grid::polarStereographicType;
	def.slatitude=27.046;
	def.slongitude=-147.879;
	def.llatitude=60.;
	def.olongitude=-105;
	def.dx=def.dy=190.5;
	def.projection_flag=0;
	break;
    case 26:
	dim.x=53;
	dim.y=45;
	def.type=Grid::polarStereographicType;
	def.slatitude=7.647;
	def.slongitude=-133.443;
	def.olongitude=-105.;
	def.dx=def.dy=190.5;
	def.llatitude=60.;
	def.projection_flag=0;
	break;
    case 27:
	dim.x=dim.y=65;
	def.type=Grid::polarStereographicType;
	def.slatitude=-20.83;
	def.slongitude=-125.0;
	def.olongitude=-80.;
	def.dx=def.dy=381;
	def.llatitude=60.;
	def.projection_flag=0;
	break;
    case 28:
	dim.x=dim.y=65;
	def.type=Grid::polarStereographicType;
	def.slatitude=20.83;
	def.slongitude=145.0;
	def.olongitude=100.;
	def.dx=def.dy=381;
	def.llatitude=-60.;
	def.projection_flag=1;
	break;
    case 29:
	dim.x=145;
	dim.y=37;
	def.type=Grid::latitudeLongitudeType;
	def.slatitude=0.;
	def.elatitude=90.;
	def.laincrement=2.5;
	def.slongitude=0.;
	def.elongitude=360.;
	def.loincrement=2.5;
	break;
    case 30:
	dim.x=145;
	dim.y=37;
	def.type=Grid::latitudeLongitudeType;
	def.slatitude=-90.;
	def.elatitude=0.;
	def.laincrement=2.5;
	def.slongitude=0.;
	def.elongitude=360.;
	def.loincrement=2.5;
	break;
    case 32:
	dim.x=31;
	dim.y=24;
	def.type=Grid::polarStereographicType;
	def.slatitude=21.215;
	def.slongitude=-121.314;
	def.olongitude=-105.;
	def.dx=def.dy=190.5;
	def.llatitude=60.;
	def.projection_flag=0;
	break;
    case 33:
	dim.x=181;
	dim.y=46;
	def.type=Grid::latitudeLongitudeType;
	def.slatitude=0.;
	def.elatitude=90.;
	def.laincrement=2.;
	def.slongitude=0.;
	def.elongitude=360.;
	def.loincrement=2.;
	break;
    case 34:
	dim.x=181;
	dim.y=46;
	def.type=Grid::latitudeLongitudeType;
	def.slatitude=-90.;
	def.elatitude=0.;
	def.laincrement=2.;
	def.slongitude=0.;
	def.elongitude=360.;
	def.loincrement=2.;
	break;
    case 36:
	dim.x=41;
	dim.y=38;
	def.type=Grid::polarStereographicType;
	def.slatitude=18.682;
	def.slongitude=-128.702;
	def.olongitude=-105.;
	def.dx=def.dy=190.5;
	def.llatitude=60.;
	def.projection_flag=0;
	break;
    case 37:
	dim.x=145;
	dim.y=37;
	def.type=Grid::latitudeLongitudeType;
	def.slatitude=1.25;
	def.elatitude=91.25;
	def.laincrement=2.5;
	def.slongitude=1.25;
	def.elongitude=361.25;
	def.loincrement=2.5;
	break;
    case 38:
	dim.x=145;
	dim.y=37;
	def.type=Grid::latitudeLongitudeType;
	def.slatitude=-91.25;
	def.elatitude=-1.25;
	def.laincrement=2.5;
	def.slongitude=1.25;
	def.elongitude=361.25;
	def.loincrement=2.5;
	break;
    case 47:
	dim.x=113;
	dim.y=89;
	def.type=Grid::polarStereographicType;
	def.slatitude=23.097;
	def.slongitude=-119.036;
	def.olongitude=-105.;
	def.dx=def.dy=47.625;
	def.llatitude=60.;
	def.projection_flag=0;
	break;
    case 90:
// this is a patch; the grid is really a rotated lat/lon grid
	dim.x=92;
	dim.y=141;
	def.type=Grid::staggeredLatitudeLongitudeType;
	def.slatitude=15.;
	def.elatitude=75.;
	def.laincrement=0.53846;
	def.slongitude=-135.;
	def.elongitude=-60.;
	def.loincrement=0.57692;
	break;
    case 101:
	dim.x=113;
	dim.y=91;
	def.type=Grid::polarStereographicType;
	def.slatitude=10.528;
	def.slongitude=-137.146;
	def.olongitude=-105.;
	def.dx=def.dy=91.452;
	def.llatitude=60.;
	def.projection_flag=0;
	break;
    case 104:
	dim.x=147;
	dim.y=110;
	def.type=Grid::polarStereographicType;
	def.slatitude=-0.269;
	def.slongitude=-139.475;
	def.olongitude=-105.;
	def.dx=def.dy=90.75464;
	def.llatitude=60.;
	def.projection_flag=0;
	break;
    default:
	std::cerr << "Error: unknown grid type " << grid.grid_type << std::endl;
	exit(1);
  }
  bits::get(stream_buffer,yr,192,8);
  if (yr > 30)
    yr+=1900;
  else
    yr+=2000;
  bits::get(stream_buffer,mo,200,8);
  bits::get(stream_buffer,dy,208,8);
  bits::get(stream_buffer,time,216,8);
  reference_date_time_.set(yr,mo,dy,time*10000);
  bits::get(stream_buffer,on84.time_mark,32,4);
  grid.fcst_time=0;
  switch (on84.time_mark) {
    case 0:
    {
	bits::get(stream_buffer,on84.f1,24,8);
	grid.fcst_time=on84.f1*10000;
	valid_date_time_=reference_date_time_.time_added(grid.fcst_time);
	on84.f2=0;
	grid.nmean=0;
	break;
    }
    case 2:
    {
	bits::get(stream_buffer,on84.f1,24,8);
	bits::get(stream_buffer,on84.f2,88,8);
	valid_date_time_=reference_date_time_.time_added(on84.f2);
	grid.fcst_time=on84.f1*10000;
	break;
    }
    case 3:
    {
	bits::get(stream_buffer,on84.f1,24,8);
	bits::get(stream_buffer,on84.f2,88,8);
	grid.fcst_time=on84.f2*10000;
	valid_date_time_=reference_date_time_.time_added(grid.fcst_time);
	grid.nmean=0;
	break;
    }
    case 4:
    {
	bits::get(stream_buffer,on84.f1,24,8);
	bits::get(stream_buffer,on84.f2,88,8);
	grid.nmean= (on84.f1 != 0) ? on84.f1 : on84.f2;
	valid_date_time_=reference_date_time_;
	break;
    }
    default:
    {
	std::cerr << "Error: time marker " << on84.time_mark << " not recognized" << std::endl;
	exit(1);
    }
  }
  bits::get(stream_buffer,on84.run_mark,224,8);
  bits::get(stream_buffer,on84.gen_prog,232,8);

  if (!fill_header_only) {
    auto base=floatutils::ibmconv(stream_buffer,288);
    bits::get(stream_buffer,scale,336,16);
    if (scale >= 0x8000) scale-=0x10000;
    scale-=15;
    packed=new int[dim.size];
    if (gridpoints_ == nullptr) {
	gridpoints_=new double *[dim.y];
	for (n=0; n < dim.y; n++)
	  gridpoints_[n]=new double[dim.x];
	on84.capacity=dim.size;
    }
    bits::get(stream_buffer,packed,384,16,0,dim.size);
    stats.max_val=-Grid::missing_value;
    stats.min_val=Grid::missing_value;
    stats.avg_val=0.;

    switch (grid.grid_type) {
	case 32:
	case 36:
	  for (m=0; m < dim.x; m++) {
	    for (n=0; n < dim.y; n++) {
		if (packed[cnt] >= 0x8000) packed[cnt]-=0x10000;
		gridpoints_[n][m]=packed[cnt]*pow(2.,scale)+base;
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
		cnt++;
	    }
	  }
	  break;
	default:
	  for (n=0; n < dim.y; n++) {
	    if (def.type == Grid::staggeredLatitudeLongitudeType)
		end= ( (n % 2) == 0) ? dim.x : dim.x-1;
	    else
		end=dim.x;
	    for (m=0; m < end; m++) {
		if (packed[cnt] >= 0x8000)
		  packed[cnt]-=0x10000;
		gridpoints_[n][m]=packed[cnt]*pow(2.,scale)+base;
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
		cnt++;
	    }
	    if (def.type == Grid::staggeredLatitudeLongitudeType && (n % 2) != 0)
		gridpoints_[n][end]=Grid::missing_value;
	  }
    }

    if (avg_cnt > 0)
	stats.avg_val/=static_cast<float>(avg_cnt);
    switch (grid.grid_type) {
	case 27:
	case 28:
	  grid.pole=gridpoints_[32][32];
	  break;
	case 29:
	  grid.pole=gridpoints_[36][112];
	  break;
	case 30:
	  grid.pole=gridpoints_[0][112];
	  break;
	case 33:
	  grid.pole=gridpoints_[45][140];
	  break;
	case 34:
	  grid.pole=gridpoints_[0][140];
	  break;
	default:
	  grid.pole=Grid::missing_value;
    }
    grid.filled=true;

    delete[] packed;
  }
}

bool ON84Grid::is_averaged_grid() const
{
  switch (on84.time_mark) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 5:
    case 7:
    case 10:
	return false;
    case 4:
    case 6:
	return true;
    default:
	std::cerr << "Error: time marker " << on84.time_mark << " not recognized - unable to determine whether or not grid is an average" << std::endl;
	exit(1);
  }

  return false;
}

void ON84Grid::print(std::ostream& outs) const
{
  int n,m,stop,l;

  bool scientific=false;
  if (!floatutils::myequalf(stats.avg_val,0.) && fabs(stats.avg_val) < 0.01) {
    scientific=true;
  }
  switch (grid.grid_type) {
    case 29:
    case 30:
    {
	for (n=0; n < dim.x; n+=15) {
	  stop=n+15;
	  if (stop > dim.x) {
	    stop=dim.x;
	  }
	  outs << "    \\ LON";
	  for (l=n; l < stop; ++l) {
	    outs << std::setw(9) << l*def.loincrement;
	  }
	  outs << "\n LAT \\ +-";
	  for (l=n; l < stop; ++l) {
	    outs << "---------";
	  }
	  outs << std::endl;
	  for (m=dim.y-1; m >= 0; --m) {
	    outs << std::setw(6) << def.slatitude+m*def.laincrement << " | ";
	    outs.precision(2);
	    for (l=n; l < stop; ++l) {
		if (floatutils::myequalf(gridpoints_[m][l],Grid::missing_value)) {
		  outs << "         ";
		}
		else {
		  if (scientific) {
		    outs.unsetf(std::ios::fixed);
		    outs.setf(std::ios::scientific);
		  }
		  outs << std::setw(9) << gridpoints_[m][l];
		  if (scientific) {
		    outs.unsetf(std::ios::scientific);
		    outs.setf(std::ios::fixed);
		  }
		}
	    }
	    outs << std::endl;
	  }
	  outs << std::endl;
	}
	break;
    }
    case 27:
    case 28:
    case 36:
    case 47:
    {
	for (n=0; n < dim.x; n+=15) {
	  stop=n+15;
	  if (stop > dim.x) {
	    stop=dim.x;
	  }
	  outs << " \\ X  ";
	  for (l=n; l < stop; ++l) {
	    outs << std::setw(9) << l+1;
	  }
	  outs << "\nY \\ +-";
	  for (l=n; l < stop; ++l) {
	    outs << "---------";
	  }
	  outs << std::endl;
	  for (m=dim.y-1; m >= 0; --m) {
	    outs << std::setw(3) << m+1 << " | ";
	    outs.precision(2);
	    for (l=n; l < stop; ++l) {
		if (floatutils::myequalf(gridpoints_[m][l],Grid::missing_value)) {
		  outs << "         ";
		}
		else {
		  if (scientific) {
		    outs.unsetf(std::ios::fixed);
		    outs.setf(std::ios::scientific);
		  }
		  outs << std::setw(9) << gridpoints_[m][l];
		  if (scientific) {
		    outs.unsetf(std::ios::scientific);
		    outs.setf(std::ios::fixed);
		  }
		}
	    }
	    outs << std::endl;
	  }
	  outs << std::endl;
	}
	break;
    }
    default:
    {
	std::cerr << "Error: unknown grid type " << grid.grid_type << std::endl;
	exit(1);
    }
  }
}

void ON84Grid::print(std::ostream& outs,float bottom_latitude,float top_latitude,float west_longitude,float east_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
}

void ON84Grid::print_ascii(std::ostream& outs) const
{
}

void ON84Grid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
  bool scientific=false;

  if (!floatutils::myequalf(stats.avg_val,0.) && fabs(stats.avg_val) < 0.01) {
    scientific=true;
  }
  outs.setf(std::ios::fixed);
  outs.precision(1);
  if (verbose) {
    outs << "  Time: " << reference_date_time_.to_string() << "  Valid Time: " << valid_date_time_.to_string() << "  NumAvg: " << std::setw(3) << grid.nmean << "  TMarker: " << std::setw(2) << on84.time_mark << "  F1: " << std::setw(4) << on84.f1 << "  F2: " << std::setw(4) << on84.f2 << "  Param: " << std::setw(3) << grid.param << " Name: ";
    if (static_cast<size_t>(grid.param) < ON84Grid::PARAMETER_SIZE) {
	outs << parameter_short_name[grid.param];
    }
    else {
	outs << grid.param;
    }
    if (floatutils::myequalf(grid.level2,0.)) {
	if (floatutils::myequalf(grid.level1,0.))
	  outs << "  Level: " << parameter_units[grid.level1_type];
	else {
	  outs.precision(3);
	  outs << "  Level: " << std::setw(8) << grid.level1 << parameter_units[grid.level1_type];
	  outs.precision(1);
	}
    }
    else {
	outs << "  Levels: ";
	outs.precision(3);
	outs << std::setw(8) << first_level_value() << parameter_units[grid.level1_type] << ", " << std::setw(8) << second_level_value() << parameter_units[grid.level2_type];
	outs.precision(1);
    }
    outs << "\n  Grid: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  NumPoints: " << std::setw(5) << dim.size << "  Type: " << std::setw(3) << grid.grid_type;
    switch (grid.grid_type) {
	case 29:
	case 30:
	case 33:
	case 34:
	  outs << "  LonRange: " << std::setw(6) << def.slongitude << " to " << std::setw(6) << def.elongitude << " by " << std::setw(4) << def.loincrement << "  LatRange: " << std::setw(5) << def.slatitude << " to " << std::setw(5) << def.elatitude << " by " << std::setw(4) << def.laincrement;
	  break;
	case 27:
	case 28:
	case 36:
	  outs << "  Lat1: " << std::setw(5) << def.slatitude << "  Lon1: " << std::setw(6) << def.slongitude << "  Orient: " << std::setw(6) << def.olongitude << "  Lengths - Dx: " << std::setw(6) << def.dx << " Dy: " << std::setw(6) << def.dy << " at " << std::setw(5) << def.llatitude;
	  break;
    }
    outs << "  RMarker: " << std::setw(2) << on84.run_mark << "  GenProg: " << std::setw(2) << on84.gen_prog << std::endl;
    if (grid.filled) {
	if (scientific) {
	  outs.unsetf(std::ios::fixed);
	  outs.setf(std::ios::scientific);
	  outs.precision(3);
	}
	else
	  outs.precision(2);
	outs << "  MinVal: " << stats.min_val << " (" << stats.min_i << "," << stats.min_j << ")  MaxVal: " << stats.max_val << " (" << stats.max_i << "," << stats.max_j << ")  AvgVal: " << stats.avg_val << "  Pole: ";
	if (floatutils::myequalf(grid.pole,Grid::missing_value))
	  outs << "N/A" << std::endl;
	else
	  outs << grid.pole << std::endl;
	if (scientific) {
	  outs.unsetf(std::ios::scientific);
	  outs.setf(std::ios::fixed);
	}
	outs.precision(1);
    }
  }
  else {
    outs << " Type=" << grid.grid_type << " Time=" << reference_date_time_.to_string("%Y%m%d%H") << " ValidTime=" << valid_date_time_.to_string("%Y%m%d%H") << " NAvg=" << grid.nmean << " TMark=" << on84.time_mark << " F1=" << on84.f1 << " F2=" << on84.f2 << " Param=" << grid.param << " Name=";
    if (static_cast<size_t>(grid.param) < ON84Grid::PARAMETER_SIZE) {
	outs << parameter_short_name[grid.param];
    }
    if (floatutils::myequalf(grid.level2,0.)) {
	if (floatutils::myequalf(grid.level1,0.))
	  outs << " Level=" << parameter_short_name[grid.level1_type];
	else {
	  outs.precision(3);
	  outs << " Level=" << grid.level1 << parameter_units[grid.level1_type];
	  outs.precision(1);
	}
    }
    else {
	outs << " Levels=";
	outs.precision(3);
	outs << grid.level1 << parameter_units[grid.level1_type] << "," << grid.level2 << parameter_units[grid.level2_type];
	outs.precision(1);
    }
    outs << " RMark=" << on84.run_mark << " GenProg=" << on84.gen_prog;
    if (grid.filled) {
	if (scientific) {
	  outs.unsetf(std::ios::fixed);
	  outs.setf(std::ios::scientific);
	  outs.precision(3);
	}
	else
	  outs.precision(2);
	outs << " Min=" << stats.min_val << " Max=" << stats.max_val << " Avg=" << stats.avg_val << " Pole=";
	if (floatutils::myequalf(grid.pole,Grid::missing_value))
	  outs << "N/A";
	else
	  outs << grid.pole;
	if (scientific) {
	  outs.unsetf(std::ios::scientific);
	  outs.setf(std::ios::fixed);
	}
	outs.precision(1);
    }
    outs << std::endl;
  }
}

int InputQuasiON84GridStream::read(unsigned char *buffer,size_t buffer_length)
{
  int nb;

// read a grid from the stream
  if (icosstream != nullptr) {
// eof
    if (icosstream->peek() == bfstream::eof) {
	return bfstream::eof;
    }
// new header
    if ( (nb=icosstream->peek()) == 32) {
	unsigned char header[32];
	icosstream->read(header,32);
	if ( (nb=icosstream->peek()) == 24) {
	  icosstream->read(header,24);
	  union {
	    int idum;
	    float fdum;
	  };
	  bits::get(header,idum,0,32);
	  fcst_hr=fdum;
	  bits::get(header,hr,32,32);
	  bits::get(header,mo,64,32);
	  bits::get(header,dy,96,32);
	  bits::get(header,yr,128,32);
std::cerr << fcst_hr << " " << yr << " " << mo << " " << dy << " " << hr << std::endl;
	  rec_num=0;
	}
	else {
	  return bfstream::error;
	}
    }
// on84 record
    int b1;
    if ( (nb=icosstream->read(buffer,buffer_length)) > 0) {
	b1=nb;
	if ( (nb=icosstream->read(&buffer[nb],buffer_length-nb)) > 0) {
	  nb+=b1;;
std::cerr << "on84 " << nb << " " << rec_num << std::endl;
	  ++rec_num;
	  switch (rec_num) {
	    case 1:
	    case 2:
	    case 3:
	    case 4:
	    case 5:
	    case 7:
	    case 8:
	    case 9:
	    case 12:
	    case 13:
	    {
		bits::set(buffer,410+rec_num,0,12);
		bits::set(buffer,129,12,12);
		break;
	    }
	    case 6:
	    {
		bits::set(buffer,410+rec_num,0,12);
		bits::set(buffer,7,12,12);
		bits::set(buffer,0,36,20);
		bits::set(buffer,7,76,12);
		bits::set(buffer,150,100,20);
		break;
	    }
	    case 10:
	    case 11:
	    {
		bits::set(buffer,410+rec_num,0,12);
		break;
	    }
	    case 14:
	    {
		bits::set(buffer,410+rec_num,0,12);
		break;
	    }
	    case 15:
	    {
		bits::set(buffer,410+rec_num,0,12);
		break;
	    }
	    case 16:
	    {
		bits::set(buffer,410+rec_num,0,12);
		break;
	    }
	    case 17:
	    {
		bits::set(buffer,410+rec_num,0,12);
		break;
	    }
	    case 18:
	    {
		bits::set(buffer,410+rec_num,0,12);
		break;
	    }
	    case 19:
	    {
		bits::set(buffer,410+rec_num,0,12);
		break;
	    }
	    case 20:
	    {
		bits::set(buffer,410+rec_num,0,12);
		break;
	    }
	    case 21:
	    {
		bits::set(buffer,410+rec_num,0,12);
		break;
	    }
	    case 22:
	    {
		bits::set(buffer,410+rec_num,0,12);
		break;
	    }
	    case 23:
	    {
		if (fcst_hr < 7.) {
		  bits::set(buffer,410+rec_num,0,12);
		}
		else {
		  bits::set(buffer,440+rec_num,0,12);
		}
		bits::set(buffer,129,12,12);
		break;
	    }
	    case 24:
	    {
		bits::set(buffer,410+rec_num,0,12);
		break;
	    }
	    case 25:
	    {
		bits::set(buffer,410+rec_num,0,12);
		break;
	    }
	    case 26:
	    {
		bits::set(buffer,410+rec_num,0,12);
		break;
	    }
	    case 27:
	    {
		bits::set(buffer,410+rec_num,0,12);
		break;
	    }
	    case 28:
	    case 29:
	    {
		if (yr < 90 || (yr == 90 && (mo < 5 || (mo == 5 && dy < 31)))) {
		  bits::set(buffer,410+rec_num,0,12);
		}
		else {
		  bits::set(buffer,440+rec_num,0,12);
		}
		break;
	    }
	    case 30:
	    {
		bits::set(buffer,410+rec_num,0,12);
		bits::set(buffer,129,12,12);
		break;
	    }
	  }
	  bits::set(buffer,lroundf(fcst_hr),24,8);
	  bits::set(buffer,254,152,8);
	  bits::set(buffer,yr,192,8);
	  bits::set(buffer,mo,200,8);
	  bits::set(buffer,dy,208,8);
	  bits::set(buffer,hr,216,8);
	}
    }
    return nb;
  }
  else {
    return bfstream::error;
  }
}
