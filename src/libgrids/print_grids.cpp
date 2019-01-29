#include <iomanip>
#include <grid.hpp>

namespace gridPrint {

int print(std::string input_filename,size_t format,bool headers_only,bool verbose,size_t start,size_t stop,std::string path_to_parameter_map)
{
  idstream *grid_stream;
  Grid *source_grid;
  GRIBMessage *msg=NULL;
  switch (format) {
    case Grid::cgcm1Format:
    {
	grid_stream=new InputCGCM1GridStream;
	source_grid=new CGCM1Grid;
	break;
    }
    case Grid::dslpFormat:
    {
	grid_stream=new InputDiamondSLPGridStream;
	source_grid=new DiamondSLPGrid;
	break;
    }
    case Grid::gribFormat:
    {
	grid_stream=new InputGRIBStream;
	msg=new GRIBMessage;
	source_grid=new GRIBGrid;
	break;
    }
    case Grid::grib2Format:
    {
	grid_stream=new InputGRIBStream;
	msg=new GRIB2Message;
	source_grid=new GRIB2Grid;
	break;
    }
    case Grid::jraieeemmFormat:
    {
	grid_stream=new InputJRAIEEEMMGridStream;
	source_grid=new JRAIEEEMMGrid;
	break;
    }
    case Grid::latlonFormat:
    {
	grid_stream=new InputLatLonGridStream;
	source_grid=new LatLonGrid;
	break;
    }
    case Grid::latlon25Format:
    {
	grid_stream=new InputLatLonGridStream(2.5);
	source_grid=new LatLonGrid(2.5);
	break;
    }
    case Grid::navyFormat:
    {
	grid_stream=new InputNavyGridStream;
	source_grid=new NavyGrid;
	break;
    }
    case Grid::octagonalFormat:
    {
	grid_stream=new InputOctagonalGridStream;
	source_grid=new OctagonalGrid;
	break;
    }
    case Grid::oldlatlonFormat:
    {
	grid_stream=new InputOldLatLonGridStream;
	source_grid=new OldLatLonGrid;
	break;
    }
    case Grid::on84Format:
    {
	grid_stream=new InputON84GridStream;
	source_grid=new ON84Grid;
	break;
    }
    case Grid::slpFormat:
    {
	grid_stream=new InputSLPGridStream;
	source_grid=new SLPGrid;
	break;
    }
    case Grid::tdlgribFormat:
    {
	grid_stream=new InputTDLGRIBGridStream;
	source_grid=new TDLGRIBGrid;
	break;
    }
    case Grid::tropicalFormat:
    {
	grid_stream=new InputTropicalGridStream;
	source_grid=new TropicalGrid;
	break;
    }
    case Grid::ukslpFormat:
    {
	grid_stream=new InputUKSLPGridStream;
	source_grid=new UKSLPGrid;
	break;
    }
    case Grid::ussrslpFormat:
    {
	grid_stream=new InputUSSRSLPGridStream;
	source_grid=new USSRSLPGrid;
	break;
    }
    default:
    {
	std::cerr << "Error: invalid format specified " << format << std::endl;
	return 1;
    }
  }
  if (!grid_stream->open(input_filename)) {
    std::cerr << "Error opening " << input_filename << std::endl;
    exit(1);
  }
  for (const auto& byte_stream : *grid_stream) {
    if (grid_stream->number_read() >= start) {
	if (format == Grid::gribFormat || format == Grid::grib2Format) {
	  msg->fill(byte_stream,headers_only);
	  if (verbose) {
	    std::cout << "MESSAGE: " << std::setw(6) << grid_stream->number_read();
	  }
	  else {
	    std::cout << "MSG=" << grid_stream->number_read();
	  }
	  msg->print_header(std::cout,verbose,path_to_parameter_map);
	}
	else {
	  source_grid->fill(byte_stream,headers_only);
	  if (verbose) {
	    std::cout << "GRID: " << std::setw(6) << grid_stream->number_read();
	  }
	  else {
	    std::cout << "GRID=" << grid_stream->number_read();
	  }
	  source_grid->print_header(std::cout,verbose,path_to_parameter_map);
	}
	if (!headers_only && verbose) {
	  if (format == Grid::gribFormat || format == Grid::grib2Format) {
	    for (size_t n=0; n < msg->number_of_grids(); ++n) {
		source_grid=msg->grid(n);
		source_grid->print(std::cout);
	    }
	  }
	  else {
	    source_grid->print(std::cout);
	  }
	}
    }
    if (grid_stream->number_read() == stop) {
	break;
    }
  }
  return 0;
}

int print_ascii(std::string input_filename,size_t format,size_t start,size_t stop)
{
  idstream *grid_stream;
  Grid *source_grid;
  switch (format) {
    case Grid::gribFormat:
    {
	grid_stream=new InputGRIBStream;
	source_grid=new GRIBGrid;
	break;
    }
    case Grid::tdlgribFormat:
    {
	grid_stream=new InputTDLGRIBGridStream;
	source_grid=new TDLGRIBGrid;
	break;
    }
    case Grid::octagonalFormat:
    {
	grid_stream=new InputOctagonalGridStream;
	source_grid=new OctagonalGrid;
	break;
    }
    case Grid::latlonFormat:
    {
	grid_stream=new InputLatLonGridStream;
	source_grid=new LatLonGrid;
	break;
    }
    case Grid::latlon25Format:
    {
	grid_stream=new InputLatLonGridStream(2.5);
	source_grid=new LatLonGrid(2.5);
	break;
    }
    case Grid::slpFormat:
    {
	grid_stream=new InputSLPGridStream;
	source_grid=new SLPGrid;
	break;
    }
    case Grid::on84Format:
    {
	grid_stream=new InputON84GridStream;
	source_grid=new ON84Grid;
	break;
    }
    case Grid::cgcm1Format:
    {
	grid_stream=new InputCGCM1GridStream;
	source_grid=new CGCM1Grid;
	break;
    }
    case Grid::ussrslpFormat:
    {
	grid_stream=new InputUSSRSLPGridStream;
	source_grid=new USSRSLPGrid;
	break;
    }
    case Grid::navyFormat:
    {
	grid_stream=new InputNavyGridStream;
	source_grid=new NavyGrid;
	break;
    }
    default:
    {
	std::cerr << "Error: invalid format specified " << format << std::endl;
	return 1;
    }
  }

  if (!grid_stream->open(input_filename)) {
    std::cerr << "Error opening " << input_filename << std::endl;
    exit(1);
  }
  for (const auto& byte_stream : *grid_stream) {
    if (grid_stream->number_read() == stop) {
	break;
    }
    if (grid_stream->number_read() >= start) {
	source_grid->fill(byte_stream,Grid::full_grid);
	source_grid->print_ascii(std::cout);
    }
  }
  return 0;
}

} // end namespace gridPrint
