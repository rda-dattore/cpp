#include <grid.hpp>

int printASCIIGrids(std::string input_filename,size_t format,size_t start,size_t stop)
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
