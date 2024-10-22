#include <iomanip>
#include <grid.hpp>

namespace grid_print {

int print(std::string input_filename, Grid::Format format,bool headers_only,bool verbose,size_t start,size_t stop,std::string path_to_parameter_map)
{
  idstream *grid_stream;
  Grid *source_grid;
  GRIBMessage *msg=NULL;
  switch (format) {
    case Grid::Format::cgcm1: {
	grid_stream=new InputCGCM1GridStream;
	source_grid=new CGCM1Grid;
	break;
    }
    case Grid::Format::dslp: {
	grid_stream=new InputDiamondSLPGridStream;
	source_grid=new DiamondSLPGrid;
	break;
    }
    case Grid::Format::grib: {
	grid_stream=new InputGRIBStream;
	msg=new GRIBMessage;
	source_grid=new GRIBGrid;
	break;
    }
    case Grid::Format::grib2: {
	grid_stream=new InputGRIBStream;
	msg=new GRIB2Message;
	source_grid=new GRIB2Grid;
	break;
    }
    case Grid::Format::jraieeemm: {
	grid_stream=new InputJRAIEEEMMGridStream;
	source_grid=new JRAIEEEMMGrid;
	break;
    }
    case Grid::Format::latlon: {
	grid_stream=new InputLatLonGridStream;
	source_grid=new LatLonGrid;
	break;
    }
    case Grid::Format::latlon25: {
	grid_stream=new InputLatLonGridStream(2.5);
	source_grid=new LatLonGrid(2.5);
	break;
    }
    case Grid::Format::navy: {
	grid_stream=new InputNavyGridStream;
	source_grid=new NavyGrid;
	break;
    }
    case Grid::Format::octagonal: {
	grid_stream=new InputOctagonalGridStream;
	source_grid=new OctagonalGrid;
	break;
    }
    case Grid::Format::oldlatlon: {
	grid_stream=new InputOldLatLonGridStream;
	source_grid=new OldLatLonGrid;
	break;
    }
    case Grid::Format::on84: {
	grid_stream=new InputON84GridStream;
	source_grid=new ON84Grid;
	break;
    }
    case Grid::Format::slp: {
	grid_stream=new InputSLPGridStream;
	source_grid=new SLPGrid;
	break;
    }
    case Grid::Format::tdlgrib: {
	grid_stream=new InputTDLGRIBGridStream;
	source_grid=new TDLGRIBGrid;
	break;
    }
    case Grid::Format::tropical: {
	grid_stream=new InputTropicalGridStream;
	source_grid=new TropicalGrid;
	break;
    }
    case Grid::Format::ukslp: {
	grid_stream=new InputUKSLPGridStream;
	source_grid=new UKSLPGrid;
	break;
    }
    case Grid::Format::ussrslp: {
	grid_stream=new InputUSSRSLPGridStream;
	source_grid=new USSRSLPGrid;
	break;
    }
    default: {
	std::cerr << "Error: invalid format specified " << static_cast<int>(format) << std::endl;
	return 1;
    }
  }
  if (!grid_stream->open(input_filename)) {
    std::cerr << "Error opening " << input_filename << std::endl;
    exit(1);
  }
  for (const auto& byte_stream : *grid_stream) {
    if (grid_stream->number_read() >= start) {
	if (format == Grid::Format::grib || format == Grid::Format::grib2) {
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
	  if (format == Grid::Format::grib || format == Grid::Format::grib2) {
	    for (size_t n=0; n < msg->number_of_grids(); ++n) {
		source_grid=msg->grid(n);
		source_grid->set_path_to_gaussian_latitude_data("/glade/u/home/rdadata/share/GRIB");
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

int print_ascii(std::string input_filename, Grid::Format format,size_t start,size_t stop)
{
  idstream *grid_stream;
  Grid *source_grid;
  switch (format) {
    case Grid::Format::grib: {
	grid_stream=new InputGRIBStream;
	source_grid=new GRIBGrid;
	break;
    }
    case Grid::Format::tdlgrib: {
	grid_stream=new InputTDLGRIBGridStream;
	source_grid=new TDLGRIBGrid;
	break;
    }
    case Grid::Format::octagonal: {
	grid_stream=new InputOctagonalGridStream;
	source_grid=new OctagonalGrid;
	break;
    }
    case Grid::Format::latlon: {
	grid_stream=new InputLatLonGridStream;
	source_grid=new LatLonGrid;
	break;
    }
    case Grid::Format::latlon25: {
	grid_stream=new InputLatLonGridStream(2.5);
	source_grid=new LatLonGrid(2.5);
	break;
    }
    case Grid::Format::slp: {
	grid_stream=new InputSLPGridStream;
	source_grid=new SLPGrid;
	break;
    }
    case Grid::Format::on84: {
	grid_stream=new InputON84GridStream;
	source_grid=new ON84Grid;
	break;
    }
    case Grid::Format::cgcm1: {
	grid_stream=new InputCGCM1GridStream;
	source_grid=new CGCM1Grid;
	break;
    }
    case Grid::Format::ussrslp: {
	grid_stream=new InputUSSRSLPGridStream;
	source_grid=new USSRSLPGrid;
	break;
    }
    case Grid::Format::navy: {
	grid_stream=new InputNavyGridStream;
	source_grid=new NavyGrid;
	break;
    }
    default: {
	std::cerr << "Error: invalid format specified " << static_cast<int>(format) << std::endl;
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
	source_grid->fill(byte_stream, Grid::FULL_GRID);
	source_grid->print_ascii(std::cout);
    }
  }
  return 0;
}

} // end namespace gridPrint
