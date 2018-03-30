#include <iomanip>
#include <bufr.hpp>
#include <strutils.hpp>
#include <bits.hpp>

void TD5900WindsBUFRData::print() const
{
  std::cout.setf(std::ios::fixed);
  std::cout.precision(2);
  std::cout << "OBS TIME: " << datetime.to_string("%Y-%m-%d %H:%MM") << " TIME SIG: " << time_sig << " AVG: " << avg_period << "  ID: " << wmoid << " (" << stn_shortname << ")  LAT: " << lat << " LON: " << lon << " ELEV: " << elev << "  NLEV: " << hgt.length() << "  EQUIP TYPE: " << equip_type << "  FREQ: " << round(freq/1000000.) << "MHz" << std::endl;
  if (!filled)
    return;
  std::cout << "    LEV  Q    HGT   DD    FF  SDEV     W  SDEV" << std::endl;
  std::cout.precision(1);
  for (size_t n=0; n < hgt.length(); ++n) {
    std::cout << std::setw(7) << n+1 << std::setw(3) << static_cast<int>(qual(n)) << std::setw(7) << hgt(n) << std::setw(5) << dd(n) << std::setw(6) << ff(n) << std::setw(6) << ff_stddev(n) << std::setw(6) << w(n) << std::setw(6) << w_stddev(n) << std::endl;
  }
}

void handle_td5900_winds_bufr(BUFR::Descriptor& descriptor_to_handle,const unsigned char *input_buffer,const InputBUFRStream::TableBEntry& tbentry,size_t offset,BUFRData **bufr_data,int subset_number,short num_subsets,bool fill_header_only)
{
  TD5900WindsBUFRData *bd=reinterpret_cast<TD5900WindsBUFRData *>(bufr_data);
  long long packed=0;
  char cpacked[256];

  switch (descriptor_to_handle.x) {
    case 1:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->wmoid=strutils::ftos(packed,2,0,'0');
	    return;
	  }
	  case 2:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->wmoid+=strutils::ftos(packed,3,0,'0');
	    return;
	  }
	  case 18:
	  {
	    bits::get(input_buffer,cpacked,offset,8,0,tbentry.width/8);
	    bd->stn_shortname.assign(cpacked,tbentry.width/8);
	    strutils::trim(bd->stn_shortname);
	    return;
	  }
	}
	break;
    }
    case 2:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->stn_type=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  }
	  case 3:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->equip_type=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  }
	  case 121:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->freq=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  }
	}
	break;
    }
    case 4:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->datetime.set_year(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    return;
	  }
	  case 2:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->datetime.set_month(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    return;
	  }
	  case 3:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->datetime.set_day(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    return;
	  }
	  case 4:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->datetime.set_time(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val)*10000);
	    return;
	  }
	  case 5:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->datetime.set_time(bd->datetime.time()+bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val)*100);
	    return;
	  }
	  case 26:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->avg_period=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  }
	}
	break;
    }
    case 5:
    {
	switch (descriptor_to_handle.y) {
	  case 2:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->lat=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  }
	}
	break;
    }
    case 6:
    {
	switch (descriptor_to_handle.y) {
	  case 2:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->lon=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  }
	}
	break;
    }
    case 7:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->elev=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  }
	  case 6:
	  {
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		(bd->hgt)[bd->hgt.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		(bd->dd)[bd->hgt.length()-1]=-9999;
		(bd->ff)[bd->hgt.length()-1]=-999.9;
	    }
	    return;
	  }
	}
	break;
    }
    case 8:
    {
	switch (descriptor_to_handle.y) {
	  case 21:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->time_sig=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  }
	}
	break;
    }
    case 11:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		(bd->dd)[bd->hgt.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    return;
	  }
	  case 2:
	  {
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		(bd->ff)[bd->hgt.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		if ((bd->ff)[bd->hgt.length()-1] < -9999.) {
		  (bd->ff)[bd->hgt.length()-1]=-99.;
		}
	    }
	    return;
	  }
	  case 6:
	  {
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		(bd->w)[bd->hgt.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		if ((bd->w)[bd->hgt.length()-1] < -9999.) {
		  (bd->w)[bd->hgt.length()-1]=-99.;
		}
	    }
	    return;
	  }
	  case 50:
	  {
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		(bd->ff_stddev)[bd->hgt.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		if ((bd->ff_stddev)[bd->hgt.length()-1] < -9999.) {
		  (bd->ff_stddev)[bd->hgt.length()-1]=-99.;
		}
	    }
	    return;
	  }
	  case 51:
	  {
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		(bd->w_stddev)[bd->hgt.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		if ((bd->w_stddev)[bd->hgt.length()-1] < -9999.) {
		  (bd->w_stddev)[bd->hgt.length()-1]=-99.;
		}
	    }
	    return;
	  }
	}
	break;
    }
    case 25:
    {
	switch (descriptor_to_handle.y) {
	  case 34:
	  {
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		(bd->qual)[bd->hgt.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    return;
	  }
	}
	break;
    }
  }
}

void TD5900SurfaceBUFRData::print() const
{
  std::cout.setf(std::ios::fixed);
  std::cout.precision(2);
  std::cout << "OBS TIME: " << datetime.to_string("%Y-%m-%d %H:%MM") << " TIME SIG: " << time_sig << " AVG: " << avg_period << "  ID: " << wmoid << " (" << stn_shortname << ")  LAT: " << lat << " LON: " << lon << " ELEV: " << elev << std::endl;
  if (!filled)
    return;
  std::cout.precision(1);
  std::cout << "    STNP: " << std::setw(6) << stnp << "  SLP: " << std::setw(6) << slp << "  TEMP: " << std::setw(5) << temp << "  RH: " << std::setw(3) << rh << "  WIND: " << std::setw(3) << dd << "/" << std::setw(5) << ff;
  std::cout.precision(4);
  std::cout << "  RAIN RATE: " << std::setw(7) << rain_rate << std::endl;
  std::cout.precision(1);
}

void handle_td5900_surface_bufr(BUFR::Descriptor& descriptor_to_handle,const unsigned char *input_buffer,const InputBUFRStream::TableBEntry& tbentry,size_t offset,BUFRData **bufr_data,int subset_number,short num_subsets,bool fill_header_only)
{
  TD5900SurfaceBUFRData *bd=reinterpret_cast<TD5900SurfaceBUFRData *>(bufr_data);
  long long packed=0;
  char cpacked[256];

  switch (descriptor_to_handle.x) {
    case 1:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->wmoid=strutils::ftos(packed,2,0,'0');
	    return;
	  }
	  case 2:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->wmoid+=strutils::ftos(packed,3,0,'0');
	    return;
	  }
	  case 18:
	  {
	    bits::get(input_buffer,cpacked,offset,8,0,tbentry.width/8);
	    bd->stn_shortname.assign(cpacked,tbentry.width/8);
	    strutils::trim(bd->stn_shortname);
	    return;
	  }
	}
	break;
    }
    case 2:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->stn_type=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  }
	}
	break;
    }
    case 4:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->datetime.set_year(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    return;
	  }
	  case 2:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->datetime.set_month(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    return;
	  }
	  case 3:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->datetime.set_day(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    return;
	  }
	  case 4:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->datetime.set_time(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val)*10000);
	    return;
	  }
	  case 5:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->datetime.set_time(bd->datetime.time()+bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val)*100);
	    return;
	  }
	  case 26:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->avg_period=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  }
	}
	break;
    }
    case 5:
    {
	switch (descriptor_to_handle.y) {
	  case 2:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->lat=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  }
	}
	break;
    }
    case 6:
    {
	switch (descriptor_to_handle.y) {
	  case 2:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->lon=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  }
	}
	break;
    }
    case 7:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->elev=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  }
	}
	break;
    }
    case 8:
    {
	switch (descriptor_to_handle.y) {
	  case 21:
	  {
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->time_sig=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  }
	}
	break;
    }
    case 10:
    {
	switch (descriptor_to_handle.y) {
	  case 4:
	  {
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd->stnp=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    return;
	  }
	  case 51:
	  {
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd->slp=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    return;
	  }
	}
	break;
    }
    case 11:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd->dd=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    return;
	  }
	  case 2:
	  {
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd->ff=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    return;
	  }
	}
	break;
    }
    case 12:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd->temp=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    return;
	  }
	}
	break;
    }
    case 13:
    {
	switch (descriptor_to_handle.y) {
	  case 3:
	  {
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd->rh=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    return;
	  }
	  case 14:
	  {
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd->rain_rate=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    return;
	  }
	}
	break;
    }
  }
}

void TD5900MomentBUFRData::print() const
{
  size_t n,m;

  std::cout.setf(std::ios::fixed);
  std::cout.precision(2);
  std::cout << "OBS TIME: " << datetime.to_string("%Y-%m-%d %H:%MM") << " TIME SIG: " << time_sig << " AVG: " << avg_period << "  ID: " << wmoid << " (" << stn_shortname << ")  LAT: " << lat << " LON: " << lon << " ELEV: " << elev << "  NBEAMS: " << azimuth.length() << "  EQUIP TYPE: " << equip_type << "  FREQ: " << round(freq/1000000.) << "MHz" << std::endl;
  if (!filled)
    return;
  for (n=0; n < azimuth.length(); n++) {
    std::cout << "  BEAM#: " << std::setw(3) << n+1 << "  AZIMUTH: " << azimuth(n) << "  ELEVATION: " << elevation(n) << "  #SAMPLES: " << beam_data(n).hgt.length() << std::endl;
    std::cout << "    SAMPLE#    HGT  Q  NAVG  0thMOM  SNR  RAD_VEL  SPEC_WIDTH" << std::endl;
    for (m=0; m < beam_data(n).hgt.length(); m++)
	std::cout << std::setw(11) << m+1 << std::setw(7) << beam_data(n).hgt(m) << std::setw(3) << static_cast<int>(beam_data(n).qual(m)) << std::setw(6) << beam_data(n).navg(m) << std::setw(8) << beam_data(n).mom_0(m) << std::setw(5) << beam_data(n).snr(m) << std::setw(9) << beam_data(n).rad_vel(m) << std::setw(12) << beam_data(n).spec_width(m) << std::endl;
  }
  std::cout.precision(1);
}

void handle_td5900_moment_bufr(BUFR::Descriptor& descriptor_to_handle,const unsigned char *input_buffer,const InputBUFRStream::TableBEntry& tbentry,size_t offset,BUFRData **bufr_data,int subset_number,short num_subsets,bool fill_header_only)
{
  TD5900MomentBUFRData *bd=reinterpret_cast<TD5900MomentBUFRData *>(bufr_data);
  long long packed=0;
  char cpacked[256];

  switch (descriptor_to_handle.x) {
    case 1:
	switch (descriptor_to_handle.y) {
	  case 1:
          bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->wmoid=strutils::ftos(packed,2,0,'0');
	    return;
	  case 2:
          bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->wmoid+=strutils::ftos(packed,3,0,'0');
	    return;
	  case 18:
	    bits::get(input_buffer,cpacked,offset,8,0,tbentry.width/8);
	    bd->stn_shortname.assign(cpacked,tbentry.width/8);
	    strutils::trim(bd->stn_shortname);
	    return;
	}
	break;
    case 2:
	switch (descriptor_to_handle.y) {
	  case 1:
          bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->stn_type=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  case 3:
          bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->equip_type=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  case 121:
          bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->freq=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  case 134:
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		(bd->azimuth)[bd->azimuth.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		(bd->elevation)[bd->azimuth.length()-1]=-9999;
	    }
	    return;
	  case 135:
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		(bd->elevation)[bd->elevation.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    return;
	}
	break;
    case 4:
	switch (descriptor_to_handle.y) {
	  case 1:
          bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->datetime.set_year(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    return;
	  case 2:
          bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->datetime.set_month(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    return;
	  case 3:
          bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->datetime.set_day(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    return;
	  case 4:
          bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->datetime.set_time(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val)*10000);
	    return;
	  case 5:
          bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->datetime.set_time(bd->datetime.time()+bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val)*100);
	    return;
	  case 26:
          bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->avg_period=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	}
	break;
    case 5:
	switch (descriptor_to_handle.y) {
	  case 2:
          bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->lat=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	}
	break;
    case 6:
	switch (descriptor_to_handle.y) {
	  case 2:
          bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->lon=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	}
	break;
    case 7:
	switch (descriptor_to_handle.y) {
	  case 1:
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->elev=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  case 6:
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		(bd->beam_data)[bd->azimuth.length()-1].hgt[(bd->beam_data)[bd->azimuth.length()-1].hgt.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		(bd->beam_data)[bd->azimuth.length()-1].rad_vel[(bd->beam_data)[bd->azimuth.length()-1].hgt.length()-1]=-99.;
		(bd->beam_data)[bd->azimuth.length()-1].spec_width[(bd->beam_data)[bd->azimuth.length()-1].hgt.length()-1]=-99.;
		(bd->beam_data)[bd->azimuth.length()-1].mom_0[(bd->beam_data)[bd->azimuth.length()-1].hgt.length()-1]=-99;
		(bd->beam_data)[bd->azimuth.length()-1].snr[(bd->beam_data)[bd->azimuth.length()-1].hgt.length()-1]=-99;
	    }
	    return;
	}
	break;
    case 8:
	switch (descriptor_to_handle.y) {
	  case 21:
	    bits::get(input_buffer,packed,offset,tbentry.width);
	    bd->time_sig=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    return;
	  case 22:
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		(bd->beam_data)[bd->azimuth.length()-1].navg[(bd->beam_data)[bd->azimuth.length()-1].hgt.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);;
	    }
	    return;
	}
	break;
    case 21:
	switch (descriptor_to_handle.y) {
	  case 14:
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		(bd->beam_data)[bd->azimuth.length()-1].rad_vel[(bd->beam_data)[bd->azimuth.length()-1].hgt.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    return;
	  case 17:
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		(bd->beam_data)[bd->azimuth.length()-1].spec_width[(bd->beam_data)[bd->azimuth.length()-1].hgt.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    return;
	  case 30:
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		if (packed < -9999)
		  packed=-99;
		(bd->beam_data)[bd->azimuth.length()-1].snr[(bd->beam_data)[bd->azimuth.length()-1].hgt.length()-1]=packed;
	    }
	    return;
	  case 91:
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		(bd->beam_data)[bd->azimuth.length()-1].mom_0[(bd->beam_data)[bd->azimuth.length()-1].hgt.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    return;
	}
	break;
    case 25:
	switch (descriptor_to_handle.y) {
	  case 34:
	    if (!fill_header_only) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		(bd->beam_data)[bd->azimuth.length()-1].qual[(bd->beam_data)[bd->azimuth.length()-1].hgt.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    return;
	}
	break;
  }
}
