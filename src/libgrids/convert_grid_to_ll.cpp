#include <iomanip>
#include <grid.hpp>

namespace gridConversions {

int convert_grid_to_ll(const Grid *source_grid, Grid::Format format,odstream *ostream,size_t grid_number,float resolution)
{
  static LatLonGrid ucomp(resolution);
  LatLonGrid ll_grid(resolution);
  switch (format) {
    case Grid::Format::grib: {
	ll_grid=*(reinterpret_cast<GRIBGrid *>(const_cast<Grid *>(source_grid)));
      break;
    }
    case Grid::Format::octagonal: {
	ll_grid=*(reinterpret_cast<OctagonalGrid *>(const_cast<Grid *>(source_grid)));
      break;
    }
    case Grid::Format::latlon: {
	ll_grid=*(reinterpret_cast<LatLonGrid *>(const_cast<Grid *>(source_grid)));
      break;
    }
    case Grid::Format::slp: {
std::cerr << "Error: unable to convert Sea-Level Pressure grids to LatLon format" <<
  std::endl;
return -1;
    }
    case Grid::Format::on84: {
	ll_grid=*(reinterpret_cast<ON84Grid *>(const_cast<Grid *>(source_grid)));
	break;
    }
    case Grid::Format::cgcm1: {
std::cerr << "Error: unable to convert CGCM1 grids to LatLon format" << std::endl;
return -1;
    }
    default: {
	return -1;
    }
  }
  static std::unique_ptr<unsigned char []> output_buffer;
  const size_t BUF_LEN=15000;
  output_buffer.reset(new unsigned char[BUF_LEN]);
  auto num_bytes=0;
  if (format != Grid::Format::octagonal || (format == Grid::Format::octagonal && ll_grid.parameter() != 30 && ll_grid.parameter() != 31)) {
    num_bytes=ll_grid.copy_to_buffer(output_buffer.get(),BUF_LEN);
    if (num_bytes > 0) {
	if (ostream->write(output_buffer.get(),num_bytes) != num_bytes) {
	  std::cerr << "Error: write error on grid " << grid_number << std::endl;
	}
    }
  }
  else {
    if (ll_grid.parameter() == 30) {
	ucomp=ll_grid;
    }
    else {
	if (ll_grid.reference_date_time() != ucomp.reference_date_time()) {
	  return 0;
	}
	convert_ij_wind_to_xy(ucomp,ll_grid);
	num_bytes=ucomp.copy_to_buffer(output_buffer.get(),BUF_LEN);
	if (num_bytes > 0) {
	  if (ostream->write(output_buffer.get(),num_bytes) != num_bytes) {
	    std::cerr << "Error: write error on grid " << grid_number << std::endl;
	  }
	}
	int num_v=ll_grid.copy_to_buffer(output_buffer.get(),BUF_LEN);
	if (num_v > 0) {
	  if (ostream->write(output_buffer.get(),num_v) != num_v) {
	    std::cerr << "Error: write error on grid " << grid_number << std::endl;
	  }
	}
	num_bytes+=num_v;
    }
  }
  return num_bytes;
}

} // end namespace gridConversions
