#include <iomanip>
#include <regex>
#include <bufr.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>

size_t ADP_next_offset,ADP_off_adjust;

void NCEPADPBUFRData::print() const
{
  if (!filled) {
    return;
  }
  std::cout << "OBS TIME: " << datetime.to_string("%Y-%m-%d %H:%MM") << " ID: ";
  if (wmoid.length() > 0) {
    std::cout << wmoid << " (WMO)";
  }
  else if (acrn.length() > 0) {
    std::cout << acrn << " (ACFT)";
  }
  else if (acftid.length() > 0) {
    std::cout << acftid << " (FLT #)";
  }
  else if (satid.length() > 0) {
    std::cout << satid << " (SAT)";
  }
  else if (stsn.length() > 0) {
    std::cout << stsn << " (OTHER)";
  }
  else if (rpid.length() > 0) {
    std::cout << rpid << " (OTHER)";
  }
  else {
    std::cout << "??";
  }
  std::cout << " LAT: " << lat << " LON: " << lon << " ELEV: " << elev;
  if (multi_level_data.lev_type.length() > 0) {
    std::cout << " NLEV: " << multi_level_data.lev_type.length() << std::endl;
    std::cout << "    LEV    PRES    HEIGHT    GEOPOT    TEMP    DEWP      DD      FF TYPE" << std::endl;
    std::cout.setf(std::ios::fixed);
    std::cout.precision(2);
    for (size_t n=0; n < multi_level_data.lev_type.length(); ++n) {
	std::cout << std::setw(7) << n+1 << std::setw(8) << multi_level_data.pres(n) << std::setw(10) << multi_level_data.z(n) << std::setw(10) << multi_level_data.gp(n) << std::setw(8) << multi_level_data.t(n) << std::setw(8) << multi_level_data.dp(n) << std::setw(8) << multi_level_data.dd(n) << std::setw(8) << multi_level_data.ff(n) << std::setw(5) << static_cast<int>(multi_level_data.lev_type(n)) << std::endl;
    }
  }
  else {
    std::cout << std::endl;
    std::cout.setf(std::ios::fixed);
    std::cout.precision(1);
    std::cout << "     PRES: " << single_level_data.pres << "  ALT: " << single_level_data.z << "  PALT: " << single_level_data.palt << "  GEOPOT: " << single_level_data.gp << "  TEMP: " << single_level_data.t << "  DEWP: " << single_level_data.dp << "  RH: " << single_level_data.rh << "  DD: " << single_level_data.dd << "  FF: " << single_level_data.ff << "  PRCP (1/3/12/24): ";
    std::cout.precision(2);
    std::cout << single_level_data.prcp[0] << "/" << single_level_data.prcp[1] << "/" << single_level_data.prcp[2] << "/" << single_level_data.prcp[3];
    std::cout.precision(1);
    std::cout << std::endl;
  }
}

void handle_ncep_adpbufr(BUFR::Descriptor& descriptor_to_handle,const unsigned char *input_buffer,const InputBUFRStream::TableBEntry& tbentry,size_t offset,BUFRData **bufr_data,int subset_number,short num_subsets,bool fill_header_only)
{
  NCEPADPBUFRData **bd=reinterpret_cast<NCEPADPBUFRData **>(bufr_data);
  long long packed;
  double *parray,ddum;
  char cpacked[256];
  size_t n;

  if (subset_number > 0) {
    offset-=ADP_off_adjust;
  }
  switch (descriptor_to_handle.x) {
    case 1:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->wmoid=strutils::ftos(packed,2,0,'0');
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  bd[n]->wmoid=strutils::ftos(parray[n],2,0,'0');
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 2:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->wmoid+=strutils::ftos(packed,3,0,'0');
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  bd[n]->wmoid+=strutils::ftos(parray[n],3,0,'0');
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 6:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,cpacked,offset,8,0,tbentry.width/8);
		bd[subset_number]->acftid.assign(cpacked,tbentry.width/8);
		strutils::trim(bd[subset_number]->acftid);
	    }
	    return;
	  }
	  case 7:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->satid=strutils::itos(packed);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  bd[n]->satid=strutils::itos(parray[n]);
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 8:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,cpacked,offset,8,0,tbentry.width/8);
		bd[subset_number]->acrn.assign(cpacked,tbentry.width/8);
		strutils::trim(bd[subset_number]->acrn);
	    }
	    return;
	  }
	  case 15:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,cpacked,offset,8,0,tbentry.width/8);
		bd[subset_number]->stsn.assign(cpacked,tbentry.width/8);
		strutils::trim(bd[subset_number]->stsn);
	    }
	    return;
	  }
	  case 194:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,cpacked,offset,8,0,tbentry.width/8);
		bd[subset_number]->acftid.assign(cpacked,tbentry.width/8);
		strutils::trim(bd[subset_number]->acftid);
	    }
	    return;
	  }
	  case 198:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,cpacked,offset,8,0,tbentry.width/8);
		bd[subset_number]->rpid.assign(cpacked,tbentry.width/8);
		strutils::trim(bd[subset_number]->rpid);
	    }
	    return;
	  }
	  default:
	  {
	    if (descriptor_to_handle.y < 192) {
		return;
	    }
	  }
	}
	break;
    }
    case 4:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    if (subset_number >= 0) {
		if (bd[subset_number]->set_date) {
		  return;
		}
		if (!bd[subset_number]->found_raw) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  bd[subset_number]->datetime.set_year(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
//std::cerr << "year: " << bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val) << std::endl;
		}
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < static_cast<size_t>(num_subsets); n++) {
		  bd[n]->datetime.set_year(parray[n]);
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 2:
	  {
	    if (subset_number >= 0) {
		if (bd[subset_number]->set_date) {
		  return;
		}
		if (!bd[subset_number]->found_raw) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  bd[subset_number]->datetime.set_month(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	      }
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < static_cast<size_t>(num_subsets); n++) {
		  bd[n]->datetime.set_month(parray[n]);
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 3:
	  {
	    if (subset_number >= 0) {
		if (bd[subset_number]->set_date) {
		  return;
		}
		if (!bd[subset_number]->found_raw) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  bd[subset_number]->datetime.set_day(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
		}
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  bd[n]->datetime.set_day(parray[n]);
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 4:
	  {
	    if (subset_number >= 0) {
		if (bd[subset_number]->set_date) {
		  return;
		}
		if (!bd[subset_number]->found_raw) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  ddum=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		  if (ddum > -99998.) {
		    bd[subset_number]->datetime.set_time(ddum*10000);
		  }
	      }
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  if (parray[n] > -99998.) {
		    bd[n]->datetime.set_time(parray[n]*10000);
		    bd[n]->filled=true;
		  }
		}
	    }
	    return;
	  }
	  case 5:
	  {
	    if (subset_number >= 0) {
		if (bd[subset_number]->set_date) {
		  return;
		}
		if (!bd[subset_number]->found_raw) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  ddum=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		  if (ddum > -99998.) {
		    bd[subset_number]->datetime.set_time(bd[subset_number]->datetime.time()+ddum*100);
		  }
		}
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  if (parray[n] > -99998.) {
		    bd[n]->datetime.set_time(bd[n]->datetime.time()+parray[n]*100);
		    bd[n]->filled=true;
		  }
		}
	    }
	    return;
	  }
	  case 43:
	  {
	    if (subset_number >= 0) {
		if (bd[subset_number]->set_date) {
		  return;
		}
		if (!bd[subset_number]->found_raw) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  bd[subset_number]->datetime.add_days(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
		}
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  if (!bd[n]->set_date) {
		    bd[n]->datetime.add_days(parray[n]);
		    bd[n]->filled=true;
		  }
		}
	    }
	    return;
	  }
	  case 210:
	  {
	    if (subset_number >= 0) {
		if (!bd[subset_number]->found_raw) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  ddum=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		  if (ddum > -99998.)
		    bd[subset_number]->datetime.set_time(ddum*10000);
	      }
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  if (parray[n] > -99998.) {
		    bd[n]->datetime.set_time(parray[n]*10000);
		    bd[n]->filled=true;
		  }
		}
	    }
	    return;
	  }
	  case 211:
	  {
	    if (subset_number >= 0) {
		if (!bd[subset_number]->found_raw) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  ddum=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		  if (ddum > -99998.)
		    bd[subset_number]->datetime.set_time(bd[subset_number]->datetime.time()+ddum*100);
	      }
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  if (parray[n] > -99998.) {
		    bd[n]->datetime.set_time(bd[n]->datetime.time()+parray[n]*100);
		    bd[n]->filled=true;
		  }
		}
	    }
	    return;
	  }
	  default:
	  {
	    if (descriptor_to_handle.y < 192) {
		return;
	    }
	  }
	}
	break;
    }
    case 5:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->lat=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  bd[n]->lat=parray[n];
		  bd[n]->filled=true;
		}
	    }
//	    bd[subset_number]->set_date=true;
	    return;
	  }
	  case 2:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->lat=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  bd[n]->lat=parray[n];
		  bd[n]->filled=true;
		}
	    }
//	    bd[subset_number]->set_date=true;
	    return;
	  }
	  default:
	  {
	    if (descriptor_to_handle.y < 192) {
		return;
	    }
	  }
	}
	break;
    }
    case 6:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->lon=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  bd[n]->lon=parray[n];
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 2:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->lon=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  bd[n]->lon=parray[n];
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  default:
	  {
	    if (descriptor_to_handle.y < 192) {
		return;
	    }
	  }
	}
	break;
    }
    case 7:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->elev=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  bd[n]->elev=parray[n];
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 2:
	  {
	    if (!fill_header_only) {
		if (subset_number >= 0) {
		  if (bd[subset_number]->single_level_data.z < -9999.) {
		    bits::get(input_buffer,packed,offset,tbentry.width);
		    bd[subset_number]->single_level_data.z=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		  }
		}
		else {
		  unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		  for (int n=0; n < num_subsets; ++n) {
		    if (bd[n]->single_level_data.z < -9999.) {
			bd[n]->single_level_data.z=parray[n];
			bd[n]->filled=true;
		    }
		  }
		}
	    }
	    return;
	  }
	  case 4:
	  {
	    if (!fill_header_only) {
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  if (bd[subset_number]->multi_level_data.lev_type.length() > 0) {
		    (bd[subset_number]->multi_level_data.pres)[bd[subset_number]->multi_level_data.pres.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val)/100.;
		  }
		  else {
		    if (bd[subset_number]->single_level_data.pres < 0) {
			bd[subset_number]->single_level_data.pres=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
			if (bd[subset_number]->single_level_data.pres > 0) {
			  bd[subset_number]->single_level_data.pres/=100.;
			}
		    }
		  }
		}
		else {
		  unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		  for (int n=0; n < num_subsets; ++n) {
		    if (bd[n]->multi_level_data.lev_type.length() > 0) {
			(bd[n]->multi_level_data.pres)[bd[n]->multi_level_data.pres.length()-1]=parray[n]/100.;
			bd[n]->filled=true;
		    }
		    else {
			if (bd[n]->single_level_data.pres < 0) {
			  bd[n]->single_level_data.pres=parray[n];
			  if (bd[n]->single_level_data.pres > 0) {
			    bd[n]->single_level_data.pres/=100.;
			    bd[n]->filled=true;
			  }
			}
		    }
		  }
		}
	    }
	    return;
	  }
	  case 7:
	  {
	    if (!fill_header_only) {
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  if (bd[subset_number]->multi_level_data.lev_type.length() > 0)
		    (bd[subset_number]->multi_level_data.z)[bd[subset_number]->multi_level_data.z.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		  else {
		    if (bd[subset_number]->single_level_data.z < -9999.)
			bd[subset_number]->single_level_data.z=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		  }
		}
		else {
		  unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		  for (int n=0; n < num_subsets; ++n) {
		    if (bd[n]->multi_level_data.lev_type.length() > 0) {
			(bd[n]->multi_level_data.z)[bd[n]->multi_level_data.z.length()-1]=parray[n];
			bd[n]->filled=true;
		    }
		    else {
			if (bd[n]->single_level_data.z < -9999.) {
			  bd[n]->single_level_data.z=parray[n];
			  bd[n]->filled=true;
			}
		    }
		  }
		}
	    }
	    return;
	  }
	  case 8:
	  {
	    if (!fill_header_only) {
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  (bd[subset_number]->multi_level_data.gp)[bd[subset_number]->multi_level_data.gp.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		  if ((bd[subset_number]->multi_level_data.gp)[bd[subset_number]->multi_level_data.gp.length()-1] > -9999.)
		    (bd[subset_number]->multi_level_data.gp)[bd[subset_number]->multi_level_data.gp.length()-1]/=9.8;
		}
		else {
		  unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		  for (int n=0; n < num_subsets; ++n) {
		    (bd[n]->multi_level_data.gp)[bd[n]->multi_level_data.gp.length()-1]=parray[n];
		    if ((bd[n]->multi_level_data.gp)[bd[n]->multi_level_data.gp.length()-1] > -9999.) {
			(bd[n]->multi_level_data.gp)[bd[n]->multi_level_data.gp.length()-1]/=9.8;
		    }
		    bd[n]->filled=true;
		  }
		}
	    }
	    return;
	  }
	  case 196:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->single_level_data.palt=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  bd[n]->single_level_data.palt=parray[n];
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 197:
	  {
	    if (subset_number >= 0) {
		if (bd[subset_number]->single_level_data.z < -9999.) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  bd[subset_number]->single_level_data.z=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		}
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  bd[n]->single_level_data.z=parray[n];
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  default:
	  {
	    if (descriptor_to_handle.y < 192) {
		return;
	    }
	  }
	}
	break;
    }
    case 8:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    if (!fill_header_only) {
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  (bd[subset_number]->multi_level_data.lev_type)[bd[subset_number]->multi_level_data.lev_type.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		  if ((bd[subset_number]->multi_level_data.lev_type)[bd[subset_number]->multi_level_data.lev_type.length()-1] != static_cast<int>(pow(2.,tbentry.width)-1)) {
		    n=0;
		    while ((bd[subset_number]->multi_level_data.lev_type)[bd[subset_number]->multi_level_data.lev_type.length()-1] >= 2) {
			(bd[subset_number]->multi_level_data.lev_type)[bd[subset_number]->multi_level_data.lev_type.length()-1]/=2;
			++n;
		    }
		    (bd[subset_number]->multi_level_data.lev_type)[bd[subset_number]->multi_level_data.lev_type.length()-1]=tbentry.width-n;
		  }
		  (bd[subset_number]->multi_level_data.pres)[bd[subset_number]->multi_level_data.lev_type.length()-1]=-999.99;
		  (bd[subset_number]->multi_level_data.z)[bd[subset_number]->multi_level_data.lev_type.length()-1]=-99999.;
		  (bd[subset_number]->multi_level_data.gp)[bd[subset_number]->multi_level_data.lev_type.length()-1]=-99999.;
		  (bd[subset_number]->multi_level_data.t)[bd[subset_number]->multi_level_data.lev_type.length()-1]=-999.;
		  (bd[subset_number]->multi_level_data.dp)[bd[subset_number]->multi_level_data.lev_type.length()-1]=-999.;
		  (bd[subset_number]->multi_level_data.dd)[bd[subset_number]->multi_level_data.lev_type.length()-1]=-999.99;
		  (bd[subset_number]->multi_level_data.ff)[bd[subset_number]->multi_level_data.lev_type.length()-1]=-999.99;
		}
		else {
		  unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		  for (int n=0; n < num_subsets; ++n) {
		    (bd[n]->multi_level_data.lev_type)[bd[n]->multi_level_data.lev_type.length()]=parray[n];
		    if ((bd[n]->multi_level_data.lev_type)[bd[n]->multi_level_data.lev_type.length()-1] != static_cast<int>(pow(2.,tbentry.width)-1)) {
			auto m=0;
			while ((bd[n]->multi_level_data.lev_type)[bd[n]->multi_level_data.lev_type.length()-1] >= 2) {
			  (bd[n]->multi_level_data.lev_type)[bd[n]->multi_level_data.lev_type.length()-1]/=2;
			  ++m;
			}
			(bd[n]->multi_level_data.lev_type)[bd[n]->multi_level_data.lev_type.length()-1]=tbentry.width-m;
		    }
		    (bd[n]->multi_level_data.pres)[bd[n]->multi_level_data.lev_type.length()-1]=-999.99;
		    (bd[n]->multi_level_data.z)[bd[n]->multi_level_data.lev_type.length()-1]=-99999.;
		    (bd[n]->multi_level_data.gp)[bd[n]->multi_level_data.lev_type.length()-1]=-99999.;
		    (bd[n]->multi_level_data.t)[bd[n]->multi_level_data.lev_type.length()-1]=-999.;
		    (bd[n]->multi_level_data.dp)[bd[n]->multi_level_data.lev_type.length()-1]=-999.;
		    (bd[n]->multi_level_data.dd)[bd[n]->multi_level_data.lev_type.length()-1]=-999.99;
		    (bd[n]->multi_level_data.ff)[bd[n]->multi_level_data.lev_type.length()-1]=-999.99;
		  }
		}
	    }
	    return;
	  }
	  default:
	  {
	    if (descriptor_to_handle.y < 192) {
		return;
	    }
	  }
	}
	break;
    }
    case 10:
    {
	switch (descriptor_to_handle.y) {
	  case 8:
	  {
	    if (!fill_header_only) {
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  if (bd[subset_number]->multi_level_data.lev_type.length() > 0) {
		    (bd[subset_number]->multi_level_data.gp)[bd[subset_number]->multi_level_data.gp.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		    if ((bd[subset_number]->multi_level_data.gp)[bd[subset_number]->multi_level_data.gp.length()-1] > -9999.) {
			(bd[subset_number]->multi_level_data.gp)[bd[subset_number]->multi_level_data.gp.length()-1]/=9.8;
		    }
		  }
		  else {
		    bd[subset_number]->single_level_data.gp=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		    if (bd[subset_number]->single_level_data.gp > -9999.) {
			bd[subset_number]->single_level_data.gp/=9.8;
		    }
		  }
		}
		else {
		  unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		  for (int n=0; n < num_subsets; ++n) {
		    if (bd[n]->multi_level_data.lev_type.length() > 0) {
			(bd[n]->multi_level_data.gp)[bd[n]->multi_level_data.gp.length()-1]=parray[n];
			if ((bd[n]->multi_level_data.gp)[bd[n]->multi_level_data.gp.length()-1] > -9999.) {
			  (bd[n]->multi_level_data.gp)[bd[n]->multi_level_data.gp.length()-1]/=9.8;
			}
			bd[n]->filled=true;
		    }
		    else {
			bd[n]->single_level_data.gp=parray[n];
			if (bd[n]->single_level_data.gp > -9999.) {
			  bd[n]->single_level_data.gp/=9.8;
			}
			bd[n]->filled=true;
		    }
		  }
		}
	    }
	    return;
	  }
	  case 70:
	  {
	    if (subset_number >= 0) {
		if (bd[subset_number]->single_level_data.z < -9999.) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  bd[subset_number]->single_level_data.z=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		}
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  if (bd[n]->single_level_data.z < -9999.) {
		    bd[n]->single_level_data.z=parray[n];
		    bd[n]->filled=true;
		  }
		}
	    }
	    return;
	  }
	  default:
	  {
	    if (descriptor_to_handle.y < 192) {
		return;
	    }
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
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  if (bd[subset_number]->multi_level_data.lev_type.length() > 0) {
		    (bd[subset_number]->multi_level_data.dd)[bd[subset_number]->multi_level_data.dd.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		    if ((bd[subset_number]->multi_level_data.dd)[bd[subset_number]->multi_level_data.dd.length()-1] < 0.)
			(bd[subset_number]->multi_level_data.dd)[bd[subset_number]->multi_level_data.dd.length()-1]/=100.;
		  }
		  else
		    bd[subset_number]->single_level_data.dd=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		}
	    }
	    return;
	  }
	  case 2:
	  {
	    if (!fill_header_only) {
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  if (bd[subset_number]->multi_level_data.lev_type.length() > 0) {
		    (bd[subset_number]->multi_level_data.ff)[bd[subset_number]->multi_level_data.ff.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		    if ((bd[subset_number]->multi_level_data.ff)[bd[subset_number]->multi_level_data.ff.length()-1] < 0.)
			(bd[subset_number]->multi_level_data.ff)[bd[subset_number]->multi_level_data.ff.length()-1]/=100.;
		  }
		  else
		    bd[subset_number]->single_level_data.ff=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		}
	    }
	    return;
	  }
	  default:
	  {
	    if (descriptor_to_handle.y < 192) {
		return;
	    }
	  }
	}
	break;
    }
    case 12:
    {
	switch (descriptor_to_handle.y) {
	  case 101:
	  {
	    if (!fill_header_only) {
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  if (bd[subset_number]->multi_level_data.lev_type.length() > 0)
		    (bd[subset_number]->multi_level_data.t)[bd[subset_number]->multi_level_data.t.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		  else
		    bd[subset_number]->single_level_data.t=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		}
	    }
	    return;
	  }
	  case 103:
	  {
	    if (!fill_header_only) {
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  if (bd[subset_number]->multi_level_data.lev_type.length() > 0)
		    (bd[subset_number]->multi_level_data.dp)[bd[subset_number]->multi_level_data.dp.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		  else
		    bd[subset_number]->single_level_data.dp=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		}
	    }
	    return;
	  }
	  default:
	  {
	    if (descriptor_to_handle.y < 192) {
		return;
	    }
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
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  bd[subset_number]->single_level_data.rh=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		}
	    }
	    return;
	  }
	  case 19:
	  {
	    if (!fill_header_only) {
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  bd[subset_number]->single_level_data.prcp[0]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		}
	    }
	    return;
	  }
	  case 20:
	  {
	    if (!fill_header_only) {
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  bd[subset_number]->single_level_data.prcp[1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		}
	    }
	    return;
	  }
	  case 22:
	  {
	    if (!fill_header_only) {
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  bd[subset_number]->single_level_data.prcp[2]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		}
	    }
	    return;
	  }
	  case 23:
	  {
	    if (!fill_header_only) {
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  bd[subset_number]->single_level_data.prcp[3]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		}
	    }
	    return;
	  }
	  default:
	  {
	    if (descriptor_to_handle.y < 192) {
		return;
	    }
	  }
	}
	break;
    }
    case 58:
    {
	switch (descriptor_to_handle.y) {
	  case 8:
	  {
	    if (subset_number >= 0) {
		bd[subset_number]->found_raw=true;
	    }
	    return;
	  }
	  default:
	  {
	    if (descriptor_to_handle.y < 192) {
		return;
	    }
	  }
	}
	break;
    }
    case 63:
    {
	switch (descriptor_to_handle.y) {
	  case 0:
	  {
	    if (subset_number == 0) {
		ADP_next_offset=0;
		ADP_off_adjust=0;
	    }
	    if (ADP_next_offset == 0) {
		ADP_next_offset=offset;
	    }
	    offset+=ADP_off_adjust;
	    ADP_off_adjust=offset-ADP_next_offset;
	    bits::get(input_buffer,packed,offset-ADP_off_adjust,tbentry.width);
	    ADP_next_offset+=static_cast<size_t>(packed*8);
	    return;
	  }
	  default:
	  {
	    if (descriptor_to_handle.y < 192) {
		return;
	    }
	  }
	}
	break;
    }
    default:
    {
	if (descriptor_to_handle.x < 48) {
	  return;
	}
    }
  }
  if (std::regex_search(tbentry.element_name,std::regex("^ACRN"))) {
    if (subset_number >= 0) {
	bits::get(input_buffer,cpacked,offset,8,0,tbentry.width/8);
	bd[subset_number]->acrn.assign(cpacked,tbentry.width/8);
	strutils::trim(bd[subset_number]->acrn);
    }
    return;
  }
  else if (std::regex_search(tbentry.element_name,std::regex("^ACID"))) {
    if (subset_number >= 0) {
	bits::get(input_buffer,cpacked,offset,8,0,tbentry.width/8);
	bd[subset_number]->acftid.assign(cpacked,tbentry.width/8);
	strutils::trim(bd[subset_number]->acftid);
    }
    return;
  }
  else if (std::regex_search(tbentry.element_name,std::regex("^SAID"))) {
    if (subset_number >= 0) {
	bits::get(input_buffer,packed,offset,tbentry.width);
	bd[subset_number]->satid=strutils::itos(packed);
    }
    return;
  }
  else if (std::regex_search(tbentry.element_name,std::regex("^RPID"))) {
    if (subset_number >= 0) {
	bits::get(input_buffer,cpacked,offset,8,0,tbentry.width/8);
	bd[subset_number]->rpid.assign(cpacked,tbentry.width/8);
	strutils::trim(bd[subset_number]->rpid);
    }
    return;
  }
  else if (std::regex_search(tbentry.element_name,std::regex("^STSN"))) {
    if (subset_number >= 0) {
	bits::get(input_buffer,cpacked,offset,8,0,tbentry.width/8);
	bd[subset_number]->stsn.assign(cpacked,tbentry.width/8);
	strutils::trim(bd[subset_number]->stsn);
    }
    return;
  }
  else if (std::regex_search(tbentry.element_name,std::regex("^IALT"))) {
    if (subset_number >= 0) {
	if (bd[subset_number]->single_level_data.z < -9999.) {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->single_level_data.z=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	}
    }
    return;
  }
  else if (std::regex_search(tbentry.element_name,std::regex("^PSAL"))) {
    if (subset_number >= 0) {
	bits::get(input_buffer,packed,offset,tbentry.width);
	bd[subset_number]->single_level_data.palt=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
    }
    return;
  }
  else if (std::regex_search(tbentry.element_name,std::regex("^FLVL"))) {
    if (subset_number >= 0) {
	if (bd[subset_number]->single_level_data.z < -9999.) {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->single_level_data.z=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	}
    }
    return;
  }
  descriptor_to_handle.ignore=true;
}

void NCEPPREPBUFRData::print() const
{
  if (!filled) {
    return;
  }
  std::cout << "Station ID: " << stnid;
  if (acftid.length() > 0) {
    std::cout << "  ACID: " << acftid;
  }
  if (satid.length() > 0) {
    std::cout << "  SAID: " << satid;
  }
  std::cout << "  Location: " << lat << " " << elon << " " << elev << "  Hour: " << hr << "(" << dhr << ")  Instrument: " << type.instr << "  Report: " << type.prepbufr << "/" << type.ON29 << "  Number of ON29 Data Categories: " << cat_groups.size() << std::endl;
  std::cout.setf(std::ios::fixed);
  std::cout.precision(1);
  for (size_t n=0; n < cat_groups.size(); ++n) {
    std::cout << n+1 << " - ON29 Data Category: " << cat_groups[n].code << "  Number of Levels: " << cat_groups[n].pres.length() << std::endl;
    for (size_t m=0; m < cat_groups[n].pres.length(); ++m) {
	std::cout << "    LEV   PRES      HGT   TEMP     DP       Q    DD    FF      U      V" << std::endl;
    }
  }
}

void handle_ncep_prepbufr(BUFR::Descriptor& descriptor_to_handle,const unsigned char *input_buffer,const InputBUFRStream::TableBEntry& tbentry,size_t offset,BUFRData **bufr_data,int subset_number,short num_subsets,bool fill_header_only)
{
  long long packed;
  char cpacked[8];
  NCEPPREPBUFRData **bd=reinterpret_cast<NCEPPREPBUFRData **>(bufr_data);
  double val;
  static my::map<BUFR::StringEntry> *element_name_table=nullptr;
  BUFR::StringEntry se;
  size_t idx;

  if (element_name_table == nullptr) {
    element_name_table=new my::map<BUFR::StringEntry>;
    se.key="SID";
    element_name_table->insert(se);
    se.key="ACID";
    element_name_table->insert(se);
    se.key="SAID";
    element_name_table->insert(se);
    se.key="XOB";
    element_name_table->insert(se);
    se.key="YOB";
    element_name_table->insert(se);
    se.key="DHR";
    element_name_table->insert(se);
    se.key="ELV";
    element_name_table->insert(se);
    se.key="TYP";
    element_name_table->insert(se);
    se.key="T29";
    element_name_table->insert(se);
    se.key="ITP";
    element_name_table->insert(se);
    se.key="SQN";
    element_name_table->insert(se);
    se.key="CAT";
    element_name_table->insert(se);
    se.key="RPT";
    element_name_table->insert(se);
    se.key="POB";
    element_name_table->insert(se);
    se.key="PFC";
    element_name_table->insert(se);
    se.key="POE";
    element_name_table->insert(se);
    se.key="PAN";
    element_name_table->insert(se);
    se.key="ZOB";
    element_name_table->insert(se);
    se.key="ZFC";
    element_name_table->insert(se);
    se.key="ZAN";
    element_name_table->insert(se);
    se.key="ZOE";
    element_name_table->insert(se);
    se.key="TOB";
    element_name_table->insert(se);
    se.key="TFC";
    element_name_table->insert(se);
    se.key="TAN";
    element_name_table->insert(se);
    se.key="TOE";
    element_name_table->insert(se);
    se.key="QOB";
    element_name_table->insert(se);
    se.key="QFC";
    element_name_table->insert(se);
    se.key="QAN";
    element_name_table->insert(se);
    se.key="QOE";
    element_name_table->insert(se);
    se.key="DDO";
    element_name_table->insert(se);
    se.key="FFO";
    element_name_table->insert(se);
    se.key="UOB";
    element_name_table->insert(se);
    se.key="VOB";
    element_name_table->insert(se);
    se.key="UFC";
    element_name_table->insert(se);
    se.key="VFC";
    element_name_table->insert(se);
    se.key="UAN";
    element_name_table->insert(se);
    se.key="VAN";
    element_name_table->insert(se);
  }
  if ( (idx=tbentry.element_name.find(" ")) != std::string::npos) {
    se.key=tbentry.element_name.substr(0,idx);
  }
  if (element_name_table->found(se.key,se)) {
//std::cerr << "H " << static_cast<int>(descriptor_to_handle.f) << " " << static_cast<int>(descriptor_to_handle.x) << " " << static_cast<int>(descriptor_to_handle.y) << " " << offset << " " << subset_number << std::endl;
    if (se.key == "SID") {
	bits::get(input_buffer,cpacked,offset,8,0,tbentry.width/8);
	bd[subset_number]->stnid.assign(cpacked,tbentry.width/8);
	return;
    }
    else if (se.key == "ACID") {
	bits::get(input_buffer,cpacked,offset,8,0,tbentry.width/8);
	bd[subset_number]->acftid.assign(cpacked,tbentry.width/8);
	return;
    }
    else if (se.key == "SAID") {
	bits::get(input_buffer,packed,offset,tbentry.width);
	bd[subset_number]->satid=strutils::itos(packed);
	return;
    }
    else if (se.key == "XOB") {
	bits::get(input_buffer,packed,offset,tbentry.width);
	bd[subset_number]->elon=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	return;
    }
    else if (se.key == "YOB") {
	bits::get(input_buffer,packed,offset,tbentry.width);
	bd[subset_number]->lat=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	return;
    }
    else if (se.key == "DHR") {
	bits::get(input_buffer,packed,offset,tbentry.width);
	val=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	if (val < -999.) {
	  bd[subset_number]->dhr=-999;
	}
	else {
	  bd[subset_number]->dhr=val;
	}
	return;
    }
    else if (se.key == "ELV") {
	bits::get(input_buffer,packed,offset,tbentry.width);
	bd[subset_number]->elev=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	return;
    }
    else if (se.key == "TYP") {
	bits::get(input_buffer,packed,offset,tbentry.width);
	bd[subset_number]->type.prepbufr=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	return;
    }
    else if (se.key == "T29") {
	bits::get(input_buffer,packed,offset,tbentry.width);
	bd[subset_number]->type.ON29=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	return;
    }
    else if (se.key == "ITP") {
	bits::get(input_buffer,packed,offset,tbentry.width);
	bd[subset_number]->type.instr=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	return;
    }
    else if (se.key == "SQN") {
	bits::get(input_buffer,packed,offset,tbentry.width);
	bd[subset_number]->seq_num=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	return;
    }
    else if (se.key == "CAT") {
	bits::get(input_buffer,packed,offset,tbentry.width);
	val=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	bd[subset_number]->cat_groups.emplace_back();
	if (val < -999.) {
	  bd[subset_number]->cat_groups.back().code=63;
	}
	else {
	  bd[subset_number]->cat_groups.back().code=val;
	}
	return;
    }
    else if (se.key == "RPT") {
	bits::get(input_buffer,packed,offset,tbentry.width);
	bd[subset_number]->hr=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	if (bd[subset_number]->hr > 24) {
	  bd[subset_number]->hr=-999;
	}
	return;
    }
    if (!fill_header_only) {
	if (se.key == "POB") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().pres[bd[subset_number]->cat_groups.back().pres.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "PFC") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().pres_fcst[bd[subset_number]->cat_groups.back().pres_fcst.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "POE") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().pres_err[bd[subset_number]->cat_groups.back().pres_err.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "PAN") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().pres_anal[bd[subset_number]->cat_groups.back().pres_anal.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "ZOB") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().hgt[bd[subset_number]->cat_groups.back().hgt.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "ZFC") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().hgt_fcst[bd[subset_number]->cat_groups.back().hgt_fcst.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "ZAN") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  if (packed == static_cast<int>(pow(2.,tbentry.width)-1)) {
	    bd[subset_number]->cat_groups.back().hgt_anal[bd[subset_number]->cat_groups.back().hgt_anal.length()]=-99999.;
	  }
	  else {
	    bd[subset_number]->cat_groups.back().hgt_anal[bd[subset_number]->cat_groups.back().hgt_anal.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  }
	  return;
	}
	else if (se.key == "ZOE") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  if (packed == static_cast<int>(pow(2.,tbentry.width)-1)) {
	    bd[subset_number]->cat_groups.back().hgt_err[bd[subset_number]->cat_groups.back().hgt_err.length()]=-99999.;
	  }
	  else {
	    bd[subset_number]->cat_groups.back().hgt_err[bd[subset_number]->cat_groups.back().hgt_err.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  }
	  return;
	}
	else if (se.key == "TOB") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().temp[bd[subset_number]->cat_groups.back().temp.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "TFC") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().temp_fcst[bd[subset_number]->cat_groups.back().temp_fcst.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "TAN") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().temp_anal[bd[subset_number]->cat_groups.back().temp_anal.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "TOE") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().temp_err[bd[subset_number]->cat_groups.back().temp_err.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "QOB") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().q[bd[subset_number]->cat_groups.back().q.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "QFC") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().q_fcst[bd[subset_number]->cat_groups.back().q_fcst.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "QAN") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().q_anal[bd[subset_number]->cat_groups.back().q_anal.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "QOE") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().q_err[bd[subset_number]->cat_groups.back().q_err.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "DDO") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().dd[bd[subset_number]->cat_groups.back().dd.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "FFO") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().ff[bd[subset_number]->cat_groups.back().ff.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "UOB") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().u[bd[subset_number]->cat_groups.back().u.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "VOB") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().v[bd[subset_number]->cat_groups.back().v.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "UFC") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().u_fcst[bd[subset_number]->cat_groups.back().u_fcst.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "VFC") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().v_fcst[bd[subset_number]->cat_groups.back().v_fcst.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "UAN") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().u_anal[bd[subset_number]->cat_groups.back().u_anal.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
	else if (se.key == "VAN") {
	  bits::get(input_buffer,packed,offset,tbentry.width);
	  bd[subset_number]->cat_groups.back().v_anal[bd[subset_number]->cat_groups.back().v_anal.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	  return;
	}
    }
  }
  descriptor_to_handle.ignore=true;
}

void NCEPRadianceBUFRData::print() const
{
  size_t x,n;

  if (!filled) {
    return;
  }
  if (radiance_groups.size() > 1) {
    std::cout << std::endl;
  }
  for (x=0; x < radiance_groups.size(); x++) {
    std::cout << "GROUP: " << (x+1) << " OBS TIME: " << radiance_groups[x].datetime.to_string("%Y-%m-%d %H:%MM:%SS") << " ID: ";
    if (satid.length() > 0) {
	std::cout << satid << " (SAT)";
    }
    else {
	std::cout << "??";
    }
    std::cout << " LAT: " << radiance_groups[x].lat << " LON: " << radiance_groups[x].lon << " ELEV: " << elev << " NUM_CHANNELS: " << radiance_groups[x].channel_data.num.length() << " SATINSTR: " << radiance_groups[x].sat_instr << " FOV_NUM: " << radiance_groups[x].fov_num << std::endl;
    std::cout << "   CHANNEL  BRIGHT_TEMP" << std::endl;
    std::cout.setf(std::ios::fixed);
    std::cout.precision(2);
    for (n=0; n < radiance_groups[x].channel_data.num.length(); ++n) {
	std::cout << std::setw(10) << static_cast<int>(radiance_groups[x].channel_data.num(n)) << std::setw(13) << radiance_groups[x].channel_data.bright_temp(n) << std::endl;
    }
  }
}

void handle_ncep_radiance_bufr(BUFR::Descriptor& descriptor_to_handle,const unsigned char *input_buffer,const InputBUFRStream::TableBEntry& tbentry,size_t offset,BUFRData **bufr_data,int subset_number,short num_subsets,bool fill_header_only)
{
  NCEPRadianceBUFRData **bd=reinterpret_cast<NCEPRadianceBUFRData **>(bufr_data);
  long long packed;
  static double *parray=NULL;
  size_t n;

  if (parray != NULL) {
    delete[] parray;
    parray=NULL;
  }
  switch (descriptor_to_handle.x) {
    case 4:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		if (bd[subset_number]->radiance_groups.size() > 0 && bd[subset_number]->radiance_groups.back().datetime.year() == 1000) {
		  bd[subset_number]->radiance_groups.back().datetime.set_year(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
		}
		else {
		  bd[subset_number]->radiance_groups.emplace_back();
		  bd[subset_number]->radiance_groups.back().datetime.set_year(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
		  bd[subset_number]->radiance_groups.back().sat_instr=-999;
		}
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
		  if (bd[n]->radiance_groups.size() > 0 && bd[n]->radiance_groups.back().datetime.year() == 1000) {
		    bd[n]->radiance_groups.back().datetime.set_year(parray[n]);
		  }
		  else {
		    bd[n]->radiance_groups.emplace_back();
		    bd[n]->radiance_groups.back().datetime.set_year(parray[n]);
		    bd[n]->radiance_groups.back().sat_instr=-999;
		  }
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 2:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->radiance_groups.back().datetime.set_month(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
		  bd[n]->radiance_groups.back().datetime.set_month(parray[n]);
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 3:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->radiance_groups.back().datetime.set_day(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
		  bd[n]->radiance_groups.back().datetime.set_day(parray[n]);
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 4:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->radiance_groups.back().datetime.set_time(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val)*10000);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
		  bd[n]->radiance_groups.back().datetime.set_time(parray[n]*10000);
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 5:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->radiance_groups.back().datetime.set_time(bd[subset_number]->radiance_groups.back().datetime.time()+bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val)*100);
		  bd[subset_number]->filled=true;
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
		  bd[n]->radiance_groups.back().datetime.set_time(bd[n]->radiance_groups.back().datetime.time()+parray[n]*100);
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 6:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->radiance_groups.back().datetime.set_time(bd[subset_number]->radiance_groups.back().datetime.time()+bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
		  if (floatutils::myequalf(parray[n],60.)) {
		    bd[n]->radiance_groups.back().datetime.add_minutes(1);
		  }
		  else {
		    bd[n]->radiance_groups.back().datetime.set_time(bd[n]->radiance_groups.back().datetime.time()+parray[n]);
		  }
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	}
	break;
    }
    case 5:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		size_t idx= (bd[subset_number]->radiance_groups.size() > 0) ? bd[subset_number]->radiance_groups.size()-1 : 0;
		bd[subset_number]->radiance_groups[idx].lat=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
		  bd[n]->radiance_groups.back().lat=parray[n];
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 2:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		size_t idx= (bd[subset_number]->radiance_groups.size() > 0) ? bd[subset_number]->radiance_groups.size()-1 : 0;
		bd[subset_number]->radiance_groups[idx].lat=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
		  bd[n]->radiance_groups.back().lat=parray[n];
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 42:
	  {
	    if (!fill_header_only) {
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  bd[subset_number]->radiance_groups.back().channel_data.num[bd[subset_number]->radiance_groups.back().channel_data.num.length()]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		  bd[subset_number]->radiance_groups.back().channel_data.bright_temp[bd[subset_number]->radiance_groups.back().channel_data.bright_temp.length()]=-99999.;
		}
		else {
		  unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		  for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
		    bd[n]->radiance_groups.back().channel_data.num[bd[n]->radiance_groups.back().channel_data.num.length()]=parray[n];
		    bd[n]->radiance_groups.back().channel_data.bright_temp[bd[n]->radiance_groups.back().channel_data.bright_temp.length()]=-99999.;
		    bd[n]->filled=true;
		  }
		}
	    }
	    return;
	  }
	  case 43:
	  {
	    if (!fill_header_only) {
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  bd[subset_number]->radiance_groups.back().fov_num=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		}
		else {
		  unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		  for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
		    bd[n]->radiance_groups.back().fov_num=parray[n];
		    bd[n]->filled=true;
		  }
		}
	    }
	    return;
	  }
	}
	break;
    }
    case 6:
    {
	switch (descriptor_to_handle.y) {
	  case 1:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		size_t idx= (bd[subset_number]->radiance_groups.size() > 0) ? bd[subset_number]->radiance_groups.size()-1 : 0;
		bd[subset_number]->radiance_groups[idx].lon=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
		  bd[n]->radiance_groups.back().lon=parray[n];
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 2:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		size_t idx= (bd[subset_number]->radiance_groups.size() > 0) ? bd[subset_number]->radiance_groups.size()-1 : 0;
		bd[subset_number]->radiance_groups[idx].lon=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
		  bd[n]->radiance_groups.back().lon=parray[n];
		  bd[n]->filled=true;
		}
	    }
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
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->elev=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
		  bd[n]->elev=parray[n];
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	  case 2:
	  {
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->elev=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
		  bd[n]->elev=parray[n];
		  bd[n]->filled=true;
		}
	    }
	    return;
	  }
	}
	break;
    }
    case 8:
    {
	switch (descriptor_to_handle.y) {
	  case 12:
	  {
/*
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		bd[subset_number]->land_sea=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < static_cast<size_t>(num_subsets); n++) {
		  bd[n]->land_sea=parray[n];
		  bd[n]->filled=true;
		}
	    }
*/
	    return;
	  }
	}
	break;
    }
    case 12:
    {
	switch (descriptor_to_handle.y) {
	  case 163:
	  {
	    if (!fill_header_only) {
		if (subset_number >= 0) {
		  bits::get(input_buffer,packed,offset,tbentry.width);
		  bd[subset_number]->radiance_groups.back().channel_data.bright_temp[bd[subset_number]->radiance_groups.back().channel_data.bright_temp.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
		}
		else {
		  unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		  for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
		    bd[n]->radiance_groups.back().channel_data.bright_temp[bd[n]->radiance_groups.back().channel_data.bright_temp.length()-1]=parray[n];
		    bd[n]->filled=true;
		  }
		}
	    }
	    return;
	  }
	  case 206:
	  {
/*
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		(bd[subset_number]->channel_data.tcorr)[bd[subset_number]->channel_data.tcorr.length()-1]=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < static_cast<size_t>(num_subsets); n++) {
		  (bd[n]->channel_data.tcorr)[bd[n]->channel_data.tcorr.length()-1]=parray[n];
		  bd[n]->filled=true;
		}
	    }
*/
	    return;
	  }
	}
	break;
    }
  }
  if (strutils::has_beginning(tbentry.element_name,"SAID")) {
    if (subset_number >= 0) {
	bits::get(input_buffer,packed,offset,tbentry.width);
	bd[subset_number]->satid=strutils::itos(packed);
    }
    else {
	unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
	for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
	  bd[n]->satid=strutils::itos(parray[n]);
	  bd[n]->filled=true;
	}
    }
    return;
  }
  else if (strutils::has_beginning(tbentry.element_name,"SIID")) {
    if (subset_number >= 0) {
	bits::get(input_buffer,packed,offset,tbentry.width);
	if (bd[subset_number]->radiance_groups.size() > 0 && bd[subset_number]->radiance_groups.back().sat_instr < 0)
	  bd[subset_number]->radiance_groups.back().sat_instr=packed;
	else {
	  bd[subset_number]->radiance_groups.emplace_back();
	  bd[subset_number]->radiance_groups.back().sat_instr=packed;
	  bd[subset_number]->radiance_groups.back().datetime.set_year(1000);
	}
    }
    else {
	unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
	for (n=0; n < static_cast<size_t>(num_subsets); ++n) {
	  if (bd[n]->radiance_groups.size() > 0 && bd[n]->radiance_groups.back().sat_instr < 0)
	    bd[n]->radiance_groups.back().sat_instr=parray[n];
	  else {
	    bd[n]->radiance_groups.emplace_back();
	    bd[n]->radiance_groups.back().sat_instr=parray[n];
	    bd[n]->radiance_groups.back().datetime.set_year(1000);
	  }
	  bd[n]->filled=true;
	}
    }
    return;
  }
  descriptor_to_handle.ignore=true;
}

std::string prepbufr_id_key(std::string clean_ID,std::string pentry_key,std::string message_type)
{
  std::string key,sdum;

  key=clean_ID;
  if (std::regex_search(message_type,std::regex("^ADPUPA")) || std::regex_search(message_type,std::regex("^PROFLR")) || std::regex_search(message_type,std::regex("^ADPSFC")) || std::regex_search(message_type,std::regex("^RASSDA"))) {
    if (pentry_key == "aircraft") {
	key=pentry_key+"[!]PREPBUFRacftID[!]"+key.substr(0,3);
    }
    else {
	if (strutils::is_numeric(key)) {
	  if (key.length() == 5) {
	    key=pentry_key+"[!]WMO[!]"+key;
	  }
	  else {
	    key=pentry_key+"[!]other[!]"+key;
	  }
	}
	else {
	  key=pentry_key+"[!]callSign[!]"+key;
	}
     }
  }
  else if (std::regex_search(message_type,std::regex("^AIRCAR"))) {
    key=pentry_key+"[!]PREPBUFRacftID[!]"+key;
  }
  else if (std::regex_search(message_type,std::regex("^AIRCFT"))) {
    if (key.length() == 6 && key[0] == 'P' && key[5] == 'P' && strutils::is_numeric(key.substr(1,4))) {
	key=pentry_key+"[!]PREPBUFRacftID[!]PP";
    }
    else {
	key=pentry_key+"[!]PREPBUFRacftID[!]"+key;
    }
  }
  else if (std::regex_search(message_type,std::regex("^SATWND")) || std::regex_search(message_type,std::regex("^SATEMP")) || std::regex_search(message_type,std::regex("^SPSSMI"))) {
    sdum=key;
    if ((sdum[0] >= '0' && sdum[0] <= '9') || sdum[0] == '*') {
	if (sdum.length() == 6 || sdum.length() == 7) {
	  key=pentry_key+"[!]PREPBUFRsatID[!]"+sdum.substr(5);
	}
	else if (sdum.length() == 8) {
	  while (sdum.length() > 0 && ((sdum[0] >= '0' && sdum[0] <= '9') || sdum[0] == '*')) {
	    sdum=sdum.substr(1);
	  }
	  if (sdum.length() > 0) {
	    key=pentry_key+"[!]PREPBUFRsatID[!]"+sdum;
	  }
	  else {
	    key="";
	  }
	}
	else if (strutils::is_numeric(sdum)) {
	  key=pentry_key+"[!]BUFRsatID[!]"+sdum;
	}
	else {
	  key="";
	}
    }
    else if (sdum[0] >= 'A' && sdum[0] <= 'Z') {
	if (sdum[1] == '1' && sdum[5] >= 'A' && sdum[5] <= 'Z') {
	  key=pentry_key+"[!]PREPBUFRsatID[!]"+sdum.substr(0,2)+sdum.substr(5,1);
	}
	else if (sdum.length() == 8 && sdum[1] >= 'A' && sdum[1] <= 'Z' && sdum[7] >= 'A' && sdum[7] <= 'Z' && strutils::is_numeric(sdum.substr(2,5))) {
	  key=pentry_key+"[!]PREPBUFRsatID[!]"+sdum.substr(0,2)+sdum.substr(7,1);
	}
	else if (sdum.length() == 7 && sdum[1] >= 'A' && sdum[1] <= 'Z' && sdum[5] >= 'A' && sdum[5] <= 'Z' && strutils::is_numeric(sdum.substr(2,3))) {
	  key=pentry_key+"[!]PREPBUFRsatID[!]"+sdum.substr(0,2)+sdum.substr(5,2);
	}
	else {
	  key=pentry_key+"[!]PREPBUFRsatID[!]"+sdum.substr(0,1);
	  sdum=sdum.substr(1);
	  strutils::trim(sdum);
	  if (!strutils::is_numeric(sdum)) {
	    while (sdum.length() > 0 && sdum[0] >= '0' && sdum[0] <= '9') {
		sdum=sdum.substr(1);
	    }
	    key+=sdum;
	  }
	}
    }
    else {
	key="";
    }
  }
  else if (std::regex_search(message_type,std::regex("^GOESND"))) {
    key=pentry_key+"[!]PREPBUFRsatID[!]"+key.substr(0,1)+key.substr(7);
  }
  else if (std::regex_search(message_type,std::regex("^GPSIPW"))) {
    key=pentry_key+"[!]PREPBUFRsatID[!]"+key;
  }
  else if (std::regex_search(message_type,std::regex("^VADWND"))) {
    key=pentry_key+"[!]PREPBUFRvadID[!]"+key.substr(4);
  }
  else if (std::regex_search(message_type,std::regex("^SFCSHP"))) {
    key=pentry_key+"[!]PREPBUFRmarineID[!]"+key;
  }
  else if (std::regex_search(message_type,std::regex("^SFCBOG"))) {
    if (key[0] == 'P') {
	key=pentry_key+"[!]other[!]P";
    }
    else {
	key="";
    }
  }
  else if (std::regex_search(message_type,std::regex("^ERS1DA"))) {
    key=pentry_key+"[!]PREPBUFRsatID[!]E";
  }
  else if (std::regex_search(message_type,std::regex("^SYNDAT"))) {
    key=pentry_key+"[!]PREPBUFRsyndatID[!]"+key.substr(0,4);
  }
  else if (std::regex_search(message_type,std::regex("^QKSWND"))) {
    if (key[7] == 'Q') {
	if (key[1] == 'S') {
	  key=pentry_key+"[!]PREPBUFRsatID[!]SQ";
	}
	else {
	  key=pentry_key+"[!]PREPBUFRsatID[!]Q";
	}
    }
    else if (strutils::is_numeric(key)) {
	key=pentry_key+"[!]BUFRsatID[!]"+key;
    }
    else {
	key="";
    }
  }
  else if (std::regex_search(message_type,std::regex("^ASCATW"))) {
    if (key[7] == 'A') {
	if (key[1] == 'S') {
	  key=pentry_key+"[!]PREPBUFRsatID[!]SA";
	}
	else {
	  key=pentry_key+"[!]PREPBUFRsatID[!]A";
	}
    }
    else if (strutils::is_numeric(key)) {
	key=pentry_key+"[!]BUFRsatID[!]"+key;
    }
    else {
	key="";
    }
  }
  else if (std::regex_search(message_type,std::regex("^WDSATR"))) {
    if (key[7] == 'W') {
	if (key[1] == 'S') {
	  key=pentry_key+"[!]PREPBUFRsatID[!]SW";
	}
	else {
	  key=pentry_key+"[!]PREPBUFRsatID[!]W";
	}
    }
    else if (strutils::is_numeric(key)) {
	key=pentry_key+"[!]BUFRsatID[!]"+key;
    }
    else {
	key="";
    }
  }
  else if (std::regex_search(message_type,std::regex("^MSONET"))) {
    auto sp=strutils::split(key);
    if (strutils::is_alphanumeric(sp[0])) {
	key=pentry_key+"[!]callSign[!]"+sp[0];
    }
    else {
	key="";
    }
  }
  else {
    key="";
  }
  return key;
}
