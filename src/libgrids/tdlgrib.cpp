// FILE: tdlgribgrid.cpp

#include <iomanip>
#include <grid.hpp>
#include <utils.hpp>
#include <bits.hpp>


int InputTDLGRIBGridStream::peek()
{
return bfstream::error;
}

int InputTDLGRIBGridStream::read(unsigned char *buffer,size_t buffer_length)
{
  size_t reclen,bytes_read;

// read a grid from the stream
  if (ics != NULL) {
std::cerr << "Error: unable to read cos-blocked TDLGRIBGridStream" << std::endl;
exit(1);
  }
  else if (if77s != NULL) {
    if ( (bytes_read=if77s->read(buffer,buffer_length)) <= 0)
	return bytes_read;
    if (bytes_read == buffer_length) {
	std::cerr << "Error: buffer overflow" << std::endl;
	exit(1);
    }
// check for the presence of 'TDLP' in bytes 8-11
    if (buffer[8] != 0x54 && buffer[9] != 0x44 && buffer[10] != 0x4c &&
        buffer[11] != 0x50) {
	std::cerr << "Error: not a TDL GRIB record" << std::endl;
	exit(1);
    }
  }
  else {
    fs.read(reinterpret_cast<char *>(buffer),12);
    if (fs.eof()) {
	return bfstream::eof;
    }
    else if (fs.gcount() != 12) {
	return bfstream::error;
    }
// check for the presence of 'TDLP' in bytes 8-11
    if (buffer[8] != 0x54 && buffer[9] != 0x44 && buffer[10] != 0x4c &&
        buffer[11] != 0x50) {
	std::cerr << "Error: not a TDL GRIB record" << std::endl;
	exit(1);
    }
// get the length of the current record
    bits::get(buffer,reclen,32,32);
    if (reclen+8 > buffer_length) {
	std::cerr << "Error: buffer overflow" << std::endl;
	exit(1);
    }
    fs.read(reinterpret_cast<char *>(&buffer[12]),reclen-4);
    bytes_read=fs.gcount();
    if (bytes_read < reclen+8)
	bytes_read=bfstream::error;
  }
  ++num_read;
  return bytes_read;
}

void TDLGRIBMessage::unpack_is(const unsigned char *stream_buffer)
{
  int test;

  bits::get(stream_buffer,test,64,32);
  if (test != 0x54444c50) {
    std::cerr << "Error: not a TDL GRIB record" << std::endl;
    exit(1);
  }
  bits::get(stream_buffer,mlength,96,24);
  bits::get(stream_buffer,m_edition,120,8);
  m_lengths.is=8;
  curr_off=16;
}

void TDLGRIBMessage::unpack_pds(const unsigned char *stream_buffer)
{
  short off=curr_off*8,soff,mins;
  size_t dum;
  unsigned char flag;
  short yr,mo,dy,time;
  TDLGRIBGrid *t;

  t=reinterpret_cast<TDLGRIBGrid *>(grids.back().get());
  bits::get(stream_buffer,m_lengths.pds,off,8);
  bits::get(stream_buffer,flag,off+8,8);
  if ( (flag & 0x2) == 0x2)
    bms_included=true;
  else {
    bms_included=false;
    m_lengths.bms=0;
  }
  if ( (flag & 0x1) == 0x1)
    gds_included=true;
  else {
    gds_included=false;
    m_lengths.gds=0;
  }
  bits::get(stream_buffer,mins,off+56,8);
  bits::get(stream_buffer,dum,off+64,32);
  time=dum % 100;
  time=time*100+mins;
  dum/=100;
  dy=dum % 100;
  dum/=100;
  mo=dum % 100;
  dum/=100;
  yr=dum;
  t->m_reference_date_time.set(yr,mo,dy,time*100);
  t->grid.nmean=0;
  bits::get(stream_buffer,dum,off+96,32);
  t->grib.process=dum % 100;
  dum/=100;
  t->tdl.B=dum % 10;
  dum/=10;
  t->tdl.param=dum;
  bits::get(stream_buffer,dum,off+128,32);
  t->grid.level1=(dum % 10000);
  dum/=10000;
  t->grid.level2=(dum % 10000);
  dum/=10000;
  if (!floatutils::myequalf(t->grid.level2,0.)) {
    t->grid.level2_type=0;
  }
  t->tdl.V=dum;
  bits::get(stream_buffer,dum,off+160,32);
  t->grid.fcst_time=(dum % 1000)*10000;
  dum/=1000;
  t->tdl.HH=dum % 100;
  dum/=100;
  t->tdl.O=dum % 10;
  dum/=10;
  t->tdl.RR=dum % 100;
  dum/=100;
  t->tdl.T=dum;
  switch (t->tdl.O) {
    case 0:
	bits::get(stream_buffer,dum,off+240,8);
	if (dum != 0)
	  t->grid.fcst_time+=dum*100;
	break;
    default:
	std::cerr << "Error: unable to process O = " << t->tdl.O << std::endl;
	exit(1);
  }
  t->m_valid_date_time=t->m_reference_date_time.time_added(t->grid.fcst_time);
  bits::get(stream_buffer,t->tdl.seq,off+256,8);
  bits::get(stream_buffer,t->grib.D,off+264,8);
  if (t->grib.D > 0x80)
    t->grib.D=0x80-t->grib.D;
  bits::get(stream_buffer,t->tdl.E,off+272,8);
  if (t->tdl.E > 0x80)
    t->tdl.E=0x80-t->tdl.E;
// unpack PDS supplement, if it exists
  bits::get(stream_buffer,m_lengths.pds_supp,off+304,8);
  if (m_lengths.pds_supp > 0) {
    pds_supp.reset(new unsigned char[m_lengths.pds_supp]);
    soff=38;
    std::copy(&stream_buffer[curr_off+soff],&stream_buffer[curr_off+soff]+m_lengths.pds_supp,pds_supp.get());
  }
  else {
    pds_supp.reset(nullptr);
  }
// copy the 39 PDS bytes into a special location for use when converting to GRIB
  std::copy(&stream_buffer[curr_off],&stream_buffer[curr_off]+39,t->tdl.pds39_a);
  curr_off+=m_lengths.pds;
}

void TDLGRIBMessage::unpack_gds(const unsigned char *stream_buffer)
{
  short off=curr_off*8;
  int dum;
  TDLGRIBGrid *t;

  t=reinterpret_cast<TDLGRIBGrid *>(grids.back().get());
  bits::get(stream_buffer,m_lengths.gds,off,8);
  bits::get(stream_buffer,t->grid.grid_type,off+8,8);
  switch (t->grid.grid_type) {
// N. Hemisphere Polar Stereographic
    case 5:
	t->def.type=Grid::Type::polarStereographic;
	t->def.projection_flag=0;
	bits::get(stream_buffer,t->dim.x,off+16,16);
	bits::get(stream_buffer,t->dim.y,off+32,16);
	t->dim.size=t->dim.x*t->dim.y;
	bits::get(stream_buffer,dum,off+48,24);
	if (dum > 0x800000)	
	  dum=0x800000-dum;
	t->def.slatitude=dum/10000.;
	bits::get(stream_buffer,dum,off+72,24);
	if (dum > 0x800000)
	  dum=0x800000-dum;
	t->def.slongitude=dum/10000.;
	if (t->def.slongitude <= 180.)
	  t->def.slongitude=-t->def.slongitude;
	bits::get(stream_buffer,dum,off+96,24);
	if (dum > 0x800000)
	  dum=0x800000-dum;
	t->def.olongitude=dum/10000.;
	if (t->def.olongitude <= 180.)
	  t->def.olongitude=-t->def.olongitude;
	bits::get(stream_buffer,dum,off+120,32);
	t->def.dx=t->def.dy=dum/1000000.;
	bits::get(stream_buffer,dum,off+152,24);
	if (dum > 0x800000)
	  dum=0x800000-dum;
	t->def.llatitude=dum/10000.;
	break;
    default:
	std::cerr << "Error: unknown grid type " << t->grid.grid_type << std::endl;
	exit(1);
  }
  curr_off+=m_lengths.gds;
}

void TDLGRIBMessage::unpack_bms(const unsigned char *input_buffer)
{
std::cerr << "Error: bit-map section not currently supported" << std::endl;
exit(1);
}

void TDLGRIBMessage::unpack_bds(const unsigned char *stream_buffer,bool fill_header_only)
{
  short off=curr_off*8;
  short flag,bits[3];
  size_t sign;
  double e,d;
  size_t *grp_mins,*grp_bits,cnt=0,avg_cnt=0;
  int num_packed,num_grps,*nvals;
  int n,m,l;
  int *pval,omin=0,sum=0,inc;
  bool simple_packing=true,second_order=false;
  TDLGRIBGrid *t;

  t=reinterpret_cast<TDLGRIBGrid *>(grids.back().get());
  bits::get(stream_buffer,m_lengths.bds,off,24);
  bits::get(stream_buffer,flag,off+24,8);
  if ((flag & 0x10) != 0) {
    std::cerr << "Error: not gridpoint data" << std::endl;
    exit(1);
  }
  if ((flag & 0x8) == 0x8)
    simple_packing=false;
  bits::get(stream_buffer,num_packed,off+32,32);
  off+=64;
  if ((flag & 0x2) == 0x2) {
std::cerr << "Error: unable to handle missing values" << std::endl;
exit(1);
  }
  e=pow(2.,-(t->tdl.E));
  d=pow(10.,-(t->grib.D));
  if (simple_packing) {
// simple_packing
    std::cerr << "Error: simple unpacking not defined" << std::endl;
    exit(1);
  }
  else {
// complex packing
    switch (flag & 0x4) {
	case 0x4:
	{
	  second_order=true;
// first value in original field
	  bits::get(stream_buffer,sign,off,1);
	  bits::get(stream_buffer,t->tdl.first_val,off+1,31);
	  if (sign == 1) {
	    t->tdl.first_val=-(t->tdl.first_val);
	  }
// first 1st-order difference
	  bits::get(stream_buffer,bits[0],off+32,5);
	  bits::get(stream_buffer,sign,off+37,1);
	  bits::get(stream_buffer,t->tdl.first_diff,off+38,bits[0]);
	  if (sign == 1) {
	    t->tdl.first_diff=-(t->tdl.first_diff);
	  }
	  off=off+38+bits[0];
	}
	case 0x0:
	{
// overall minimum
	  bits::get(stream_buffer,bits[0],off,5);
	  bits::get(stream_buffer,sign,off+5,1);
	  bits::get(stream_buffer,omin,off+6,bits[0]);
	  if (sign == 1) {
	    omin=-omin;
	  }
	  off=off+6+bits[0];
	  if (fill_header_only) {
	    return;
	  }
	  bits::get(stream_buffer,num_grps,off,16);
	  bits::get(stream_buffer,bits,off+16,5,0,3);
	  off+=31;
	  grp_mins=new size_t[num_grps];
	  bits::get(stream_buffer,grp_mins,off,bits[0],0,num_grps);
	  off=off+bits[0]*num_grps;
	  grp_bits=new size_t[num_grps];
	  bits::get(stream_buffer,grp_bits,off,bits[1],0,num_grps);
	  off=off+bits[1]*num_grps;
	  nvals=new int[num_grps];
	  bits::get(stream_buffer,nvals,off,bits[2],0,num_grps);
	  off=off+bits[2]*num_grps;
	  pval=new int[num_packed];
	  for (n=0; n < num_grps; ++n) {
	    for (m=0; m < nvals[n]; ++m) {
		if (grp_bits[n] > 0) {
		  bits::get(stream_buffer,pval[cnt],off,grp_bits[n]);
		  off+=grp_bits[n];
		}
		else {
		  pval[cnt]=0;
		}
		pval[cnt]+=grp_mins[n];
		++cnt;
	    }
	  }
	  delete[] grp_mins;
	  delete[] grp_bits;
	  delete[] nvals;
	  if (t->m_gridpoints != NULL && t->dim.size > t->grib.capacity.points) {
	    for (n=0; n < t->dim.y; ++n) {
		delete[] t->m_gridpoints[n];
	    }
	    delete[] t->m_gridpoints;
	    t->m_gridpoints=NULL;
	  }
	  if (t->m_gridpoints == NULL) {
	    t->m_gridpoints=new double *[t->dim.y];
	    for (n=0; n < t->dim.y; ++n) {
		t->m_gridpoints[n]=new double[t->dim.x];
	    }
	    t->grib.capacity.points=t->dim.size;
	  }
	  cnt= (second_order) ? 2 : 0;
	  t->stats.min_val=Grid::MISSING_VALUE;
	  t->stats.max_val=-Grid::MISSING_VALUE;
	  t->stats.avg_val=0.;
	  for (n=0; n < t->dim.y; ++n) {
	    if ( (n % 2) == 0) {
		m=0;
		inc=1;
	    }
	    else {
		m=t->dim.x-1;
		inc=-1;
	    }
	    for (l=0; m < t->dim.x; l++,m+=inc) {
		if (second_order) {
		  if (n == 0 && l < 2) {
		    if (l == 0) {
			t->m_gridpoints[n][l]=t->tdl.first_val*e*d;
		    }
		    else if (l == 1) {
			sum=t->tdl.first_diff;
			t->m_gridpoints[n][l]=(t->tdl.first_val+sum)*e*d;
			t->tdl.first_val+=sum;
		    }
		  }
		  else {
		    sum+=(omin+pval[cnt++]);
		    t->m_gridpoints[n][m]=(t->tdl.first_val+sum)*e*d;
		    t->tdl.first_val+=sum;
		  }
		}
		else {
		  t->m_gridpoints[n][m]=(omin+pval[cnt++])*e*d;
		}
		t->stats.avg_val+=t->m_gridpoints[n][m];
		++avg_cnt;
		if (t->m_gridpoints[n][m] < t->stats.min_val) {
		  t->stats.min_val=t->m_gridpoints[n][m];
		  t->stats.min_i=m+1;
		  t->stats.min_j=n+1;
		}
		if (t->m_gridpoints[n][m] > t->stats.max_val) {
		  t->stats.max_val=t->m_gridpoints[n][m];
		  t->stats.max_i=m+1;
		  t->stats.max_j=n+1;
		}
	    }
	  }
	  delete[] pval;
	  if (avg_cnt > 0) {
	    t->stats.avg_val/=static_cast<float>(avg_cnt);
	  }
	  t->grid.filled=true;
	  break;
	}
    }
  }
}

void TDLGRIBMessage::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
  if (stream_buffer == nullptr) {
    std::cerr << "Error: empty file stream" << std::endl;
    exit(1);
  }
  for (auto& grid : grids) {
    grid.reset();
  }
  unpack_is(stream_buffer);
  auto t=new TDLGRIBGrid;
  t->grid.filled=false;
  grids.emplace_back(t);
  unpack_pds(stream_buffer);
  if (gds_included) {
    unpack_gds(stream_buffer);
  }
  if (bms_included) {
    unpack_bms(stream_buffer);
  }
  unpack_bds(stream_buffer,fill_header_only);
  unpack_end(stream_buffer);
}

void TDLGRIBMessage::print_header(std::ostream& outs,bool verbose) const
{
  if (verbose) {
    outs << " TDLP Ed " << m_edition << "  Lengths: " << mlength << std::endl;
    if (m_lengths.pds_supp > 0) {
        outs << "\n  Supplement to the PDS:";
        for (int n=0; n < m_lengths.pds_supp; ++n) {
          if (pds_supp[n] < 32 || pds_supp[n] > 127) {
            outs << " \\" << std::setw(3) << std::setfill('0') << std::oct << static_cast<int>(pds_supp[n]) << std::setfill(' ') << std::dec;
	  }
          else {
            outs << " " << pds_supp[n];
	  }
        }
    }
    (reinterpret_cast<TDLGRIBGrid *>(grids.back().get()))->print_header(outs,verbose);
  }
  else {
    outs << " Ed=" << m_edition;
    (reinterpret_cast<TDLGRIBGrid *>(grids.back().get()))->print_header(outs,verbose);
  }
}

TDLGRIBGrid& TDLGRIBGrid::operator=(const TDLGRIBGrid& source)
{
  if (this == &source) {
    return *this;
  }
  m_reference_date_time=source.m_reference_date_time;
  m_valid_date_time=source.m_valid_date_time;
  dim=source.dim;
  def=source.def;
  stats=source.stats;
  grid=source.grid;
  grib=source.grib;
  bitmap.applies=source.bitmap.applies;
  if (bitmap.capacity != source.bitmap.capacity) {
    if (bitmap.map != NULL) {
	delete[] bitmap.map;
	bitmap.map=NULL;
    }
    bitmap.capacity=source.bitmap.capacity;
    bitmap.map=new unsigned char[bitmap.capacity];
  }
  for (size_t n=0; n < bitmap.capacity; ++n) {
    bitmap.map[n]=source.bitmap.map[n];
  }
  tdl=source.tdl;
  return *this;
}

void TDLGRIBGrid::print(std::ostream& outs) const
{
  int i,j,k,max_i;
  bool scientific=false;

  if (grid.filled) {
    if (!floatutils::myequalf(stats.avg_val,0.) && fabs(stats.avg_val) < 0.01) {
	scientific=true;
    }
    switch (grid.grid_type) {
	case 5:
	{
	  for (i=0; i < dim.x; i+=16) {
	    max_i=i+16;
	    if (max_i > dim.x) max_i=dim.x;
	    outs << "\n   \\ X";
	    for (k=i; k < max_i; ++k) {
		outs << std::setw(9) << k+1;
	    }
	    outs << "\n Y  +-";
	    for (k=i; k < max_i; ++k) {
		outs << "---------";
	    }
	    outs << std::endl;
	    for (j=dim.y-1; j >= 0; --j) {
		outs << std::setw(3) << j+1 << " | ";
		for (k=i; k < max_i; ++k) {
		  if (floatutils::myequalf(m_gridpoints[j][k],Grid::MISSING_VALUE)) {
		    outs << "         ";
		  }
		  else {
		    if (scientific) {
			outs.unsetf(std::ios::fixed);
			outs.setf(std::ios::scientific);
			outs << std::setw(9) << m_gridpoints[j][k];
			outs.unsetf(std::ios::scientific);
			outs.setf(std::ios::fixed);
		    }
		    else {
			outs << std::setw(9) << m_gridpoints[j][k];
		    }
		  }
		}
		outs << std::endl;
	    }
	  }
	  break;
	}
    }
    outs << std::endl;
  }
}

void TDLGRIBGrid::print(std::ostream& outs,float start_latitude,float end_latitude,float start_longitude,float end_longitude,float lat_increment,float lon_increment,bool full_longitude_strip) const
{
}

void TDLGRIBGrid::print_ascii(std::ostream& outs) const
{
}

void TDLGRIBGrid::v_print_header(std::ostream& outs,bool verbose,std::string path_to_parameter_map) const
{
/*
  static const char *units[]={
  "MINUTES"  ,"HOURS"    ,"DAYS"     ,"MONTHS"   ,"YEARS"    ,"DECADES"  ,
  "NORMALS"  ,"CENTURIES","3 HOURS"  ,"6 HOURS"  ,"12 HOURS" };
*/
  outs.setf(std::ios::fixed);
  outs.precision(2);
  bool scientific=false;
  if (!floatutils::myequalf(stats.avg_val,0.) && fabs(stats.avg_val) < 0.01) {
    scientific=true;
  }
  if (verbose) {
    outs << "  Model: " << std::setw(3) << grib.process << "  Sequence: " << std::setw(3) << tdl.seq << "  RefTime: " << m_reference_date_time.to_string() << "  ValidTime: " << m_valid_date_time.to_string() << "  NumAvg: " << std::setw(3) << grid.nmean << "  Param: " << std::setw(6) << tdl.param;
    if (floatutils::myequalf(grid.level2,0.)) {
	outs << "  Level: " << std::setw(4) << grid.level1;
    }
    else {
	outs << "  Layer- Top: " << std::setw(4) << grid.level1 << " Bottom: " << std::setw(4) << grid.level2;
    }
    switch (grid.grid_type) {
// Polar Stereographic
	case 5:
	{
	  outs << "\n  Grid: " << std::setw(3) << dim.x << " x " << std::setw(3) << dim.y << "  Type: " << std::setw(3) << grid.grid_type;
	  outs.precision(4);
	  outs << "  Lat1: " << std::setw(8) << def.slatitude << " Lon1: " << std::setw(8) << def.slongitude << " Orient: " << std::setw(8) << def.olongitude << "  Dx: " << std::setw(8) << def.dx << " Dy: " << std::setw(8) << def.dy << " at " << std::setw(8) << def.llatitude;
	  outs.precision(2);
	  break;
	}
    }
    if (grid.filled) {
	outs << std::endl;
	if (scientific) {
	  outs.unsetf(std::ios::fixed);
	  outs.setf(std::ios::scientific);
	  outs.precision(3);
	  outs << "  MinVal: " << stats.min_val << " (" << stats.min_i << "," << stats.min_j << ")  MaxVal: " << stats.max_val << " (" << stats.max_i << "," << stats.max_j << ")  AvgVal: " << stats.avg_val;
	  outs.unsetf(std::ios::scientific);
	  outs.setf(std::ios::fixed);
	  outs.precision(2);
	}
	else
	  outs << "  MinVal: " << std::setw(9) << stats.min_val << " (" << stats.min_i << "," << stats.min_j << ")  MaxVal: " << std::setw(9) << stats.max_val << " (" << stats.max_i << "," << stats.max_j << ")  AvgVal: " << std::setw(9) << stats.avg_val;
    }
    outs << std::endl;
  }
  else {
    outs << " Model=" << grib.process << " Seq=" << tdl.seq << " RefTime=" << m_reference_date_time.to_string("%Y%m%d%H%M") << " NAvg=" << grid.nmean << " ValidTime=" << m_valid_date_time.to_string("%Y%m%d%H%M") << " Param=" << std::setfill('0') << std::setw(6) << tdl.param << std::setfill(' ');
    if (floatutils::myequalf(grid.level2,0.)) {
	outs << " Level=" << grid.level1;
    }
    else {
	outs << " Layer=" << grid.level1 << "," << grid.level2;
    }
    if (scientific) {
	outs.unsetf(std::ios::fixed);
	outs.setf(std::ios::scientific);
	outs.precision(3);
	outs << " Min=" << stats.min_val;
	if (grid.filled) {
	  outs << " Max=" << stats.max_val << " Avg=" << stats.avg_val;
	}
	outs << std::endl;
	outs.unsetf(std::ios::scientific);
	outs.setf(std::ios::fixed);
	outs.precision(2);
    }
    else {
	outs << " Min=" << stats.min_val;
	if (grid.filled) {
	  outs << " Max=" << stats.max_val << " Avg=" << stats.avg_val;
	}
	outs << std::endl;
    }
  }
}

size_t TDLGRIBGrid::copy_to_buffer(unsigned char *output_buffer,const size_t buffer_length) const
{
return 0;
}
