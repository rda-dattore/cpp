#include <iomanip>
#include <grid.hpp>

int convert_grid_to_slp(const Grid *source_grid,size_t format,odstream *ostream,size_t grid_number)
{
  SLPGrid slp_grid;
  switch (format) {
    case Grid::cgcm1Format:
    {
std::cerr << "Error: unable to convert CGCM1 grids to SLP format" << std::endl;
return -1;
    }
    case Grid::gribFormat:
    {
	slp_grid=*(reinterpret_cast<GRIBGrid *>(const_cast<Grid *>(source_grid)));
	break;
    }
    case Grid::grib2Format:
    {
	slp_grid=*(reinterpret_cast<GRIB2Grid *>(const_cast<Grid *>(source_grid)));
	break;
    }
    case Grid::latlonFormat:
    {
std::cerr << "Error: unable to convert Latitude/Longitude grids to SLP format" << std::endl;
return -1;
    }
    case Grid::octagonalFormat:
    {
std::cerr << "Error: unable to convert Octagonal grids to SLP format" << std::endl;
return -1;
    }
    case Grid::slpFormat:
    {
	slp_grid=*(reinterpret_cast<SLPGrid *>(const_cast<Grid *>(source_grid)));
	break;
    }
    case Grid::ukslpFormat:
    {
	slp_grid=*(reinterpret_cast<UKSLPGrid *>(const_cast<Grid *>(source_grid)));
	break;
    }
  }
  static std::unique_ptr<unsigned char []> output_buffer;
  output_buffer.reset(new unsigned char[2048]);
  int num_bytes=slp_grid.copy_to_buffer(output_buffer.get(),2048);
  if (num_bytes > 0) {
    if (ostream->write(output_buffer.get(),num_bytes) != num_bytes) {
	std::cerr << "Error: write error on grid " << grid_number << std::endl;
    }
  }
  return num_bytes;
}
