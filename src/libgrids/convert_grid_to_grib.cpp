#include <iomanip>
#include <grid.hpp>

namespace gridConversions {

int convert_grid_to_grib1(const Grid *source_grid,size_t format,odstream *ostream,size_t grid_number)
{
  GRIBGrid grib_grid;
  switch (format) {
    case Grid::gribFormat:
    {
	grib_grid=*(reinterpret_cast<GRIBGrid *>(const_cast<Grid *>(source_grid)));
	break;
    }
    case Grid::grib2Format:
    {
	grib_grid=*(reinterpret_cast<GRIB2Grid *>(const_cast<Grid *>(source_grid)));
	break;
    }
    case Grid::tdlgribFormat:
    {
	grib_grid=*(reinterpret_cast<TDLGRIBGrid *>(const_cast<Grid *>(source_grid)));
	break;
    }
    case Grid::octagonalFormat:
    {
	grib_grid=*(reinterpret_cast<OctagonalGrid *>(const_cast<Grid *>(source_grid)));
	break;
    }
    case Grid::latlonFormat:
    case Grid::latlon25Format:
    {
	grib_grid=*(reinterpret_cast<LatLonGrid *>(const_cast<Grid *>(source_grid)));
	break;
    }
    case Grid::slpFormat:
    {
	grib_grid=*(reinterpret_cast<SLPGrid *>(const_cast<Grid *>(source_grid)));
	break;
    }
    case Grid::on84Format:
    {
	grib_grid=*(reinterpret_cast<ON84Grid *>(const_cast<Grid *>(source_grid)));
	break;
    }
    case Grid::cgcm1Format:
    {
	grib_grid=*(reinterpret_cast<CGCM1Grid *>(const_cast<Grid *>(source_grid)));
	break;
    }
    case Grid::ussrslpFormat:
    {
	grib_grid=*(reinterpret_cast<USSRSLPGrid *>(const_cast<Grid *>(source_grid)));
	break;
    }
  }
  GRIBMessage msg;
  msg.initialize(1,nullptr,0,true,(grib_grid.number_missing() > 0));
  msg.append_grid(&grib_grid);
  static std::unique_ptr<unsigned char []> output_buffer;
  output_buffer.reset(new unsigned char[2000000]);
  auto num_bytes=msg.copy_to_buffer(output_buffer.get(),2000000);
  if (ostream->write(output_buffer.get(),num_bytes) != num_bytes) {
    std::cerr << "Error: write error on grid " << grid_number << std::endl;
  }
  return num_bytes;
}

} // end namespace gridConversions
