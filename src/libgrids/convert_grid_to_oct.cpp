#include <iomanip>
#include <grid.hpp>

namespace gridConversions {

int convert_grid_to_oct(const Grid *source_grid, Grid::Format format,odstream *ostream,size_t grid_number)
{
  static LatLonGrid ucomp;
  OctagonalGrid oct_grid;
  int num_bytes=0,num_v;
  const size_t BUF_LEN=3000;
  unsigned char output_buffer[BUF_LEN];

  switch (format) {
    case Grid::Format::octagonal: {
	oct_grid=*(reinterpret_cast<OctagonalGrid *>(const_cast<Grid *>(source_grid)));
	break;
    }
    case Grid::Format::latlon: {
	if (source_grid->parameter() != 30 && source_grid->parameter() != 31) {
	  oct_grid=*(reinterpret_cast<LatLonGrid *>(const_cast<Grid *>(source_grid)));
	}
	break;
    }
    case Grid::Format::slp: {
std::cerr << "Error: unable to convert Sea-Level Pressure grids to Octagonal format"
  << std::endl;
return -1;
    }
    case Grid::Format::grib: {
std::cerr << "Error: unable to convert GRIB grids to Octagonal format" << std::endl;
return -1;
    }
    case Grid::Format::cgcm1: {
std::cerr << "Error: unable to convert CGCM1 grids to Octagonal format" << std::endl;
return -1;
    }
    case Grid::Format::navy: {
	oct_grid=*(reinterpret_cast<NavyGrid *>(const_cast<Grid *>(source_grid)));
	break;
    }
    default: {
	return -1;
    }
  }

  if (format != Grid::Format::latlon || (format == Grid::Format::latlon && source_grid->parameter() != 30 && source_grid->parameter() != 31)) {
    num_bytes=oct_grid.copy_to_buffer(output_buffer,BUF_LEN);
    if (num_bytes > 0) {
	if (ostream->write(output_buffer,num_bytes) != num_bytes) {
	  std::cerr << "Error: write error on grid " << grid_number << std::endl;
	}
    }
  }
  else {
    if (source_grid->parameter() == 30) {
	ucomp=*(reinterpret_cast<LatLonGrid *>(const_cast<Grid *>(source_grid)));
    }
    else {
	if (source_grid->reference_date_time() != ucomp.reference_date_time()) {
	  return 0;
	}
	convert_xy_wind_to_ij(ucomp,*(reinterpret_cast<LatLonGrid *>(const_cast<Grid *>(source_grid))));
	oct_grid=ucomp;
	num_bytes=oct_grid.copy_to_buffer(output_buffer,BUF_LEN);
	if (num_bytes > 0) {
	  if (ostream->write(output_buffer,num_bytes) != num_bytes) {
	    std::cerr << "Error: write error on grid " << grid_number << std::endl;
	  }
	}
	oct_grid=*(reinterpret_cast<LatLonGrid *>(const_cast<Grid *>(source_grid)));
	num_v=oct_grid.copy_to_buffer(output_buffer,BUF_LEN);
	if (num_v > 0) {
	  if (ostream->write(output_buffer,num_v) != num_v) {
	    std::cerr << "Error: write error on grid " << grid_number << std::endl;
	  }
	}
	num_bytes+=num_v;
    }
  }
  return num_bytes;
}

} // end namespace gridConversions
