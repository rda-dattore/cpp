#include <iomanip>
#include <deque>
#include <grid.hpp>
#include <utils.hpp>
#include <bits.hpp>

const size_t NUM_KEYS=3;
struct InventoryLine {
  InventoryLine() : key(),grid_type(0),time(0),fcst_hr(0),level_type(0),level1(0),level2(0),param(0),source(0),t_range(0),count() {}

  std::vector<size_t> key;
  short grid_type,time,fcst_hr,level_type,level1,level2,param,source,t_range;
  short count[32];
};
struct QueueEntry {
  QueueEntry() : key() {}

  size_t key[NUM_KEYS];
};

int inventory_grids(std::string input_filename,Grid::Format format) {
  idstream *grid_stream;
  Grid *source_grid;
  size_t num_grids=0;
  int bytes_read,file_size=0;
  short last_month=0,last_year=0;
  bool first=true;
  my::map<InventoryLine> hash_table;
  InventoryLine inv_line;
  size_t search_keys[NUM_KEYS];
  std::vector<size_t> keys;
  std::deque<QueueEntry> key_queue;
  QueueEntry queue_entry;
  size_t num_keys=NUM_KEYS;
  const size_t MAX_LEN=50000;
  unsigned char buffer[MAX_LEN];

  switch (format) {
    case Grid::Format::grib: {
	grid_stream=new InputGRIBStream;
	source_grid=new GRIBGrid;
// print a header for the inventory
	std::cout << " INVENTORY OF GRIB GRID FILE " << input_filename << std::endl << std::endl;
	break;
    }
    case Grid::Format::octagonal: {
	grid_stream=new InputOctagonalGridStream;
	source_grid=new OctagonalGrid;
// print a header for the inventory
	std::cout << " INVENTORY OF DSS OCTAGONAL GRID FILE " << input_filename << std::endl << std::endl;
	break;
    }
    case Grid::Format::tropical: {
	grid_stream=new InputTropicalGridStream;
	source_grid=new TropicalGrid;
// print a header for the inventory
	std::cout << " INVENTORY OF DSS TROPICAL GRID FILE " << input_filename << std::endl << std::endl;
	break;
    }
    case Grid::Format::latlon: {
	grid_stream=new InputLatLonGridStream;
	source_grid=new LatLonGrid;
// print a header for the inventory
	std::cout << " INVENTORY OF DSS 5-DEGREE LAT/LON GRID FILE " << input_filename << std::endl << std::endl;
	break;
    }
    case Grid::Format::navy: {
	grid_stream=new InputNavyGridStream;
	source_grid=new NavyGrid;
// print a header for the inventory
	std::cout << " INVENTORY OF DSS NAVY GRID FILE " << input_filename << std::endl << std::endl;
	break;
    }
    case Grid::Format::slp: {
	grid_stream=new InputSLPGridStream;
	source_grid=new SLPGrid;
// print a header for the inventory
	std::cout << " INVENTORY OF DSS SEA-LEVEL PRESSURE GRID FILE " << input_filename << std::endl << std::endl;
	break;
    }
    default: {
	std::cerr << "Error: unable to create inventory for format " << static_cast<int>(format) << std::endl;
	return 1;
    }
  }
// initializations
  for (size_t n=0; n < NUM_KEYS; ++n) {
    search_keys[n]=0;
  }
// read grid file until it is exhausted
  if (!grid_stream->open(input_filename)) {
    std::cerr << "Error opening " << input_filename << std::endl;
    exit(1);
  }
  while (1) {
// get grid headers
    if ( (bytes_read=grid_stream->read(buffer,MAX_LEN)) > 0)
	source_grid->fill(buffer, Grid::HEADER_ONLY);

    if (bytes_read == bfstream::error) {
	std::cerr << "Error reading grid " << grid_stream->number_read() << std::endl;
	exit(1);
    }
// if this is the first grid, do these initializations
    if (first) {
	std::cout << " THE NUMBERS IN THE INVENTORY TABLES INDICATE THE PRESENCE OF A GRID" << std::endl;
	if (source_grid->is_averaged_grid()) {
	  std::cout << " THESE NUMBERS ARE THE NUMBER OF GRIDS THAT WERE USED TO COMPUTE THE AVERAGE" << std::endl;
	}
	else {
	  std::cout << " THESE NUMBERS ARE THE IDENTIFICATION OF THE CENTER THAT IS THE SOURCE OF THE GRID" << std::endl;
	}
	last_year=source_grid->reference_date_time().year();
	last_month=source_grid->reference_date_time().month();
	first=false;
    }
// if a new month has been encountered for daily grids, or a new year for
// averaged grids, print out the current inventory before continuing
    if (bytes_read == bfstream::eof || (!source_grid->is_averaged_grid() && source_grid->reference_date_time().month() != last_month) || (source_grid->is_averaged_grid() && source_grid->reference_date_time().year() != last_year)) {
// print a header to the inventory lines for the current month or year
	std::cout << std::endl;
	if (source_grid->is_averaged_grid()) {
	  std::cout << " +-----------+" << std::endl;
	  switch (format) {
	    case Grid::Format::octagonal:
	    case Grid::Format::tropical:
	    case Grid::Format::latlon:
	    case Grid::Format::navy: {
		std::cout << " | Year " << std::setw(4) << last_year << " |" << std::endl;
		break;
	    }
	    default: {
		std::cout << " | Year " << std::setw(4) << last_year << " |" << std::endl;
	    }
	  }
	  std::cout << " +-----------+" << std::endl;
	  switch (format) {
	    case Grid::Format::grib: {
		std::cout << " Grid  Reference  Fcst  Level                 Param  Source   Time   ------------------------ MONTHS OF THE YEAR -------------------------" << std::endl;
		std::cout << " Type     Time     Hr    Type  Level1 Level2   Code   Code   Range   JAN   FEB   MAR   APR   MAY   JUN   JUL   AUG   SEP   OCT   NOV   DEC" << std::endl;
		break;
	    }
	    case Grid::Format::octagonal:
	    case Grid::Format::tropical:
	    case Grid::Format::latlon:
	    case Grid::Format::navy: {
		std::cout << " Grid  Analysis  Fcst     Levels     Param  Source  ------------------------ MONTHS OF THE YEAR -------------------------" << std::endl;
		std::cout << " Type    Time     Hr     Top Bottom   Code   Code   JAN   FEB   MAR   APR   MAY   JUN   JUL   AUG   SEP   OCT   NOV   DEC" << std::endl;
		break;
	    }
	    case Grid::Format::slp: {
		std::cout << " Grid  Analysis  Fcst         Param  Source  ------------------------ MONTHS OF THE YEAR -------------------------" << std::endl;
		std::cout << " Type    Time     Hr   Level   Code   Code   JAN   FEB   MAR   APR   MAY   JUN   JUL   AUG   SEP   OCT   NOV   DEC" << std::endl;
		break;
	    }
	    default: { }
	  }
	}
	else {
	  std::cout << " +---------------------+" << std::endl;
	  std::cout << " | Year " << std::setw(4) << last_year << "  Month " << std::setw(2) << last_month << " |" << std::endl;
	  std::cout << " +---------------------+" << std::endl;

	  switch (format) {
	    case Grid::Format::grib: {
		std::cout << " Grid  Reference  Fcst  Level                 Param   Time   ------------------------------------ DAYS OF THE MONTH ------------------------------------" << std::endl;
		std::cout << " Type     Time     Hr    Type  Level1 Level2   Code  Range   1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31" << std::endl;
		break;
	    }
	    case Grid::Format::octagonal:
	    case Grid::Format::tropical:
	    case Grid::Format::latlon:
	    case Grid::Format::navy: {
		std::cout << " Grid  Analysis  Fcst     Levels     Param   ----------------------------------- DAYS OF THE MONTH -------------------------------------" << std::endl;
		std::cout << " Type    Time     Hr     Top Bottom   Code   1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31" << std::endl;
		break;
	    }
	    case Grid::Format::slp: {
		std::cout << " Grid  Analysis  Fcst         Param   ------------------------------------ DAYS OF THE MONTH ------------------------------------" << std::endl;
		std::cout << " Type    Time     Hr   Level   Code   1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31" << std::endl;
		break;
	    }
	    default: { }
	  }
	}
// de-queue the search keys and print out their corresponding inventory lines
	while (!key_queue.empty()) {
	  queue_entry=key_queue.front();
	  key_queue.pop_front();
	  for (size_t n=0; n < NUM_KEYS; ++n) {
	    search_keys[n]=queue_entry.key[n];
	  }
	  keys.resize(num_keys);
	  for (size_t n=0; n < num_keys; ++n) {
	    keys[n]=search_keys[n];
	  }
	  if (!hash_table.found(keys,inv_line)) {
	    std::cerr << "Error: no entry in hash table for key(s)";
	    for (size_t n=0; n < num_keys; ++n) {
		std::cerr << " " << keys[n];
	    }
	    std::cerr << std::endl;
	    return 1;
	  }
	  switch (format) {
	    case Grid::Format::grib: {
		std::cout << std::setw(5) << inv_line.grid_type << "     " << std::setfill('0') << std::setw(4) << inv_line.time << std::setfill(' ') << "    " << std::setw(3) << inv_line.fcst_hr << "    " << std::setw(4) << inv_line.level_type << "  " << std::setw(6) << inv_line.level1 << " " << std::setw(6) << inv_line.level2 << "   " << std::setw(4) << inv_line.param;
		if (source_grid->is_averaged_grid()) {
		  std::cout << "  " << std::setw(5) << inv_line.source;
		}
		std::cout << "  " << std::setw(5) << inv_line.t_range;
		break;
	    }
	    case Grid::Format::octagonal:
	    case Grid::Format::tropical:
	    case Grid::Format::latlon:
	    case Grid::Format::navy: {
		std::cout << std::setw(5) << inv_line.grid_type << "     " << std::setfill('0') << std::setw(2) << inv_line.time*0.01 << std::setfill(' ') << "Z     " << std::setw(2) << inv_line.fcst_hr << "  " << std::setw(6) << inv_line.level1 << " " << std::setw(6) << inv_line.level2 << "   " << std::setw(4) << inv_line.param;
		if (source_grid->is_averaged_grid()) {
		  std::cout << "   " << std::setw(4) << inv_line.source;
		}
		break;
	    }
	    case Grid::Format::slp: {
		std::cout << std::setw(5) << inv_line.grid_type << "     " << std::setfill('0') << std::setw(2) << inv_line.time*0.01 << std::setfill(' ') << "Z     " << std::setw(2) << inv_line.fcst_hr << "  " << std::setw(6) << inv_line.level1 << "   " << std::setw(4) << inv_line.param;
		if (source_grid->is_averaged_grid()) {
		  std::cout << "   " << std::setw(4) << inv_line.source;
		}
		break;
	    }
	    default: { }
	  }
	  if (source_grid->is_averaged_grid()) {
	    for (size_t n=1; n < 13; ++n) {
		if (inv_line.count[n] > 0) {
		  if (inv_line.count[n] == 999) {
		    std::cout << "     ?";
		  }
		  else {
		    std::cout << std::setw(6) << inv_line.count[n];
		  }
		}
		else {
		  std::cout << "      ";
		}
	    }
	  }
	  else {
	    std::cout << " ";
	    for (size_t n=1; n < 32; ++n) {
		if (inv_line.count[n] > 0) {
		  std::cout << std::setw(3) << inv_line.count[n];
		}
		else {
		  std::cout << "   ";
		}
	    }
	  }
	  std::cout << std::endl;
	}
	if (bytes_read == bfstream::eof) {
	  break;
	}
// re-initializations
	hash_table.clear();
	last_year=source_grid->reference_date_time().year();
	last_month=source_grid->reference_date_time().month();
    }
    ++num_grids;
    file_size+=bytes_read;
// build the key to get the proper inventory line
    bits::set(search_keys,source_grid->type(),0,8);
    bits::set(search_keys,source_grid->reference_date_time().time()/100,8,12);
    bits::set(search_keys,source_grid->forecast_time()/10000,20,8);
    bits::set(search_keys,static_cast<short>(source_grid->first_level_value()),28,8);
    bits::set(search_keys,static_cast<short>(source_grid->second_level_value()),36,8);
    bits::set(search_keys,source_grid->parameter(),44,8);
    if (source_grid->is_averaged_grid()) {
	bits::set(search_keys,source_grid->source(),52,8);
    }
    num_keys=2;
    if (format == Grid::Format::grib) {
	bits::set(search_keys,(reinterpret_cast<GRIBGrid *>(source_grid))->first_level_type(),60,8);
	bits::set(search_keys,(reinterpret_cast<GRIBGrid *>(source_grid))->time_range(),68,8);
	num_keys=3;
    }
    keys.resize(num_keys);
    for (size_t n=0; n < num_keys; ++n) {
	keys[n]=search_keys[n];
    }
// search the table for the inventory line with this key
    if (hash_table.found(keys,inv_line)) {
	if (source_grid->is_averaged_grid()) {
	  inv_line.count[source_grid->reference_date_time().month()]=source_grid->number_averaged();
	}
	else {
	  inv_line.count[source_grid->reference_date_time().day()]=source_grid->source();
	}
	hash_table.replace(inv_line);
    }
    else {
	for (size_t n=0; n < NUM_KEYS; ++n) {
	  inv_line.key[n]=queue_entry.key[n]=search_keys[n];
	}
	inv_line.grid_type=source_grid->type();
	inv_line.time=source_grid->reference_date_time().time()/100;
	inv_line.fcst_hr=source_grid->forecast_time()/10000;
	if (format == Grid::Format::grib) {
	  inv_line.level_type=(reinterpret_cast<GRIBGrid *>(source_grid))->first_level_type();
	  inv_line.t_range=(reinterpret_cast<GRIBGrid *>(source_grid))->time_range();
	}
	inv_line.level1=source_grid->first_level_value();
	inv_line.level2=source_grid->second_level_value();
	inv_line.param=source_grid->parameter();
	for (size_t n=0; n < 32; ++n) {
	  inv_line.count[n]=0;
	}
	if (source_grid->is_averaged_grid()) {
	  inv_line.source=source_grid->source();
	  inv_line.count[source_grid->reference_date_time().month()]=source_grid->number_averaged();
	}
	else {
	  inv_line.count[source_grid->reference_date_time().day()]=source_grid->source();
	}
	hash_table.insert(inv_line);
	key_queue.push_back(queue_entry);
    }
  }
// print a trailer
  std::cout << "\n\n SUMMARY:" << std::endl;
  std::cout << "   Grids read from input file: " << std::setw(8) << num_grids << std::endl;
  std::cout << "   File size (bytes):         " << std::setw(9) << file_size << std::endl;
  if (format == Grid::Format::grib) {
    delete reinterpret_cast<GRIBGrid *>(source_grid);
  }
  return 0;
}
