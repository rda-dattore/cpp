#include <iomanip>
#include <grid.hpp>

namespace gridPrint {

int print(std::string input_filename,size_t format,bool headers_only,bool verbose,size_t start,size_t stop)
{
  idstream *grid_stream;
  GRIBMessage *msg=NULL;
  Grid *source_grid;
  int bytes_read;
  size_t file_size=0,n;
  unsigned char *buffer=nullptr;
  int buf_len=0;

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
  while (grid_stream->number_read() < stop && (bytes_read=grid_stream->peek()) != bfstream::eof) {
    if (bytes_read == bfstream::error) {
	std::cerr << "Error peeking grid " << grid_stream->number_read() << std::endl;
	exit(1);
    }
    else if (bytes_read > buf_len) {
	if (buffer != nullptr) {
	  delete[] buffer;
	}
	buf_len=bytes_read;
	buffer=new unsigned char[buf_len];
    }
    bytes_read=grid_stream->read(buffer,buf_len);
    if (bytes_read == bfstream::eof) {
	break;
    }
    else if (bytes_read == bfstream::error) {
	std::cerr << "Error reading grid " << grid_stream->number_read() << std::endl;
	exit(1);
    }
    file_size+=bytes_read;
    if (grid_stream->number_read() >= start) {
	if (format == Grid::gribFormat || format == Grid::grib2Format) {
	  msg->fill(buffer,headers_only);
	  if (verbose) {
	    std::cout << "MESSAGE: " << std::setw(6) << grid_stream->number_read();
	  }
	  else {
	    std::cout << "MSG=" << grid_stream->number_read();
	  }
	  msg->print_header(std::cout,verbose);
	}
	else {
	  source_grid->fill(buffer,headers_only);
	  if (verbose) {
	    std::cout << "GRID: " << std::setw(6) << grid_stream->number_read();
	  }
	  else {
	    std::cout << "GRID=" << grid_stream->number_read();
	  }
	  source_grid->print_header(std::cout,verbose);
	}
	if (!headers_only && verbose) {
	  if (format == Grid::gribFormat || format == Grid::grib2Format) {
	    for (n=0; n < msg->number_of_grids(); ++n) {
		source_grid=msg->grid(n);
		source_grid->print(std::cout);
	    }
	  }
	  else {
	    source_grid->print(std::cout);
	  }
	}
    }
  }
  return 0;
}

int print_ascii(std::string input_filename,size_t format,size_t start,size_t stop)
{
  idstream *grid_stream;
  Grid *source_grid;
  int bytes_read;
  size_t file_size=0;
  const size_t MAX_LEN=200000;
  unsigned char buffer[MAX_LEN];

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
  while (grid_stream->number_read() < stop && (bytes_read=grid_stream->read(buffer,MAX_LEN)) != bfstream::eof) {
    if (bytes_read == bfstream::error) {
	std::cerr << "Error reading grid " << grid_stream->number_read() << std::endl;
	exit(1);
    }
    file_size+=bytes_read;
    if (grid_stream->number_read() >= start) {
	source_grid->fill(buffer,Grid::full_grid);
	source_grid->print_ascii(std::cout);
    }
  }
  return 0;
}

} // end namespace gridPrint
