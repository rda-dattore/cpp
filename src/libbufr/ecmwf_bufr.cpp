#include <iomanip>
#include <bufr.hpp>
#include <strutils.hpp>
#include <bits.hpp>
#include <datetime.hpp>
#include <myerror.hpp>

void ECMWFBUFRData::fill(const unsigned char *input_buffer,size_t offset)
{
  size_t len;
  short yr,mo,dy,hr,min,sec;
  int idum;
  char cid[9];

  bits::get(input_buffer,len,offset,24);
  bits::get(input_buffer,ecmwf_os.rdb_type,offset+32,8);
  bits::get(input_buffer,ecmwf_os.rdb_subtype,offset+40,8);
//std::cerr << "rdb type/subtype: " << ecmwf_os.rdb_type << " " << ecmwf_os.rdb_subtype << std::endl;
  bits::get(input_buffer,yr,offset+48,12);
  bits::get(input_buffer,mo,offset+60,4);
  bits::get(input_buffer,dy,offset+64,6);
  bits::get(input_buffer,hr,offset+70,5);
  bits::get(input_buffer,min,offset+75,6);
  bits::get(input_buffer,sec,offset+81,6);
  ecmwf_os.datetime.set(yr,mo,dy,hr*10000+min*100+sec);
  switch (len) {
    case 52:
	bits::get(input_buffer,idum,offset+88,26);
	ecmwf_os.lon=(idum-18000000)/100000.;
	bits::get(input_buffer,idum,offset+120,25);
	ecmwf_os.lat=(idum-9000000)/100000.;
	switch (ecmwf_os.rdb_type) {
	  case 2:
	  case 3:
	  case 8:
	  case 12:
	    if (ecmwf_os.rdb_subtype == 31 || (ecmwf_os.rdb_subtype >= 121 && ecmwf_os.rdb_subtype <= 130))
		bits::get(input_buffer,idum,offset+168,16);
	    else
		bits::get(input_buffer,idum,offset+160,16);
	    ecmwf_os.ID=strutils::itos(idum);
	    break;
	  default:
	    bits::get(input_buffer,cid,offset+152,8,0,9);
	    ecmwf_os.ID.assign(cid,9);
	    strutils::trim(ecmwf_os.ID);
	}
	break;
    default:
	myerror="Error: unable to decode optional section for length "+strutils::itos(len);
	exit(1);
  }
  filled=true;
}

void ECMWFBUFRData::print() const
{
  std::cout << "RDB TYP/SUBTYP: " << ecmwf_os.rdb_type << "/" << ecmwf_os.rdb_subtype << "  OBS TIME: " << ecmwf_os.datetime.to_string("%Y-%m-%d %H:%MM:%SS") << "  ID: " << ecmwf_os.ID << "  LAT: " << ecmwf_os.lat << "  LON: " << ecmwf_os.lon << std::endl;
  if (!filled) {
    return;
  }
  if (wmoid.length() > 0) {
    std::cout << "    WMO: " << wmoid;
  }
  else if (shipid.length() > 0) {
    std::cout << "    SHIP: " << shipid;
  }
  else if (buoyid.length() > 0) {
    std::cout << "    BUOY: " << buoyid;
  }
  else if (satid.length() > 0) {
    std::cout << "    SATID: " << satid;
  }
  else if (acftid.length() > 0) {
    std::cout << "    ACFTID: " << acftid;
  }
  std::cout << " TIME: " << datetime.to_string("%Y-%m-%d %H:%MM:%SS") << " LAT: " << lat << " LON: " << lon << std::endl;
}

void handle_ecmwf_bufr(BUFR::Descriptor& descriptor_to_handle,const unsigned char *input_buffer,const InputBUFRStream::TableBEntry& tbentry,size_t offset,BUFRData **bufr_data,int subset_number,short num_subsets,bool fill_header_only)
{
  ECMWFBUFRData **ed=reinterpret_cast<ECMWFBUFRData **>(bufr_data);
  long long packed=0;
  double *parray;
  char cpacked[256];
  int n;

  switch (descriptor_to_handle.x) {
    case 1:
	switch (descriptor_to_handle.y) {
	  case 1:
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		if (packed < 127) {
		  ed[subset_number]->wmoid=strutils::ftos(packed,2,0,'0');
		}
		else {
		  ed[subset_number]->wmoid="99";
		}
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < num_subsets; n++) {
		  if (parray[n] < 127) {
		    ed[n]->wmoid=strutils::ftos(parray[n],2,0,'0');
		  }
		  else {
		    ed[n]->wmoid="99";
		  }
		}
		delete[] parray;
	    }
	    break;
	  case 2:
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		if (packed < 1023) {
		  ed[subset_number]->wmoid+=strutils::ftos(packed,3,0,'0');
		}
		else {
		  ed[subset_number]->wmoid+="999";
		}
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (n=0; n < num_subsets; n++) {
		  if (parray[n] < 1023) {
		    ed[n]->wmoid+=strutils::ftos(parray[n],3,0,'0');
		  }
		  else {
		    ed[n]->wmoid+="999";
		  }
		}
		delete[] parray;
	    }
	    break;
	  case 5:
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		ed[subset_number]->buoyid=strutils::ftos(packed,3,0,'0');
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  ed[n]->buoyid=strutils::ftos(parray[n],3,0,'0');
		}
		delete[] parray;
	    }
	    break;
	  case 6:
	  case 194:
	    if (subset_number >= 0) {
		bits::get(input_buffer,cpacked,offset,8,0,tbentry.width/8);
		ed[subset_number]->acftid.assign(cpacked,tbentry.width/8);
		strutils::trim(ed[subset_number]->acftid);
		if (ed[subset_number]->acftid.length() == 0) {
		  ed[subset_number]->acftid="unknown";
		}
	    }
	    else {
		myerror="Error: unabled to uncompress character data";
		exit(1);
	    }
	    break;;
	  case 7:
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		ed[subset_number]->satid=strutils::itos(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  ed[n]->satid=strutils::itos(parray[n]);
		}
		delete[] parray;
	    }
	    break;
	  case 11:
	  case 195:
	    if (subset_number >= 0) {
		bits::get(input_buffer,cpacked,offset,8,0,tbentry.width/8);
		ed[subset_number]->shipid.assign(cpacked,tbentry.width/8);
		strutils::trim(ed[subset_number]->shipid);
		if (ed[subset_number]->shipid.length() == 0) {
		  ed[subset_number]->shipid="unknown";
		}
	    }
	    else {
		myerror="Error: unabled to uncompress character data";
		exit(1);
	    }
	    break;
/*
	  case 31:
	  case 32:
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
std::cerr << "flag val is " << bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val) << std::endl;
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
std::cerr << "flag val is " << parray[0] << std::endl;
		delete[] parray;
	    }
	    break;
*/
	}
	break;
    case 4:
	switch (descriptor_to_handle.y) {
	  case 1:
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		ed[subset_number]->datetime.set_year(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  if (parray[n] > 0.) {
		    ed[n]->datetime.set_year(parray[n]);
		  }
		}
		delete[] parray;
	    }
	    break;
	  case 2:
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		ed[subset_number]->datetime.set_month(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  if (parray[n] > 0.) {
		    ed[n]->datetime.set_month(parray[n]);
		  }
		}
		delete[] parray;
	    }
	    break;
	  case 3:
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		ed[subset_number]->datetime.set_day(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  if (parray[n] > 0.) {
		    ed[n]->datetime.set_day(parray[n]);
		  }
		}
		delete[] parray;
	    }
	    break;
	  case 4:
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		ed[subset_number]->datetime.set_time(bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val)*10000);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  if (parray[n] > 0.) {
		    ed[n]->datetime.set_time(parray[n]*10000);
		  }
		}
		delete[] parray;
	    }
	    break;
	  case 5:
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		ed[subset_number]->datetime.set_time(ed[subset_number]->datetime.time()+bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val)*100);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  if (parray[n] > 0.) {
		    ed[n]->datetime.set_time(ed[n]->datetime.time()+parray[n]*100);
		  }
		}
		delete[] parray;
	    }
	    break;
	  case 6:
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		ed[subset_number]->datetime.set_time(ed[subset_number]->datetime.time()+bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val));
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  if (parray[n] > 0.) {
		    ed[n]->datetime.set_time(ed[n]->datetime.time()+parray[n]);
		  }
		}
		delete[] parray;
	    }
	    break;
	}
	break;
    case 5:
	switch (descriptor_to_handle.y) {
	  case 1:
	  case 2:
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		ed[subset_number]->lat=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  ed[n]->lat=parray[n];
		}
		delete[] parray;
	    }
	    break;
	}
	break;
    case 6:
	switch (descriptor_to_handle.y) {
	  case 1:
	  case 2:
	    if (subset_number >= 0) {
		bits::get(input_buffer,packed,offset,tbentry.width);
		ed[subset_number]->lon=bufr_value(packed,tbentry.width,tbentry.scale,tbentry.ref_val);
	    }
	    else {
		unpack_bufr_values(input_buffer,offset,tbentry.width,tbentry.scale,tbentry.ref_val,&parray,num_subsets);
		for (int n=0; n < num_subsets; ++n) {
		  ed[n]->lon=parray[n];
		}
		delete[] parray;
	    }
	    break;
	}
	break;
  }
}
