#include <iomanip>
#include <fstream>
#include <regex>
#include <grid.hpp>
#include <gridutils.hpp>
#include <netcdf.hpp>
#include <strutils.hpp>
#include <utils.hpp>
#include <bits.hpp>
#include <buffer.hpp>

namespace gridToNetCDF {

std::string time_range_113_123_cell_methods(DateTime valid_date_time,short tunit,short p1,short p2,int navg)
{
  std::string cell_methods;
  int n;

  if (tunit >= 1 && tunit <= 2) {
    n= (tunit == 2) ? 1 : 24/p2;
    if ( (dateutils::days_in_month(valid_date_time.year(),valid_date_time.month())-navg/n) < 3) {
	cell_methods="time: mean over months";
    }
    else {
	cell_methods="time: mean over intervals of "+strutils::itos(p2)+" "+strutils::to_lower(GRIBGrid::time_units[tunit]);
    }
  }
  else {
    cell_methods="time: mean over intervals of "+strutils::itos(p2)+" "+strutils::to_lower(GRIBGrid::time_units[tunit]);
  }
  return cell_methods;
}

std::string time_range_137_cell_methods(DateTime valid_date_time,short tunit,short p1,short p2,int navg)
{
  std::string cell_methods;

  cell_methods="time: sum over "+strutils::itos(p2-p1)+" "+strutils::to_lower(GRIBGrid::time_units[tunit]);
  if (tunit >= 1 && tunit <= 2) {
    if (navg/4 == static_cast<int>(dateutils::days_in_month(valid_date_time.year(),valid_date_time.month()))) {
	cell_methods+=" time: mean over months";
    }
    else {
	cell_methods+=" time: mean over intervals of "+strutils::itos(p2)+" "+strutils::to_lower(GRIBGrid::time_units[tunit]);
    }
  }
  else {
    cell_methods+=" time: sum over intervals of "+strutils::itos(p2)+" "+strutils::to_lower(GRIBGrid::time_units[tunit]);
  }
  return cell_methods;
}

std::string time_range_138_cell_methods(DateTime valid_date_time,short tunit,short p1,short p2,int navg)
{
  std::string cell_methods;

  cell_methods="time: mean over "+strutils::itos(p2-p1)+" "+strutils::to_lower(GRIBGrid::time_units[tunit]);
  if (tunit >= 1 && tunit <= 2) {
    if (navg/4 == static_cast<int>(dateutils::days_in_month(valid_date_time.year(),valid_date_time.month()))) {
	cell_methods+=" time: mean over months";
    }
    else {
	cell_methods+=" time: mean over intervals of "+strutils::itos(p2)+" "+strutils::to_lower(GRIBGrid::time_units[tunit]);
    }
  }
  else {
    cell_methods+=" time: mean over intervals of "+strutils::itos(p2)+" "+strutils::to_lower(GRIBGrid::time_units[tunit]);
  }
  return cell_methods;
}

std::string GRIB2_cell_methods(GRIB2Grid& grid)
{
  std::string cell_methods;
  size_t n;

  auto spranges=grid.statistical_process_ranges();
  for (n=0; n < spranges.size(); n++) {
    switch (spranges[n].type) {
	case 0:
	  switch (grid.time_unit()) {
	    case 1:
		cell_methods="time: mean over hours";
		break;
	  }
	  break;
	case 1:
	  switch (grid.time_unit()) {
	    case 1:
		cell_methods="time: sum over hours";
		break;
	  }
	  break;
	case 2:
	  switch (grid.time_unit()) {
	    case 1:
		cell_methods="time: maximum over hours";
		break;
	  }
	  break;
	case 3:
	  switch (grid.time_unit()) {
	    case 1:
		cell_methods="time: minimum over hours";
		break;
	  }
	  break;
	case 6:
	  switch (grid.time_unit()) {
	    case 1:
		cell_methods="time: standard_deviation over hours";
		break;
	  }
	  break;
	default:
	  switch (grid.source()) {
	    case 7:
	    case 60:
		switch (spranges[0].type) {
		  case 193:
		  case 194:
//		    cell_methods=time_range_113_123_cell_methods(grid.getEarliestValidDateTime(),grid.time_unit(),(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
cell_methods=time_range_113_123_cell_methods(grid.reference_date_time().hours_added(grid.forecast_time()/10000),grid.time_unit(),(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
		    break;
		  case 204:
//		    cell_methods=time_range_137_cell_methods(grid.getEarliestValidDateTime(),grid.time_unit(),(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
cell_methods=time_range_137_cell_methods(grid.reference_date_time().hours_added(grid.forecast_time()/10000),grid.time_unit(),(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
		    break;
		  case 205:
//		    cell_methods=time_range_138_cell_methods(grid.getEarliestValidDateTime(),grid.time_unit(),(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
cell_methods=time_range_138_cell_methods(grid.reference_date_time().hours_added(grid.forecast_time()/10000),grid.time_unit(),(spranges[0].period_time_increment.value-spranges[1].period_length.value),spranges[0].period_time_increment.value,spranges[0].period_length.value);
		    break;
		  case 255:
		    size_t pcode=grid.discipline()*1000000+grid.parameter_category()*1000+grid.parameter();
		    switch (pcode) {
			case 4:
			  switch (grid.time_unit()) {
			    case 1:
				cell_methods="time: maximum over hours";
				break;
			  }
			  break;
			case 5:
			  switch (grid.time_unit()) {
			    case 1:
				cell_methods="time: minimum over hours";
				break;
			  }
			  break;
		    }
		    break;
		}
		n=spranges.size();
		break;
	  }
    }
  }
  return cell_methods;
}

void set_reference_date_time(const Grid *grid,DateTime& ref_date_time)
{
  ref_date_time=grid->reference_date_time();
  if (ref_date_time.day() == 0) {
    ref_date_time.set_day(1);
  }
  ref_date_time.set_time(0);
}

struct GridEntry {
  GridEntry() : key(),def(),dim() {}

  size_t key;
  Grid::GridDefinition def;
  Grid::GridDimensions dim;
};
struct TimeEntry {
  TimeEntry() : key() {}

  std::string key;
};

std::string build_grib_parameter_ID(GRIBGrid *grid,xmlutils::ParameterMapper& parameter_mapper,bool& is_layer,std::string trange = "")
{
  std::string paramID;

  is_layer=false;
  paramID=grid->parameter_short_name(parameter_mapper)+"_";
  if (grid->second_level_type() > 0 && grid->second_level_type() < 255) {
    is_layer=true;
    paramID+="Y";
  }
  else {
    paramID+="L";
  }
  paramID+=strutils::itos(grid->first_level_type());
  if (trange.length() > 0) {
    paramID+="_"+trange;
  }
  strutils::replace_all(paramID," ","_");
  return paramID;
}

std::string build_grib_individual_level_description(xmlutils::LevelMapper& level_mapper,GRIBGrid *grid)
{
  std::string lev_description;
  std::string sdum;

  lev_description=grid->first_level_description(level_mapper);
  if (grid->second_level_type() > 0 && grid->second_level_type() < 255) {
    if (floatutils::myequalf(static_cast<int>(grid->first_level_value()),grid->first_level_value())) {
	sdum=strutils::itos(grid->first_level_value());
    }
    else {
	sdum=strutils::ftos(grid->first_level_value(),3);
    }
    sdum+=" to ";
    if (floatutils::myequalf(static_cast<int>(grid->second_level_value()),grid->second_level_value())) {
	sdum+=strutils::itos(grid->second_level_value());
    }
    else {
	sdum+=strutils::ftos(grid->second_level_value(),3);
    }
    strutils::replace_all(sdum,"-","B");
    if (strutils::has_ending(lev_description," level")) {
	lev_description=lev_description.substr(0,lev_description.length()-6);
    }
    lev_description+=" layer: "+sdum+" "+grid->first_level_units(level_mapper);
  }
  else if (!floatutils::myequalf(grid->first_level_value(),0.)) {
    if (floatutils::myequalf(static_cast<int>(grid->first_level_value()),grid->first_level_value())) {
	sdum=strutils::itos(grid->first_level_value());
    }
    else {
	sdum=strutils::ftos(grid->first_level_value(),3);
    }
    strutils::replace_all(sdum,"-","B");
    lev_description+=" - value: "+sdum+" "+grid->first_level_units(level_mapper);
  }
  return lev_description;
}

std::string build_grib_general_level_description(xmlutils::LevelMapper& level_mapper,GRIBGrid *grid)
{
  return grid->first_level_description(level_mapper)+"<!>"+grid->first_level_units(level_mapper);
}

std::string get_grib2_time_range(GRIB2Grid& grid)
{
  std::string trange;

  auto spranges=grid.statistical_process_ranges();
  if (spranges.size() > 0) {
    switch (spranges[0].type) {
	case 0:
	{
	  trange="Avg_"+strutils::itos(spranges[0].period_length.unit);
	  break;
	}
	case 1:
	{
	  trange="Accum_"+strutils::itos(spranges[0].period_length.unit);
	  break;
	}
	case 193:
	case 194:
	{
	  if (grid.source() == 7 || grid.source() == 60) {
	    trange="Avg";
	  }
	  else {
	    std::cerr << "Error: no mapping for sprange 0 type " << spranges[0].type << " and center " << grid.source() << std::endl;
	  }
	  break;
	}
	case 195:
	case 196:
	case 204:
	case 206:
	{
	  if (grid.source() == 7 || grid.source() == 60) {
	    trange="AccumAvg";
	  }
	  else {
	    std::cerr << "Error: no mapping for sprange 0 type " << spranges[0].type << " and center " << grid.source() << std::endl;
	  }
	  break;
	}
	case 197:
	case 205:
	case 207:
	{
	  if (grid.source() == 7 || grid.source() == 60) {
	    trange="FcstAvg";
	    if (spranges.size() > 1) {
		auto value=spranges[1].period_length.value;
		std::string units;
		switch (spranges[1].period_length.unit) {
		  case 0: {
		    units="min";
		    break;
		  }
		  case 1: {
		    units="hr";
		    break;
		  }
		  case 2: {
		    units="dy";
		    break;
		  }
		  case 3: {
		    units="mon";
		    break;
		  }
		  case 4: {
		    units="yr";
		    break;
		  }
		  case 10: {
		    value*=3;
		    units="hr";
		    break;
		  }
		  case 11: {
		    value*=6;
		    units="hr";
		    break;
		  }
		  case 12: {
		    value*=12;
		    units="hr";
		    break;
		  }
		  case 13: {
		    units="sec";
		    break;
		  }
		}
		trange+=strutils::itos(value)+units;
	    }
	  }
	  else {
	    std::cerr << "Error: no mapping for sprange 0 type " << spranges[0].type << " and center " << grid.source() << std::endl;
	  }
	  break;
	}
	case 255:
	{
	  if (grid.source() == 7 || grid.source() == 60) {
	    trange="Pd";
	  }
	  else {
	    std::cerr << "Error: no mapping for sprange 0 type " << spranges[0].type << " and center " << grid.source() << std::endl;
	  }
	  break;
	}
	default:
	{
	  std::cerr << "Error: no mapping for sprange 0 type " << spranges[0].type << std::endl;
	  exit(1);
	}
    }
  }
  return trange;
}

void add_parameter_to_unique_tables(GRIBGrid *grid,short edition,gridToNetCDF::GridData& grid_data,gridToNetCDF::HouseKeeping& hk,xmlutils::LevelMapper& level_mapper,my::map<TimeEntry>& unique_time_table,off_t record_offset,off_t record_length)
{
  netCDFStream::UniqueVariableEntry ve;
  netCDFStream::UniqueVariableDataEntry data;
  netCDFStream::UniqueVariableTimeEntry times;
  netCDFStream::UniqueVariableStringEntry se;
  TimeEntry te;
  std::string trange,product_description;
  DateTime firstValidDateTime,lastValidDateTime;
  bool is_layer,fixed_product=false;

  if (edition == 2) {
    trange=get_grib2_time_range(*(reinterpret_cast<GRIB2Grid *>(grid)));
    firstValidDateTime=grid->reference_date_time().hours_added(grid->forecast_time()/10000);
//firstValidDateTime=(reinterpret_cast<GRIB2Grid *>(grid))->getEarliestValidDateTime();
    te.key=firstValidDateTime.to_string("%Y-%m-%d %H:%MM:%SS");
    if (!unique_time_table.found(te.key,te)) {
	unique_time_table.insert(te);
    }
//    lastValidDateTime=firstValidDateTime;
lastValidDateTime=grid->valid_date_time();
    product_description=gributils::grib2_product_description(reinterpret_cast<GRIB2Grid *>(grid),firstValidDateTime,lastValidDateTime);
    size_t idx;
    if ( (idx=product_description.find(" (initial+")) != std::string::npos) {
	product_description=product_description.substr(0,idx);
    }
  }
  ve.key=build_grib_parameter_ID(grid,*(grid_data.parameter_mapper),is_layer,trange);
  if (!hk.unique_variable_table.found(ve.key,ve)) {
    ve.data.reset(new netCDFStream::UniqueVariableEntry::Data);
    ve.data->description=grid->parameter_cf_keyword(*(grid_data.parameter_mapper));
    if (ve.data->description.length() > 0) {
	ve.data->description="CF:"+ve.data->description;
    }
    else {
	ve.data->description=grid->parameter_description(*(grid_data.parameter_mapper));
    }
    ve.data->units=grid->parameter_units(*(grid_data.parameter_mapper));
    ve.data->lev_description=build_grib_individual_level_description(level_mapper,grid);
    if (edition == 2) {
	ve.data->product_description=product_description;
    }
    ve.data->cell_methods=grid_data.cell_methods;
    ve.data->is_layer=is_layer;
    hk.unique_variable_table.insert(ve);
  }
  else if (!fixed_product && edition == 2 && product_description != ve.data->product_description) {
    std::deque<std::string> sp1=strutils::split(product_description);
    std::deque<std::string> sp2=strutils::split(ve.data->product_description);
    if (sp1.size() == sp2.size()) {
	size_t n;
	for (n=1; n < sp1.size(); ++n) {
	  if (sp1[n] != sp2[n]) {
	    break;
	  }
	}
	size_t idx;
	if (n == sp1.size() && (idx=sp1[0].find("-")) != std::string::npos) {
	  ve.data->product_description="over "+sp1[0].substr(idx+1)+"s";
	  for (n=1; n < sp1.size(); ++n) {
	    ve.data->product_description.insert(0,sp1[n]+" ");
	  }
	}
    }
    fixed_product=true;
  }
  se.key=lastValidDateTime.to_string()+":"+(grid->reference_date_time()).to_string();
  if (!ve.data->unique_times.found(se.key,se)) {
    ve.data->unique_times.insert(se);
    times.first_valid_datetime.reset(new DateTime);
    *times.first_valid_datetime=firstValidDateTime;
    times.last_valid_datetime.reset(new DateTime);
    *times.last_valid_datetime=lastValidDateTime;
    times.reference_datetime.reset(new DateTime);
    *times.reference_datetime=grid->reference_date_time();
    ve.data->times.emplace_back(times);
  }
  data.time_entry.reset(new netCDFStream::UniqueVariableTimeEntry);
  data.time_entry->first_valid_datetime.reset(new DateTime);
  *(data.time_entry->first_valid_datetime)=firstValidDateTime;
  data.time_entry->last_valid_datetime.reset(new DateTime);
  *(data.time_entry->last_valid_datetime)=lastValidDateTime;
  data.time_entry->reference_datetime.reset(new DateTime);
  *(data.time_entry->reference_datetime)=grid->reference_date_time();
  data.record_offset.reset(new off_t);
  *data.record_offset=record_offset;
  data.record_length.reset(new off_t);
  *data.record_length=record_length;
  data.level.reset(new netCDFStream::UniqueVariableLevelEntry);
  se.key=strutils::itos(grid->first_level_type());
  (data.level)->key=se.key;
  (data.level)->value.reset(new float);
  *((data.level)->value)=grid->first_level_value();
  (data.level)->value2.reset(new float);
  *((data.level)->value2)=grid->second_level_value();
  se.key+="!"+strutils::ftos(grid->first_level_value(),3)+"!"+strutils::ftos(grid->second_level_value(),3);
  if (!ve.data->unique_levels.found(se.key,se)) {
    ve.data->unique_levels.insert(se);
  }
  if (ve.data->unique_levels.size() == 2) {
    ve.data->lev_description=build_grib_general_level_description(level_mapper,grid);
  }
  ++(ve.data->num_levels);
  ve.data->data.emplace_back(data);
  ve.data->num_gridpoints+=grid->dimensions().size;
}

void scan_latitudes(const Grid *source_grid,float **latrange,float **latnonrec,Grid::LLSubsetDefinition& subset_definition)
{
  std::ifstream ifs;
  char line[32768];
  int n;
  float min_dist=1.0e30,dist,temp;

  if ( (subset_definition.north_latitude-subset_definition.south_latitude) > 200.) {
    *latrange=new float[2];
    (*latrange)[0]=source_grid->definition().slatitude;
    (*latrange)[1]=source_grid->definition().elatitude;
  }
  else {
    *latrange=new float[2];
  }
  subset_definition.min_y=0x7fffffff;
  subset_definition.max_y=0;
  subset_definition.num_y=0;
  if (source_grid->definition().type == Grid::latitudeLongitudeType) {
    *latnonrec=new float[source_grid->dimensions().y];
    for (n=0,subset_definition.num_y=0; n < source_grid->dimensions().y; n++) {
	if ((reinterpret_cast<GRIBGrid *>(const_cast<Grid *>(source_grid)))->scan_mode() == 0x0) {
	  (*latnonrec)[subset_definition.num_y]=source_grid->definition().slatitude-n*source_grid->definition().laincrement;
	}
	else {
	  (*latnonrec)[subset_definition.num_y]=source_grid->definition().slatitude+n*source_grid->definition().laincrement;
	}
	if (subset_definition.south_latitude == subset_definition.north_latitude) {
	  dist=fabsf((*latnonrec)[subset_definition.num_y]-subset_definition.south_latitude);
	  if (dist < min_dist) {
	    min_dist=dist;
	    subset_definition.min_y=subset_definition.max_y=n;
	    (*latrange)[0]=(*latrange)[1]=(*latnonrec)[subset_definition.num_y];
	  }
	}
	else {
	  if (((*latnonrec)[subset_definition.num_y] > subset_definition.south_latitude || floatutils::myequalf((*latnonrec)[subset_definition.num_y],subset_definition.south_latitude,0.0001)) && ((*latnonrec)[subset_definition.num_y] < subset_definition.north_latitude || floatutils::myequalf((*latnonrec)[subset_definition.num_y],subset_definition.north_latitude,0.0001))) {
	    if (n < subset_definition.min_y) {
		subset_definition.min_y=n;
		if ( (subset_definition.north_latitude-subset_definition.south_latitude) < 200.) {
		  if ((reinterpret_cast<GRIBGrid *>(const_cast<Grid *>(source_grid)))->scan_mode() == 0x0) {
		    (*latrange)[1]=(*latnonrec)[subset_definition.num_y];
		  }
		  else {
		    (*latrange)[0]=(*latnonrec)[subset_definition.num_y];
		  }
		}
	    }
	    if (n > subset_definition.max_y) {
		subset_definition.max_y=n;
		if ( (subset_definition.north_latitude-subset_definition.south_latitude) < 200.) {
		  if ((reinterpret_cast<GRIBGrid *>(const_cast<Grid *>(source_grid)))->scan_mode() == 0x0) {
		    (*latrange)[0]=(*latnonrec)[subset_definition.num_y];
		  }
		  else {
		    (*latrange)[1]=(*latnonrec)[subset_definition.num_y];
		  }
		}
	    }
	    subset_definition.num_y++;
	  }
	}
    }
  }
  else if (source_grid->definition().type == Grid::gaussianLatitudeLongitudeType) {
    ifs.open((source_grid->path_to_gaussian_latitude_data()+"/gauslats."+strutils::itos(source_grid->definition().num_circles)).c_str());
    if (!ifs.is_open()) {
	std::cerr << "Error: unable to open gauslats." << source_grid->definition().num_circles << std::endl;
	exit(1);
    }
    *latnonrec=new float[source_grid->definition().num_circles*2];
    ifs.getline(line,32768);
    n=0;
    subset_definition.num_y=0;
    while (!ifs.eof()) {
	(*latnonrec)[subset_definition.num_y]=atof(line);
	if (subset_definition.south_latitude == subset_definition.north_latitude) {
	  dist=fabsf((*latnonrec)[subset_definition.num_y]-subset_definition.south_latitude);
	  if (dist < min_dist) {
	    min_dist=dist;
	    subset_definition.min_y=subset_definition.max_y=n;
	    (*latrange)[0]=(*latrange)[1]=(*latnonrec)[subset_definition.num_y];
	  }
	}
	else {
	  if ((*latnonrec)[subset_definition.num_y] >= subset_definition.south_latitude && (*latnonrec)[subset_definition.num_y] <= subset_definition.north_latitude) {
	    if (n < subset_definition.min_y) {
		subset_definition.min_y=n;
		if ( (subset_definition.north_latitude-subset_definition.south_latitude) < 200.) {
		  (*latrange)[1]=(*latnonrec)[subset_definition.num_y];
		}
	    }
	    if (n > subset_definition.max_y) {
		subset_definition.max_y=n;
		if ( (subset_definition.north_latitude-subset_definition.south_latitude) < 200.) {
		  (*latrange)[0]=(*latnonrec)[subset_definition.num_y];
		}
	    }
	    subset_definition.num_y++;
	  }
	}
	ifs.getline(line,32768);
	++n;
    }
    ifs.close();
  }
  if (subset_definition.south_latitude == subset_definition.north_latitude) {
    (*latnonrec)[0]=(*latrange)[0];
    subset_definition.num_y=1;
  }
  if ((*latrange)[0] > (*latrange)[1]) {
    temp=(*latrange)[0];
    (*latrange)[0]=(*latrange)[1];
    (*latrange)[1]=temp;
  }
}

void scan_longitudes(const Grid *source_grid,float **lonrange,float **lonnonrec,Grid::LLSubsetDefinition& subset_definition)
{
  Grid::GridDefinition def;
  int n;
  float min_dist=1.0e30,dist;
  bool source_lons_are_east=false;

  def=gridutils::fix_grid_definition(source_grid->definition(),source_grid->dimensions());
  subset_definition.crosses_greenwich=false;
  if ( (subset_definition.east_longitude-subset_definition.west_longitude) > 400.) {
    *lonrange=new float[2];
    (*lonrange)[0]=def.slongitude;
    (*lonrange)[1]=def.elongitude;
  }
  else {
    *lonrange=new float[2];
// data organized west to east
    if (def.slongitude < def.elongitude) {
// data are organized by east longitude
	if (def.slongitude >= 0. && def.elongitude >= 0.) {
	  source_lons_are_east=true;
	  if (subset_definition.east_longitude >= 0. && subset_definition.west_longitude < 0.) {
	    if (subset_definition.east_longitude > 0. || (floatutils::myequalf(subset_definition.east_longitude,0.,0.01) && subset_definition.east_longitude >= def.slongitude)) {
// subset crosses greenwich meridian
		subset_definition.crosses_greenwich=true;
		if (subset_definition.east_longitude < 0.) {
		  subset_definition.east_longitude+=360.;
		}
	    }
	    else {
		if (subset_definition.east_longitude <= 0.) {
		  subset_definition.east_longitude+=360.;
		}
	    }
	  }
	  else {
	    if (subset_definition.east_longitude < 0.) {
		subset_definition.east_longitude+=360.;
	    }
	  }
	  if (subset_definition.west_longitude < 0.) {
	    subset_definition.west_longitude+=360.;
	  }
	}
// data are organized by west longitude
	else {
	  if (subset_definition.east_longitude < subset_definition.west_longitude) {
// subset crosses dateline
	    subset_definition.crosses_greenwich=true;
	  }
	}
    }
// data organized east to west
    else {
std::cerr << "Error: unable to handle east-to-west oriented grids" << std::endl;
exit(1);
    }
  }
  *lonnonrec=new float[source_grid->dimensions().x];
  subset_definition.min_x=0x7fffffff;
  subset_definition.max_x=0;
  if (subset_definition.crosses_greenwich) {
    for (n=0,subset_definition.num_x=0; n < source_grid->dimensions().x; ++n) {
	(*lonnonrec)[subset_definition.num_x]=def.slongitude+n*def.loincrement;
	if ((*lonnonrec)[subset_definition.num_x] > subset_definition.west_longitude || floatutils::myequalf((*lonnonrec)[subset_definition.num_x],subset_definition.west_longitude,0.01)) {
	  if ((source_lons_are_east && (*lonnonrec)[subset_definition.num_x] > 180.) || floatutils::myequalf((*lonnonrec)[subset_definition.num_x],180.,0.01)) {
	    (*lonnonrec)[subset_definition.num_x]-=360.;
	  }
	  if (n < subset_definition.min_x) {
	    subset_definition.min_x=n;
	    if ( (subset_definition.east_longitude-subset_definition.west_longitude) < 400.) {
		(*lonrange)[0]=(*lonnonrec)[subset_definition.num_x];
	    }
	  }
	  subset_definition.num_x++;
	}
    }
    for (n=0; n < source_grid->dimensions().x; n++) {
	(*lonnonrec)[subset_definition.num_x]=def.slongitude+n*def.loincrement;
	if ((*lonnonrec)[subset_definition.num_x] < subset_definition.east_longitude || floatutils::myequalf((*lonnonrec)[subset_definition.num_x],subset_definition.east_longitude,0.01)) {
	  if (n > subset_definition.max_x) {
	    subset_definition.max_x=n;
	    if ( (subset_definition.east_longitude-subset_definition.west_longitude) < 400.) {
		(*lonrange)[1]=(*lonnonrec)[subset_definition.num_x];
	    }
	  }
	  subset_definition.num_x++;
	}
    }
  }
  else {
    for (n=0,subset_definition.num_x=0; n < source_grid->dimensions().x; ++n) {
	(*lonnonrec)[subset_definition.num_x]=def.slongitude+n*def.loincrement;
	if (subset_definition.west_longitude == subset_definition.east_longitude) {
	  dist=fabsf((*lonnonrec)[subset_definition.num_x]-subset_definition.west_longitude);
	  if (dist < min_dist) {
	    min_dist=dist;
	    subset_definition.min_x=subset_definition.max_x=n;
	    (*lonrange)[0]=(*lonrange)[1]=(*lonnonrec)[subset_definition.num_x];
	  }
	}
	else {
	  if (((*lonnonrec)[subset_definition.num_x] > subset_definition.west_longitude || floatutils::myequalf((*lonnonrec)[subset_definition.num_x],subset_definition.west_longitude,0.0001)) && ((*lonnonrec)[subset_definition.num_x] < subset_definition.east_longitude || floatutils::myequalf((*lonnonrec)[subset_definition.num_x],subset_definition.east_longitude,0.0001))) {
	    if (n < subset_definition.min_x) {
		subset_definition.min_x=n;
		if ( (subset_definition.east_longitude-subset_definition.west_longitude) < 400.) {
		  (*lonrange)[0]=(*lonnonrec)[subset_definition.num_x];
		}
	    }
	    if (n > subset_definition.max_x) {
		subset_definition.max_x=n;
		if ( (subset_definition.east_longitude-subset_definition.west_longitude) < 400.) {
		  (*lonrange)[1]=(*lonnonrec)[subset_definition.num_x];
		}
	    }
	    subset_definition.num_x++;
	  }
	}
    }
  }
  if (subset_definition.crosses_greenwich) {
    if ((*lonrange)[1] < (*lonrange)[0]) {
	(*lonrange)[0]-=360.;
    }
  }
  else if (subset_definition.west_longitude == subset_definition.east_longitude) {
    (*lonnonrec)[0]=(*lonrange)[0];
    subset_definition.num_x=1;
  }
}

bool compare_times(netCDFStream::UniqueVariableTimeEntry& left,netCDFStream::UniqueVariableTimeEntry& right)
{
  if (*left.first_valid_datetime < *right.first_valid_datetime) {
    return true;
  }
  else if (*left.first_valid_datetime > *right.first_valid_datetime) {
    return false;
  }
  else {
    if (*left.last_valid_datetime < *right.last_valid_datetime) {
	return true;
    }
    else if (*left.last_valid_datetime > *right.last_valid_datetime) {
	return false;
    }
    else {
	if (*left.reference_datetime <= *right.reference_datetime) {
	  return true;
	}
	else {
	  return false;
	}
    }
  }
}

int compare_level_values_default(float& left,float& right)
{
  if (left <= right)
    return -1;
  else
    return 1;
}

int compare_level_values(float& left,float& right)
{
  if (left >= right)
    return -1;
  else
    return 1;
}

bool compare_variable_data(const netCDFStream::UniqueVariableDataEntry& left,const netCDFStream::UniqueVariableDataEntry& right)
{
  int status;
  netCDFStream::UniqueVariableLevelEntry llev,rlev;

  status=compare_times(*left.time_entry,*right.time_entry);
  if (status < 0 && *left.time_entry == *right.time_entry) {
    llev=*left.level;
    rlev=*right.level;
    if (llev.key == "100" && rlev.key == "100")
	status=compare_level_values(*llev.value,*rlev.value);
    else
	status=compare_level_values_default(*llev.value,*rlev.value);
  }
  if (status < 0)
    return true;
  else
    return false;
}

struct LevelListEntry {
  LevelListEntry() : key(),lev_off(0) {}

  std::string key;
  size_t lev_off;
};

std::string level_list_key(netCDFStream::UniqueVariableEntry& ve)
{
  std::string key=strutils::itos(ve.data->num_levels);
  key+="!"+strutils::ftos(*(((ve.data->data.front()).level)->value),3);
  if (ve.data->is_layer) {
    key+="!"+strutils::ftos(*(((ve.data->data.front()).level)->value2),3);
  }
  key+="!"+strutils::ftos(*(((ve.data->data.back()).level)->value),3);
  if (ve.data->is_layer) {
    key+="!"+strutils::ftos(*(((ve.data->data.back()).level)->value2),3);
  }
  return key;
}

std::string convert_units_to_CF(std::string units)
{
  strutils::replace_all(units,"10^","10e");
  strutils::replace_all(units,"^","");
  strutils::replace_all(units,"deg K","K");
  return units;
}

bool is_bounded_time(std::string cell_methods)
{
  size_t idx;
  std::deque<std::string> sp;

  if (strutils::contains(cell_methods,"time: ")) {
    idx=cell_methods.find("time: ");
    sp=strutils::split(cell_methods.substr(idx));
    if (sp.size() > 2 && sp[0] == "time:" && sp[2] == "over") {
	return true;
    }
  }
  return false;
}

void add_times(OutputNetCDFStream& ostream,std::string cell_methods,const DateTime& ref_date_time,size_t *t_dimids)
{
// time
  ostream.add_variable("time",netCDFStream::NcType::FLOAT,0);
  ostream.add_variable_attribute("time","name",std::string("time"));
  ostream.add_variable_attribute("time","long_name",std::string("time"));
  ostream.add_variable_attribute("time","units",ref_date_time.to_string("hours since %Y-%m-%d %H:%MM:00.0 +0:00"));
  ostream.add_variable_attribute("time","calendar","standard");
  t_dimids[0]=0;
// time bounds, if applicable
  if (is_bounded_time(cell_methods)) {
    ostream.add_variable_attribute("time","bounds","time_bnds");
    t_dimids[1]=ostream.dimensions().size()-1;
    ostream.add_variable("time_bnds",netCDFStream::NcType::INT,2,t_dimids);
    t_dimids[2]=ostream.dimensions().size()-2;
  }
  else {
    t_dimids[1]=ostream.dimensions().size()-1;
  }
// valid date/time as YYYYMMDDHH
  if (is_bounded_time(cell_methods)) {
    ostream.add_variable("valid_date_time_range",netCDFStream::NcType::CHAR,3,t_dimids);
    ostream.add_variable_attribute("valid_date_time_range","name","valid_date_time_range");
    ostream.add_variable_attribute("valid_date_time_range","long_name","begin and end valid date and time as YYYYMMDDHH");
    t_dimids[1]=ostream.dimensions().size()-2;
  }
  else {
    ostream.add_variable("valid_date_time",netCDFStream::NcType::CHAR,2,t_dimids);
    ostream.add_variable_attribute("valid_date_time","name","valid_date_time");
    ostream.add_variable_attribute("valid_date_time","long_name","valid date and time as YYYYMMDDHH");
  }
// reference date/time as YYYYMMDDHH
  ostream.add_variable("ref_date_time",netCDFStream::NcType::CHAR,2,t_dimids);
  ostream.add_variable_attribute("ref_date_time","name","ref_date_time");
  ostream.add_variable_attribute("ref_date_time","long_name","reference date and time as YYYYMMDDHH");
// forecast hour
  ostream.add_variable("forecast_hour",netCDFStream::NcType::INT,0);
  ostream.add_variable_attribute("forecast_hour","long_name","forecast hour");
  ostream.add_variable_attribute("forecast_hour","units","hours");
}

void write_netcdf_header_from_grib_file(InputGRIBStream& istream,OutputNetCDFStream& ostream,gridToNetCDF::GridData& grid_data,gridToNetCDF::HouseKeeping& hk,std::string path_to_level_map)
{
  std::ifstream ifs;
  GRIBMessage *msg=nullptr;
  Grid *grid=nullptr;
  short edition=0;
  size_t n,dim_off=0;
  int m;
  std::string sdum,sdum2;
  Grid::GridDefinition ref_def;
  Grid::GridDimensions ref_dim;
  float *latrange=nullptr,*lonrange=nullptr;
  netCDFStream::UniqueVariableEntry ve;
  my::map<TimeEntry> unique_time_table;
  TimeEntry te;
  float *latnonrec=nullptr,*lonnonrec=nullptr,*levnonrec=nullptr,*lev2nonrec=nullptr;
  size_t t_dimids[3];
  gributils::StringEntry se;
  int nsteps=-1;
  std::list<std::list<netCDFStream::UniqueVariableTimeEntry> *> time_dims;
  std::vector<bool> time_dims_bounded;
  std::deque<std::string> sp;
  my::map<LevelListEntry> unique_level_list_table,unique_level_list_table2;
  LevelListEntry lle;
  xmlutils::LevelMapper level_mapper(path_to_level_map);
  int num_levels_in_record=0;
  bool include_parameter=true;
  bool empty_ref_def=true,show_forecast_times=false;

  grid_data.subset_definition.crosses_greenwich=false;
  grid_data.record_flag=-1;
  Buffer buffer;
  int len;
  while ( (len=istream.peek()) > 0) {
    buffer.allocate(len);
    istream.read(&buffer[0],len);
    if (istream.number_read() == 1) {
	bits::get(&buffer[0],edition,56,8);
	switch (edition) {
	  case 1:
	  {
	    msg=new GRIBMessage;
	    break;
	  }
	  case 2:
	  {
	    msg=new GRIB2Message;
	    break;
	  }
	  default:
	  {
	    std::cerr << "Error: unable to convert GRIB edition " << edition << std::endl;
	    exit(1);
	  }
	}
    }
    msg->fill(&buffer[0],true);
    for (n=0; n < msg->number_of_grids(); n++) {
	grid=msg->grid(n);
	if (hk.include_parameter_set != nullptr) {
	  include_parameter=false;
	  if (edition == 2) {
	    auto key=strutils::itos((reinterpret_cast<GRIB2Grid *>(grid))->discipline())+"."+strutils::itos((reinterpret_cast<GRIB2Grid *>(grid))->parameter_category())+"."+strutils::itos(grid->parameter());
	    if (hk.include_parameter_set->find(key) != hk.include_parameter_set->end()) {
		include_parameter=true;
	    }
	  }
	}
	if (include_parameter) {
	  if (istream.number_read() == 1 && empty_ref_def) {
	    set_reference_date_time(grid,grid_data.ref_date_time);
	    if (grid->definition().type == Grid::latitudeLongitudeType || grid->definition().type == Grid::gaussianLatitudeLongitudeType) {
		ref_def=grid->definition();
		ref_dim=grid->dimensions();
	    }
	    else {
		std::cerr << "Error: no conversion for grid type " << grid->definition().type << " yet" << std::endl;
		exit(1);
            }
	    empty_ref_def=false;
	  }
	  else if (grid->definition() != ref_def && (grid->definition().type != ref_def.type || fabs(grid->definition().slatitude-ref_def.slatitude) > 0.001 || fabs(grid->definition().slongitude-ref_def.slongitude) > 0.001 || fabs(grid->definition().elatitude-ref_def.elatitude) > 0.001 || fabs(grid->definition().elongitude-ref_def.elongitude) > 0.001 || fabs(grid->definition().loincrement-ref_def.loincrement) > 0.001 || fabs(grid->definition().laincrement-ref_def.laincrement) > 0.001 || grid->definition().projection_flag != ref_def.projection_flag || grid->definition().num_centers != ref_def.num_centers || fabs(grid->definition().stdparallel1-ref_def.stdparallel1) > 0.001 || fabs(grid->definition().stdparallel2-ref_def.stdparallel2) > 0.001)) {
	    std::cerr << "Error: grid definition changed" << std::endl;
std::cerr.setf(std::ios::fixed);
std::cerr.precision(6);
msg->print_header(std::cerr,false);
std::cerr << istream.number_read() << std::endl;
std::cerr << n << std::endl;
std::cerr << grid->definition().type << " " << ref_def.type << " " << (grid->definition().type == ref_def.type) << std::endl;
std::cerr << grid->definition().slatitude << " " << ref_def.slatitude << " " << fabs(grid->definition().slatitude-ref_def.slatitude) << " " << (grid->definition().slatitude == ref_def.slatitude) << std::endl;
std::cerr << grid->definition().slongitude << " " << ref_def.slongitude << " " << fabs(grid->definition().slongitude-ref_def.slongitude) << " " << (grid->definition().slongitude == ref_def.slongitude) << std::endl;
std::cerr << grid->definition().elatitude << " " << ref_def.elatitude << " " << fabs(grid->definition().elatitude-ref_def.elatitude) << " " << (grid->definition().elatitude == ref_def.elatitude) << std::endl;
std::cerr << grid->definition().elongitude << " " << ref_def.elongitude << " " << fabs(grid->definition().elongitude-ref_def.elongitude) << " " << (grid->definition().elongitude == ref_def.elongitude) << std::endl;
std::cerr << grid->definition().loincrement << " " << ref_def.loincrement << " " << fabs(grid->definition().loincrement-ref_def.loincrement) << " " << (grid->definition().loincrement == ref_def.loincrement) << std::endl;
std::cerr << grid->definition().laincrement << " " << ref_def.laincrement << " " << fabs(grid->definition().laincrement-ref_def.laincrement) << " " << (grid->definition().laincrement == ref_def.laincrement) << std::endl;
std::cerr << grid->definition().projection_flag << " " << ref_def.projection_flag << " " << (grid->definition().projection_flag == ref_def.projection_flag) << std::endl;
std::cerr << grid->definition().num_centers << " " << ref_def.num_centers << " " << (grid->definition().num_centers == ref_def.num_centers) << std::endl;
std::cerr << grid->definition().stdparallel1 << " " << ref_def.stdparallel1 << " " << (grid->definition().stdparallel1 == ref_def.stdparallel1) << std::endl;
std::cerr << grid->definition().stdparallel2 << " " << ref_def.stdparallel2 << " " << (grid->definition().stdparallel2 == ref_def.stdparallel2) << std::endl;
	    exit(1);
	  }
	  else if (grid->dimensions() != ref_dim) {
	    std::cerr << "Error: grid dimensions changed" << std::endl;
	    exit(1);
	  }
	  if (edition == 2) {
	    grid_data.cell_methods=GRIB2_cell_methods(*(reinterpret_cast<GRIB2Grid *>(grid)));
	  }
	  add_parameter_to_unique_tables(reinterpret_cast<GRIBGrid *>(grid),edition,grid_data,hk,level_mapper,unique_time_table,istream.current_record_offset(),msg->length());
	}
    }
  }
  istream.rewind();
// check variables to see if they should be written as record or non-record variables
  for (auto& key : hk.unique_variable_table.keys()) {
    hk.unique_variable_table.found(key,ve);
    if (ve.data->unique_levels.size() > 1) {
	grid_data.record_flag=0;
    }
    else {
	if (nsteps < 0) {
	  nsteps=ve.data->data.size();
	}
	if (static_cast<int>(ve.data->data.size()) != nsteps) {
	  grid_data.record_flag=0;
	}
    }
    for (auto& time : ve.data->times) {
	if (*(time.first_valid_datetime) != *(time.reference_datetime)) {
	  show_forecast_times=true;
	}
    }
    n=0;
    auto time_dim=time_dims.begin();
    for (auto end=time_dims.end(); time_dim != end; ++time_dim) {
	if (ve.data->times == **time_dim) {
	  break;
	}
	++n;
	grid_data.record_flag=0;
    }
    if (time_dim == time_dims.end()) {
	time_dims.emplace_back(&ve.data->times);
	time_dims_bounded.emplace_back(is_bounded_time(ve.data->cell_methods));
    }
    ve.data->dim_ids.emplace_back(n);
  }
  if (grid_data.record_flag == 0 && time_dims.size() == 1) {
    grid_data.record_flag=1;
  }
  if (grid_data.record_flag >= 0) {
    for (auto& key : hk.unique_variable_table.keys()) {
	hk.unique_variable_table.found(key,ve);
	ve.data->data.sort(compare_variable_data);
    }
  }
// global attributes
  ostream.add_global_attribute("Creation date and time",dateutils::current_date_time().to_string());
  ostream.add_global_attribute("Conventions","CF-1.5");
  ostream.add_global_attribute("Creator","NCAR - CISL RDA (dattore)");
// add dimensions
  if (grid_data.record_flag != 0) {
    ostream.add_dimension("time",0);
  }
  else {
    n=0;
    for (auto& time_dim : time_dims) {
	ostream.add_dimension("time"+strutils::itos(n),time_dim->size());
	++n;
    }
  }
// add any level dimensions
  size_t lev_off=0;
  for (auto& key : hk.unique_variable_table.keys()) {
    hk.unique_variable_table.found(key,ve);
    if (ve.data->unique_levels.size() > 1 && static_cast<int>((ve.data->unique_levels.size()*ve.data->unique_times.size())) == ve.data->num_levels) {
	lle.key=level_list_key(ve);
	if (!unique_level_list_table.found(lle.key,lle)) {
	  lle.lev_off=lev_off;
	  if (ve.data->is_layer) {
	    ostream.add_dimension("layer"+strutils::itos(lev_off++),ve.data->unique_levels.size());
	  }
	  else {
	    ostream.add_dimension("level"+strutils::itos(lev_off++),ve.data->unique_levels.size());
	  }
	  if (grid_data.record_flag != 0) {
	    num_levels_in_record+=ve.data->unique_levels.size();
	  }
	  unique_level_list_table.insert(lle);
	}
    }
  }
  grid->set_path_to_gaussian_latitude_data(grid_data.path_to_gauslat_lists);
  scan_latitudes(grid,&latrange,&latnonrec,grid_data.subset_definition);
  ostream.add_dimension("lat",grid_data.subset_definition.num_y);
  scan_longitudes(grid,&lonrange,&lonnonrec,grid_data.subset_definition);
  ostream.add_dimension("lon",grid_data.subset_definition.num_x);
  delete msg;
  ostream.add_dimension("tstrlen",10);
  t_dimids[1]=ostream.dimensions().size()-1;
// add variables
  if (grid_data.record_flag != 0) {
    if (is_bounded_time(grid_data.cell_methods)) {
	ostream.add_dimension("ntb",2);
    }
    add_times(ostream,grid_data.cell_methods,grid_data.ref_date_time,t_dimids);
    dim_off=1;
  }
  else {
    for (n=0; n < time_dims.size(); ++n) {
	if (time_dims_bounded[n]) {
	  ostream.add_dimension("ntb",2);
	  break;
	}
    }
    for (n=0; n < time_dims.size(); ++n) {
	sdum=strutils::itos(n);
	ostream.add_variable("time"+sdum,netCDFStream::NcType::FLOAT,dim_off);
	ostream.add_variable_attribute("time"+sdum,"long_name","time"+sdum);
	ostream.add_variable_attribute("time"+sdum,"units",grid_data.ref_date_time.to_string("hours since %Y-%m-%d %H:%MM:00.0 +0:00"));
	ostream.add_variable_attribute("time"+sdum,"calendar","standard");
	t_dimids[0]=dim_off;
// time bounds, if applicable
	if (time_dims_bounded[n]) {
	  ostream.add_variable_attribute("time"+sdum,"bounds","time_bnds"+sdum);
	  t_dimids[1]=ostream.dimensions().size()-1;
	  ostream.add_variable("time_bnds"+sdum,netCDFStream::NcType::INT,2,t_dimids);
	  t_dimids[2]=ostream.dimensions().size()-2;
	  ostream.add_variable("valid_date_time_range"+sdum,netCDFStream::NcType::CHAR,3,t_dimids);
	  ostream.add_variable_attribute("valid_date_time_range"+sdum,"name","valid_date_time_range");
	  ostream.add_variable_attribute("valid_date_time_range"+sdum,"long_name","begin and end valid date and time as YYYYMMDDHH");
	  t_dimids[1]=t_dimids[2];
	}
	else {
	  ostream.add_variable("valid_date_time"+sdum,netCDFStream::NcType::CHAR,2,t_dimids);
	  ostream.add_variable_attribute("valid_date_time"+sdum,"long_name","valid date and time as YYYYMMDDHH");
	}
	ostream.add_variable("ref_date_time"+sdum,netCDFStream::NcType::CHAR,2,t_dimids);
	ostream.add_variable_attribute("ref_date_time"+sdum,"long_name","reference date and time as YYYYMMDDHH");
	if (show_forecast_times) {
	  ostream.add_variable("forecast_hour"+sdum,netCDFStream::NcType::INT,dim_off);
	  ostream.add_variable_attribute("forecast_hour"+sdum,"long_name","forecast hour");
	  ostream.add_variable_attribute("forecast_hour"+sdum,"units","hours");
	}
	++dim_off;
    }
  }
// levels
  for (auto& key : hk.unique_variable_table.keys()) {
    hk.unique_variable_table.found(key,ve);
    if (ve.data->unique_levels.size() > 1 && static_cast<int>((ve.data->unique_levels.size()*ve.data->unique_times.size())) == ve.data->num_levels) {
	lle.key=level_list_key(ve);
	if (!unique_level_list_table2.found(lle.key,lle)) {
	  unique_level_list_table.found(lle.key,lle);
	  if (ve.data->is_layer) {
	    sdum="layer"+strutils::itos(lle.lev_off);
	  }
	  else {
	    sdum="level"+strutils::itos(lle.lev_off);
	  }
	  sp=strutils::split(ve.data->lev_description,"<!>");
	  if (sp.size() > 1)
	    sdum2=convert_units_to_CF(sp[1]);
	  if (ve.data->is_layer) {
	    ostream.add_variable(sdum+"_top",netCDFStream::NcType::FLOAT,dim_off+lle.lev_off);
	    ostream.add_variable_attribute(sdum+"_top","long_name",sp[0]);
	    if (sp.size() > 1) {
		ostream.add_variable_attribute(sdum+"_top","units",sdum2);
	    }
	    ostream.add_variable(sdum+"_bottom",netCDFStream::NcType::FLOAT,dim_off+lle.lev_off);
	    ostream.add_variable_attribute(sdum+"_bottom","long_name",sp[0]);
	    if (sp.size() > 1) {
		ostream.add_variable_attribute(sdum+"_bottom","units",sdum2);
	    }
	  }
	  else {
	    ostream.add_variable(sdum,netCDFStream::NcType::FLOAT,dim_off+lle.lev_off);
	    ostream.add_variable_attribute(sdum,"long_name",sp[0]);
	    if (sp.size() > 1) {
		ostream.add_variable_attribute(sdum,"units",sdum2);
	    }
	  }
	  unique_level_list_table2.insert(lle);
	}
	ve.data->dim_ids.emplace_back(dim_off+lle.lev_off);
	ve.data->lev_description="";
    }
  }
  dim_off+=unique_level_list_table2.size();
// latitude
  for (auto& key : hk.unique_variable_table.keys()) {
    hk.unique_variable_table.found(key,ve);
    ve.data->dim_ids.emplace_back(dim_off);
  }
  ostream.add_variable("lat",netCDFStream::NcType::FLOAT,dim_off++);
  ostream.add_variable_attribute("lat","name",std::string("lat"));
  ostream.add_variable_attribute("lat","long_name","latitude");
  ostream.add_variable_attribute("lat","units","degree_north");
  ostream.add_variable_attribute("lat","valid_range",netCDFStream::NcType::FLOAT,2,latrange);
  delete[] latrange;
// longitude
  for (auto& key : hk.unique_variable_table.keys()) {
    hk.unique_variable_table.found(key,ve);
    ve.data->dim_ids.emplace_back(dim_off);
  }
  ostream.add_variable("lon",netCDFStream::NcType::FLOAT,dim_off++);
  ostream.add_variable_attribute("lon","name",std::string("lon"));
  ostream.add_variable_attribute("lon","long_name","longitude");
  ostream.add_variable_attribute("lon","units","degree_east");
  ostream.add_variable_attribute("lon","valid_range",netCDFStream::NcType::FLOAT,2,lonrange);
  delete[] reinterpret_cast<float *>(lonrange);
// data variable(s)
  for (auto& key : hk.unique_variable_table.keys()) {
    if (hk.unique_variable_table.found(key,ve)) {
	ostream.add_variable(ve.key,netCDFStream::NcType::FLOAT,ve.data->dim_ids);
	if (strutils::has_beginning(ve.data->description,"CF:")) {
	  ostream.add_variable_attribute(ve.key,"standard_name",ve.data->description.substr(3));
	}
	else {
	  ostream.add_variable_attribute(ve.key,"long_name",ve.data->description);
	}
	if (ve.data->product_description.length() > 0) {
	  ostream.add_variable_attribute(ve.key,"product_description",ve.data->product_description);
	}
	ostream.add_variable_attribute(ve.key,"units",convert_units_to_CF(ve.data->units));
	if (ve.data->lev_description.length() > 0) {
	  ostream.add_variable_attribute(ve.key,"level",ve.data->lev_description);
	}
	else if (ve.data->num_levels > 1) {
	  if (ve.data->is_layer) {
	    ostream.add_variable_attribute(ve.key,"number of vertical layers",static_cast<int>(ve.data->unique_levels.size()));
	  }
	  else {
	    ostream.add_variable_attribute(ve.key,"number of vertical levels",static_cast<int>(ve.data->unique_levels.size()));
	  }
	}
	ostream.add_variable_attribute(ve.key,"_FillValue",static_cast<float>(Grid::missing_value));
	if (grid_data.cell_methods.length() > 0) {
	  ostream.add_variable_attribute(ve.key,"cell_methods",ve.data->cell_methods);
	}
    }
  }
// write the header
  ostream.write_header();
// write the non-record variables
  if (grid_data.record_flag == 0) {
    float *timenonrec=nullptr;
    int *timebndsnonrec=nullptr,*ftimenonrec=nullptr;
    char *vtimenonrec=nullptr,*rtimenonrec=nullptr;
    std::list<netCDFStream::UniqueVariableTimeEntry> *x;
    auto is_monthly_mean=std::regex_search(grid_data.cell_methods,std::regex("over months"));
    n=0;
    for (const auto& time_dim : time_dims) {
	auto time_idx=strutils::itos(n);
	x=time_dim;
	x->sort(compare_times);
	timenonrec=new float[x->size()];
	if (time_dims_bounded[n]) {
	  timebndsnonrec=new int[x->size()*2];
	  vtimenonrec=new char[x->size()*20];
	}
	else {
	  vtimenonrec=new char[x->size()*10];
	}
	rtimenonrec=new char[x->size()*10];
	if (show_forecast_times) {
	  ftimenonrec=new int[x->size()];
	}
	m=0;
	for (auto& item : *x) {
	  timenonrec[m]=item.last_valid_datetime->hours_since(grid_data.ref_date_time);
	  if (time_dims_bounded[n]) {
	    timebndsnonrec[m*2]=item.first_valid_datetime->hours_since(grid_data.ref_date_time);
	    timebndsnonrec[m*2+1]=timenonrec[m];
	    if (is_monthly_mean) {
		timenonrec[m]=(timebndsnonrec[m*2]+timebndsnonrec[m*2+1])/2.;
	    }
	    sdum=item.first_valid_datetime->to_string("%Y%m%d%H");
	    std::copy(sdum.begin(),sdum.begin()+10,&vtimenonrec[m*20]);
	    sdum=item.last_valid_datetime->to_string("%Y%m%d%H");
	    std::copy(sdum.begin(),sdum.begin()+10,&vtimenonrec[m*20+10]);
	  }
	  else {
	    sdum=item.first_valid_datetime->to_string("%Y%m%d%H");
	    std::copy(sdum.begin(),sdum.begin()+10,&vtimenonrec[m*10]);
	  }
	  sdum=item.reference_datetime->to_string("%Y%m%d%H");
	  std::copy(sdum.begin(),sdum.begin()+10,&rtimenonrec[m*10]);
	  if (show_forecast_times) {
	    ftimenonrec[m]=item.first_valid_datetime->hours_since(*(item.reference_datetime));
	  }
	  ++m;
	}
	ostream.write_non_record_data("time"+time_idx,timenonrec);
	if (timebndsnonrec != nullptr) {
	  ostream.write_non_record_data("time_bnds"+time_idx,timebndsnonrec);
	  ostream.write_non_record_data("valid_date_time_range"+time_idx,vtimenonrec);
	}
	else {
	  ostream.write_non_record_data("valid_date_time"+time_idx,vtimenonrec);
	}
	ostream.write_non_record_data("ref_date_time"+time_idx,rtimenonrec);
	if (show_forecast_times) {
	  ostream.write_non_record_data("forecast_hour"+time_idx,ftimenonrec);
	}
	delete[] timenonrec;
	if (timebndsnonrec != nullptr) {
	  delete[] timebndsnonrec;
	  timebndsnonrec=nullptr;
	}
	delete[] vtimenonrec;
	delete[] rtimenonrec;
	if (show_forecast_times) {
	  delete[] ftimenonrec;
	}
	++n;
    }
  }
// levels
  unique_level_list_table2.clear();
  for (auto& key : hk.unique_variable_table.keys()) {
    hk.unique_variable_table.found(key,ve);
    if (ve.data->unique_levels.size() > 1 && static_cast<int>(ve.data->unique_levels.size()*ve.data->unique_times.size()) == ve.data->num_levels) {
	lle.key=level_list_key(ve);
	if (!unique_level_list_table2.found(lle.key,lle)) {
	  unique_level_list_table.found(lle.key,lle);
	  levnonrec=new float[ve.data->unique_levels.size()];
	  if (ve.data->is_layer) {
	    lev2nonrec=new float[ve.data->unique_levels.size()];
	  }
	  m=0;
	  sdum="";
	  for (auto& data : ve.data->data) {
	    if (sdum.length() == 0) {
		sdum=data.level->key;
	    }
	    levnonrec[m]=*(data.level->value);
	    if (ve.data->is_layer) {
		lev2nonrec[m]=*(data.level->value2);
	    }
	    ++m;
	    if (m == static_cast<int>(ve.data->unique_levels.size())) {
		break;
	    }
	  }
	  if (m != static_cast<int>(ve.data->unique_levels.size())) {
	    std::cerr << "Error: incorrect number of levels for coordinate variable 'level" << lle.lev_off << "'" << std::endl;
	    exit(1);
	  }
	  if (ve.data->is_layer) {
	    ostream.write_non_record_data("layer"+strutils::itos(lle.lev_off)+"_top",levnonrec);
	    ostream.write_non_record_data("layer"+strutils::itos(lle.lev_off)+"_bottom",lev2nonrec);
	  }
	  else {
	    ostream.write_non_record_data("level"+strutils::itos(lle.lev_off),levnonrec);
	  }
	  delete[] levnonrec;
	  if (ve.data->is_layer) {
	    delete[] lev2nonrec;
	  }
	}
    }
  }
// lat
  ostream.write_non_record_data("lat",latnonrec);
  delete[] latnonrec;
// lon
  ostream.write_non_record_data("lon",lonnonrec);
  delete[] lonnonrec;
  grid_data.wrote_header=true;
  grid_data.num_parameters_in_file=hk.unique_variable_table.size();
  if (num_levels_in_record > 0) {
    grid_data.num_parameters_in_file*=num_levels_in_record;
  }
}

int convert_grid_to_netcdf(Grid *source_grid,size_t format,OutputNetCDFStream *ostream,gridToNetCDF::GridData& grid_data,std::string path_to_level_map)
{
  std::ifstream ifs;
  std::vector<netCDFStream::Variable> vars;
  GRIBGrid *g=reinterpret_cast<GRIBGrid *>(const_cast<Grid *>(source_grid));
  GRIB2Grid *g2=reinterpret_cast<GRIB2Grid *>(const_cast<Grid *>(source_grid));
  int n,m;
  size_t l;
  int *nonrec;
  float *latnonrec=nullptr,*lonnonrec=nullptr;
  netCDFStream::VariableData gridpoints,nctime,char_data;
  DateTime dateTime,firstValidDateTime,lastValidDateTime;
  int range_values[2];
  float *latrange=nullptr,*lonrange=nullptr;
  size_t dim_ids[3];
  double **lat,**lon;
  std::string trange,product_description,var_name,var_description,var_units,lev_description,sdum;
  size_t t_dimids[3];
  xmlutils::LevelMapper level_mapper(path_to_level_map);
  bool is_layer;

  if (!grid_data.wrote_header) {
    set_reference_date_time(source_grid,grid_data.ref_date_time);
    grid_data.num_parameters_written=0;
  }
  switch (format) {
    case Grid::octagonalFormat:
    {
	if (!grid_data.wrote_header) {
// add global attributes
	  ostream->add_global_attribute("missing_value",static_cast<float>(Grid::missing_value));
// add dimensions
	  ostream->add_dimension("time",0);
	  ostream->add_dimension("yc",51);
	  ostream->add_dimension("xc",47);
// add variables
// time
	  ostream->add_variable("time",netCDFStream::NcType::INT,0);
	  ostream->add_variable_attribute("time","name",std::string("time"));
	  ostream->add_variable_attribute("time","long_name",std::string("time"));
	  ostream->add_variable_attribute("time","units",grid_data.ref_date_time.to_string("hours since %Y-%m-%d %H UTC"));
	  ostream->add_variable_attribute("time","calendar","standard");
// y-coordinate
	  ostream->add_variable("yc",netCDFStream::NcType::INT,1);
	  ostream->add_variable_attribute("yc","name",std::string("yc"));
	  ostream->add_variable_attribute("yc","long_name",std::string("y-coordinate in cartesian system"));
	  ostream->add_variable_attribute("yc","axis",std::string("y"));
	  ostream->add_variable_attribute("yc","units",std::string("m"));
	  range_values[0]=1;
	  range_values[1]=51;
	  ostream->add_variable_attribute("yc","valid_range",netCDFStream::NcType::INT,2,range_values);
// x-coordinate
	  ostream->add_variable("xc",netCDFStream::NcType::INT,2);
	  ostream->add_variable_attribute("xc","name",std::string("xc"));
	  ostream->add_variable_attribute("xc","long_name",std::string("x-coordinate in cartesian system"));
	  ostream->add_variable_attribute("xc","axis",std::string("x"));
	  ostream->add_variable_attribute("xc","units",std::string("m"));
	  range_values[0]=1;
	  range_values[1]=47;
	  ostream->add_variable_attribute("xc","valid_range",netCDFStream::NcType::INT,2,range_values);
// latitudes
	  dim_ids[0]=1;
	  dim_ids[1]=2;
	  ostream->add_variable("lat",netCDFStream::NcType::FLOAT,2,dim_ids);
	  ostream->add_variable_attribute("lat","name",std::string("lat"));
	  ostream->add_variable_attribute("lat","long_name",std::string("north latitude"));
	  ostream->add_variable_attribute("lat","units",std::string("degrees_north"));
// longitudes
	  ostream->add_variable("lon",netCDFStream::NcType::FLOAT,2,dim_ids);
	  ostream->add_variable_attribute("lon","name",std::string("lon"));
	  ostream->add_variable_attribute("lon","long_name",std::string("east longitude"));
	  ostream->add_variable_attribute("lon","units",std::string("degrees_east"));
// octagonal grid variable
	  dim_ids[0]=0;
	  dim_ids[1]=1;
	  dim_ids[2]=2;
	  add_ncar_grid_variable_to_netcdf(ostream,source_grid,3,dim_ids);
//	  gridpoints=new float[2397];
gridpoints.resize(2397,netCDFStream::NcType::FLOAT);
// write the header
	  ostream->write_header();
// write the non-record variables
	  nonrec=new int[51];
	  for (n=0; n < 51; ++n)
	    nonrec[n]=n;
// yc
	  ostream->write_non_record_data("yc",nonrec);
// xc
	  ostream->write_non_record_data("xc",nonrec);
	  lat=new double *[51];
	  lon=new double *[51];
	  for (n=0; n < 51; ++n) {
	    lat[n]=new double[47];
	    lon[n]=new double[47];
	  }
	  gridutils::cartesianll(source_grid->dimensions(),source_grid->definition(),23,25,lat,lon);
// lat
	  for (n=l=0; n < 51; ++n) {
	    for (m=0; m < 47; ++m)
		gridpoints.set(l++,lat[n][m]);
	  }
	  ostream->write_non_record_data("lat",gridpoints.get());
// lon
	  for (n=l=0; n < 51; ++n) {
	    for (m=0; m < 47; ++m)
		gridpoints.set(l++,lon[n][m]);
	  }
	  ostream->write_non_record_data("lon",gridpoints.get());

	  for (n=0; n < 51; ++n) {
	    delete[] lat[n];
	    delete[] lon[n];
	  }
	  delete[] lat;
	  delete[] lon;
	  delete[] nonrec;
	  grid_data.wrote_header=true;
	}
	dateTime=source_grid->reference_date_time();
	if (dateTime.day() == 0) {
	  dateTime.set_day(1);
	}
	if (dateTime.time() == 3100) {
	  dateTime.set_time(0);
	}
	nctime.resize(1,netCDFStream::NcType::INT);
	nctime.set(0,dateTime.hours_since(grid_data.ref_date_time));
	ostream->add_record_data(nctime);
	for (n=l=0; n < 51; n++) {
	  for (m=0; m < 47; m++)
	    gridpoints.set(l++,source_grid->gridpoint(m,n));
	}
	ostream->add_record_data(gridpoints);
      break;
    }
    case Grid::latlonFormat:
    {
	if (!grid_data.wrote_header) {
// add global attributes
	  ostream->add_global_attribute("missing_value",static_cast<float>(Grid::missing_value));
// add dimensions
	  ostream->add_dimension("time",0);
	  ostream->add_dimension("lat",19);
	  ostream->add_dimension("lon",72);
// add variables
// time
	  ostream->add_variable("time",netCDFStream::NcType::INT,0);
	  ostream->add_variable_attribute("time","name",std::string("time"));
	  ostream->add_variable_attribute("time","long_name",std::string("time"));
	  ostream->add_variable_attribute("time","units",grid_data.ref_date_time.to_string("hours since %Y-%m-%d %H UTC"));
	  ostream->add_variable_attribute("time","calendar","standard");
// latitude
	  ostream->add_variable("lat",netCDFStream::NcType::INT,1);
	  ostream->add_variable_attribute("lat","name",std::string("lat"));
	  ostream->add_variable_attribute("lat","long_name",std::string("north latitude"));
	  ostream->add_variable_attribute("lat","units",std::string("degrees_north"));
	  range_values[0]=0;
	  range_values[1]=90;
	  ostream->add_variable_attribute("lat","valid_range",netCDFStream::NcType::INT,2,range_values);
// longitude
	  ostream->add_variable("lon",netCDFStream::NcType::INT,2);
	  ostream->add_variable_attribute("lon","name",std::string("lon"));
	  ostream->add_variable_attribute("lon","long_name",std::string("east longitude"));
	  ostream->add_variable_attribute("lon","units",std::string("degrees_east"));
	  range_values[0]=0;
	  range_values[1]=355;
	  ostream->add_variable_attribute("lon","valid_range",netCDFStream::NcType::INT,2,range_values);
// latlon grid variable
	  dim_ids[0]=0;
	  dim_ids[1]=1;
	  dim_ids[2]=2;
	  add_ncar_grid_variable_to_netcdf(ostream,source_grid,3,dim_ids);
// write the header
	  ostream->write_header();
// write the non-record variables
	  nonrec=new int[72];
	  for (n=0; n < 72; n++)
	    nonrec[n]=n*5;
// lat
	  ostream->write_non_record_data("lat",nonrec);
// lon
	  ostream->write_non_record_data("lon",nonrec);
	  delete[] nonrec;
	  grid_data.wrote_header=true;
	}
	dateTime=source_grid->reference_date_time();
	if (dateTime.day() == 0) {
	  dateTime.set_day(1);
	}
	if (dateTime.time() == 3100) {
	  dateTime.set_time(0);
	}
	nctime.resize(1,netCDFStream::NcType::INT);
	nctime.set(0,dateTime.hours_since(grid_data.ref_date_time));
	ostream->add_record_data(nctime);
	gridpoints.resize(1368,netCDFStream::NcType::FLOAT);
	for (n=l=0; n < 19; ++n) {
	  for (m=0; m < 72; ++m) {
	    gridpoints.set(l++,source_grid->gridpoint(m,n));
	  }
	}
	ostream->add_record_data(gridpoints);
	break;
    }
    case Grid::slpFormat:
    {
	if (!grid_data.wrote_header) {
// add global attributes
// add dimensions
	  ostream->add_dimension("time",0);
	  ostream->add_dimension("lat",16);
	  ostream->add_dimension("lon",72);
// add variables
// time
	  ostream->add_variable("time",netCDFStream::NcType::INT,0);
	  ostream->add_variable_attribute("time","name",std::string("time"));
	  ostream->add_variable_attribute("time","long_name",std::string("time"));
	  ostream->add_variable_attribute("time","units",grid_data.ref_date_time.to_string("hours since %Y-%m-%d %H:%MM:00.0 +0:00"));
	  ostream->add_variable_attribute("time","calendar","standard");
// integer date/time
	  ostream->add_variable("date_time",netCDFStream::NcType::INT,0);
	  ostream->add_variable_attribute("date_time","name","date_time");
	  ostream->add_variable_attribute("date_time","long_name","integer date and time");
	  ostream->add_variable_attribute("date_time","units","YYYYMMDDHH");
// latitude
	  ostream->add_variable("lat",netCDFStream::NcType::INT,1);
	  ostream->add_variable_attribute("lat","name",std::string("lat"));
	  ostream->add_variable_attribute("lat","long_name",std::string("north latitude"));
	  ostream->add_variable_attribute("lat","units",std::string("degrees_north"));
	  range_values[0]=15;
	  range_values[1]=90;
	  ostream->add_variable_attribute("lat","valid_range",netCDFStream::NcType::INT,2,range_values);
// longitude
	  ostream->add_variable("lon",netCDFStream::NcType::INT,2);
	  ostream->add_variable_attribute("lon","name",std::string("lon"));
	  ostream->add_variable_attribute("lon","long_name",std::string("east longitude"));
	  ostream->add_variable_attribute("lon","units",std::string("degrees_east"));
	  range_values[0]=0;
	  range_values[1]=355;
	  ostream->add_variable_attribute("lon","valid_range",netCDFStream::NcType::INT,2,range_values);
// slp
	  dim_ids[0]=0;
	  dim_ids[1]=1;
	  dim_ids[2]=2;
	  add_ncar_grid_variable_to_netcdf(ostream,source_grid,3,dim_ids);
// write the header
	  ostream->write_header();
// write the non-record variables
	  nonrec=new int[72];
	  for (n=0; n < 72; ++n) {
	    nonrec[n]=n*5;
	  }
// lat
	  ostream->write_non_record_data("lat",&nonrec[3]);
// lon
	  ostream->write_non_record_data("lon",nonrec);
	  delete[] nonrec;
	  grid_data.wrote_header=true;
	}
	dateTime=source_grid->reference_date_time();
	if (dateTime.day() == 0) {
	  dateTime.set_day(1);
	}
	if (dateTime.time() == 3100) {
	  dateTime.set_time(0);
	}
	nctime.resize(1,netCDFStream::NcType::INT);
	nctime.set(0,dateTime.hours_since(grid_data.ref_date_time));
	ostream->add_record_data(nctime);
	nctime.set(0,(dateTime.year()*10000+dateTime.month()*100+dateTime.day())*100+dateTime.time()/10000);
	ostream->add_record_data(nctime);
	gridpoints.resize(1152,netCDFStream::NcType::FLOAT);
	for (n=l=0; n < 15; ++n) {
	  for (m=0; m < 72; ++m) {
	    gridpoints.set(l++,source_grid->gridpoint(m,n));
	  }
	}
	for (m=0; m < 72; ++m) {
	  gridpoints.set(l++,source_grid->pole_value());
	}
	ostream->add_record_data(gridpoints);
	break;
    }
    case Grid::gribFormat:
    case Grid::grib2Format:
    {
	if (!grid_data.wrote_header) {
// global attributes
	  ostream->add_global_attribute("Creation date and time",dateutils::current_date_time().to_string());
	  ostream->add_global_attribute("Conventions","CF-1.5");
	  ostream->add_global_attribute("Creator","NCAR - CISL RDA (dattore)");
// cell methods?
	  if (format == Grid::grib2Format) {
	    grid_data.cell_methods=GRIB2_cell_methods(*g2);
	  }
// add global attributes
// add dimensions
	  ostream->add_dimension("time",0);
	  if (source_grid->definition().type == Grid::latitudeLongitudeType || source_grid->definition().type == Grid::gaussianLatitudeLongitudeType) {
	    source_grid->set_path_to_gaussian_latitude_data(grid_data.path_to_gauslat_lists);
	    scan_latitudes(source_grid,&latrange,&latnonrec,grid_data.subset_definition);
	    ostream->add_dimension("lat",grid_data.subset_definition.num_y);
	    grid_data.subset_definition.crosses_greenwich=false;
	    scan_longitudes(source_grid,&lonrange,&lonnonrec,grid_data.subset_definition);
	    ostream->add_dimension("lon",grid_data.subset_definition.num_x);
	  }
	  else {
	    std::cerr << "Error: no conversion for grid type " << source_grid->definition().type << " yet" << std::endl;
	    exit(1);
	  }
	  ostream->add_dimension("tstrlen",10);
	  if (is_bounded_time(grid_data.cell_methods)) {
	    ostream->add_dimension("ntb",2);
	  }
// add variables
	  add_times(*ostream,grid_data.cell_methods,grid_data.ref_date_time,t_dimids);
// latitude
	  ostream->add_variable("lat",netCDFStream::NcType::FLOAT,1);
	  ostream->add_variable_attribute("lat","name",std::string("lat"));
	  ostream->add_variable_attribute("lat","long_name",std::string("north latitude"));
	  ostream->add_variable_attribute("lat","units",std::string("degrees_north"));
	  ostream->add_variable_attribute("lat","valid_range",netCDFStream::NcType::FLOAT,2,latrange);
	  delete[] latrange;
// longitude
	  ostream->add_variable("lon",netCDFStream::NcType::FLOAT,2);
	  ostream->add_variable_attribute("lon","name",std::string("lon"));
	  ostream->add_variable_attribute("lon","long_name",std::string("east longitude"));
	  ostream->add_variable_attribute("lon","units",std::string("degrees_east"));
	  ostream->add_variable_attribute("lon","valid_range",netCDFStream::NcType::FLOAT,2,lonrange);
	  delete[] lonrange;
// data variable
	  dim_ids[0]=0;
	  dim_ids[1]=1;
	  dim_ids[2]=2;
	  sdum=strutils::itos(source_grid->first_level_value());
	  strutils::replace_all(sdum,"-","B");
	  if (format == Grid::grib2Format) {
	    trange=get_grib2_time_range(*g2);
	  }
	  var_name=build_grib_parameter_ID(g,*(grid_data.parameter_mapper),is_layer,trange);
	  if (format == Grid::gribFormat) {
	    var_description=g->parameter_cf_keyword(*grid_data.parameter_mapper);
	    if (var_description.length() == 0) {
		var_description=g->parameter_description(*grid_data.parameter_mapper);
	    }
	    else {
		var_description="CF:"+var_description;
	    }
	    var_units=g->parameter_units(*grid_data.parameter_mapper);
	    lev_description=build_grib_individual_level_description(level_mapper,g);
	  }
	  else {
	    var_description=g2->parameter_cf_keyword(*grid_data.parameter_mapper);
	    if (var_description.length() == 0) {
		var_description=g2->parameter_description(*grid_data.parameter_mapper);
	    }
	    else {
		var_description="CF:"+var_description;
	    }
//	    firstValidDateTime=g2->getEarliestValidDateTime();
firstValidDateTime=g2->reference_date_time().hours_added(g2->forecast_time()/10000);
//	    lastValidDateTime=firstValidDateTime;
lastValidDateTime=g2->valid_date_time();
	    product_description=gributils::grib2_product_description(g2,firstValidDateTime,lastValidDateTime);
	    if ( (l=product_description.find(" (initial+")) != std::string::npos) {
		product_description=product_description.substr(0,l);
	    }
	    var_units=g2->parameter_units(*grid_data.parameter_mapper);
	    lev_description=build_grib_individual_level_description(level_mapper,g2);
	  }
	  ostream->add_variable(var_name,netCDFStream::NcType::FLOAT,3,dim_ids);
	  if (strutils::has_beginning(var_description,"CF:")) {
	    ostream->add_variable_attribute(var_name,"standard_name",var_description.substr(3));
	  }
	  else {
	    ostream->add_variable_attribute(var_name,"long_name",var_description);
	  }
	  if (product_description.length() > 0) {
	    ostream->add_variable_attribute(var_name,"product_description",product_description);
	  }
	  ostream->add_variable_attribute(var_name,"units",convert_units_to_CF(var_units));
	  ostream->add_variable_attribute(var_name,"level",lev_description);
	  ostream->add_variable_attribute(var_name,"_FillValue",static_cast<float>(Grid::missing_value));
	  if (grid_data.cell_methods.length() > 0) {
	    ostream->add_variable_attribute(var_name,"cell_methods",grid_data.cell_methods);
	  }
// write the header
	  ostream->write_header();
// write the non-record variables
// lat
	  ostream->write_non_record_data("lat",latnonrec);
	  delete[] latnonrec;
// lon
	  ostream->write_non_record_data("lon",lonnonrec);
	  delete[] lonnonrec;
	  grid_data.wrote_header=true;
	}
	if (grid_data.num_parameters_written == 0) {
	  dateTime=source_grid->valid_date_time();
	  nctime.resize(1,netCDFStream::NcType::FLOAT);
	  nctime.set(0,dateTime.hours_since(grid_data.ref_date_time));
	  if (is_bounded_time(grid_data.cell_methods)) {
	    dateTime=source_grid->reference_date_time().hours_added(source_grid->forecast_time()/10000);
	    range_values[0]=dateTime.hours_since(grid_data.ref_date_time);
	    range_values[1]=nctime.front();
	    if (strutils::contains(grid_data.cell_methods,"over months")) {
		nctime.set(0,(range_values[0]+range_values[1])/2.);
	    }
	    else {
		nctime.set(0,range_values[1]);
	    }
	    ostream->add_record_data(nctime);
	    nctime.resize(2,netCDFStream::NcType::INT);
	    nctime.set(0,range_values[0]);
	    nctime.set(1,range_values[1]);
	    ostream->add_record_data(nctime);
	    sdum=dateTime.to_string("%Y%m%d%H");
	    dateTime=source_grid->valid_date_time();
	    sdum+=dateTime.to_string("%Y%m%d%H");
	    char_data.resize(sdum.length(),netCDFStream::NcType::CHAR);
	    for (n=0; n < static_cast<int>(sdum.length()); ++n) {
		char_data.set(n,sdum[n]);
	    }
	    ostream->add_record_data(char_data);
	  }
	  else {
	    ostream->add_record_data(nctime);
	    sdum=dateTime.to_string("%Y%m%d%H  ");
	    char_data.resize(sdum.length(),netCDFStream::NcType::CHAR);
	    for (n=0; n < static_cast<int>(sdum.length()); ++n) {
		char_data.set(n,sdum[n]);
	    }
	    ostream->add_record_data(char_data);
	  }
	  dateTime=source_grid->reference_date_time();
	  sdum=dateTime.to_string("%Y%m%d%H  ");
	  char_data.resize(sdum.length(),netCDFStream::NcType::CHAR);
	  for (n=0; n < static_cast<int>(sdum.length()); ++n) {
	    char_data.set(n,sdum[n]);
	  }
	  ostream->add_record_data(char_data);
	  nctime.resize(1,netCDFStream::NcType::INT);
	  nctime.set(0,source_grid->valid_date_time().hours_since(source_grid->reference_date_time()));
	  ostream->add_record_data(nctime);
	}
	gridpoints.resize(source_grid->dimensions().x*source_grid->dimensions().y,netCDFStream::NcType::FLOAT);
	auto precision_set=pow(10.,reinterpret_cast<GRIBGrid *>(source_grid)->decimal_scale_factor());
	if (g->scan_mode() == 0x0) {
	  for (n=grid_data.subset_definition.min_y,l=0; n <= grid_data.subset_definition.max_y; ++n) {
	    if (grid_data.subset_definition.crosses_greenwich) {
		for (m=grid_data.subset_definition.min_x; m < source_grid->dimensions().x; ++m) {
		  gridpoints.set(l++,lround(source_grid->gridpoint(m,n)*precision_set)/precision_set);
		}
		for (m=0; m <= grid_data.subset_definition.max_x; ++m) {
		  gridpoints.set(l++,lround(source_grid->gridpoint(m,n)*precision_set)/precision_set);
		}
	    }
	    else {
		for (m=grid_data.subset_definition.min_x; m <= grid_data.subset_definition.max_x; ++m) {
		  gridpoints.set(l++,lround(source_grid->gridpoint(m,n)*precision_set)/precision_set);
		}
	    }
	  }
	}
	else {
std::cerr << "Error: unable to handle scan mode " << g->scan_mode() << std::endl;
exit(1);
	}
	ostream->add_record_data(gridpoints,l);
	grid_data.num_parameters_written++;
	if (grid_data.num_parameters_written == grid_data.num_parameters_in_file) {
	  grid_data.num_parameters_written=0;
	}
	break;
    }
    case Grid::cgcm1Format:
    {
	std::cerr << "Error: unable to convert CGCM1 grids to netCDF format" << std::endl;
	return -1;
    }
    case Grid::gpcpFormat:
    {
	GPCPGrid *g=reinterpret_cast<GPCPGrid *>(const_cast<Grid *>(source_grid));
	if (!grid_data.wrote_header) {
// global attributes
	  ostream->add_global_attribute("Creation date and time",dateutils::current_date_time().to_string());
	  ostream->add_global_attribute("Conventions","CF-1.5");
	  ostream->add_global_attribute("Creator","NCAR - CISL RDA (dattore)");
// add dimensions
	  ostream->add_dimension("time",0);
	  scan_latitudes(source_grid,&latrange,&latnonrec,grid_data.subset_definition);
	  ostream->add_dimension("lat",grid_data.subset_definition.num_y);
	  grid_data.subset_definition.crosses_greenwich=false;
	  scan_longitudes(source_grid,&lonrange,&lonnonrec,grid_data.subset_definition);
	  ostream->add_dimension("lon",grid_data.subset_definition.num_x);
// add variables
// time
	  ostream->add_variable("time",netCDFStream::NcType::FLOAT,0);
	  ostream->add_variable_attribute("time","name",std::string("time"));
	  ostream->add_variable_attribute("time","long_name",std::string("time"));
	  grid_data.ref_date_time=source_grid->valid_date_time();
	  ostream->add_variable_attribute("time","units",grid_data.ref_date_time.to_string("hours since %Y-%m-%d %H:%MM:00.0 +0:00"));
	  ostream->add_variable_attribute("time","calendar","standard");
// latitude
	  ostream->add_variable("lat",netCDFStream::NcType::FLOAT,1);
	  ostream->add_variable_attribute("lat","name",std::string("lat"));
	  ostream->add_variable_attribute("lat","long_name",std::string("north latitude"));
	  ostream->add_variable_attribute("lat","units",std::string("degrees_north"));
	  ostream->add_variable_attribute("lat","valid_range",netCDFStream::NcType::FLOAT,2,latrange);
	  delete[] latrange;
// longitude
	  ostream->add_variable("lon",netCDFStream::NcType::FLOAT,2);
	  ostream->add_variable_attribute("lon","name",std::string("lon"));
	  ostream->add_variable_attribute("lon","long_name",std::string("east longitude"));
	  ostream->add_variable_attribute("lon","units",std::string("degrees_east"));
	  ostream->add_variable_attribute("lon","valid_range",netCDFStream::NcType::FLOAT,2,lonrange);
	  delete[] lonrange;
// data variable
	  dim_ids[0]=0;
	  dim_ids[1]=1;
	  dim_ids[2]=2;
	  ostream->add_variable(g->parameter_name(),netCDFStream::NcType::FLOAT,3,dim_ids);
	  ostream->add_variable_attribute(g->parameter_name(),"units",g->parameter_units());
	  ostream->add_variable_attribute(g->parameter_name(),"_FillValue",-99999.);
// write the header
	  ostream->write_header();
// write the non-record variables
// lat
	  ostream->write_non_record_data("lat",latnonrec);
	  delete[] latnonrec;
// lon
	  ostream->write_non_record_data("lon",lonnonrec);
	  delete[] lonnonrec;
	  grid_data.wrote_header=true;
	}
	nctime.resize(1,netCDFStream::NcType::FLOAT);
	nctime.set(0,source_grid->valid_date_time().hours_since(grid_data.ref_date_time));
	ostream->add_record_data(nctime);
	gridpoints.resize(source_grid->dimensions().x*source_grid->dimensions().y,netCDFStream::NcType::FLOAT);
	for (n=grid_data.subset_definition.min_y,l=0; n <= grid_data.subset_definition.max_y; ++n) {
	    for (m=grid_data.subset_definition.min_x; m <= grid_data.subset_definition.max_x; ++m) {
		gridpoints.set(l++,source_grid->gridpoint(m,n));
	    }
	}
	ostream->add_record_data(gridpoints,l);
	break;
    }
  }
  return 0;
}

short extract_record(FILE *fp,netCDFStream::UniqueVariableDataEntry data,unsigned char **buffer,int& buf_len)
{
  short edition;
  fseeko(fp,*(data.record_offset),SEEK_SET);
  if (*(data.record_length) > buf_len) {
    if (*buffer != nullptr) {
	delete[] *buffer;
    }
    buf_len=*(data.record_length);
    *buffer=new unsigned char[buf_len];
  }
  fread(*buffer,1,*(data.record_length),fp);
  bits::get(*buffer,edition,56,8);
  return edition;
}

void convert_grib_file_to_netcdf_as_non_record_variables(FILE *fp,OutputNetCDFStream& ostream,Grid::LLSubsetDefinition& subset_definition,xmlutils::ParameterMapper& parameter_mapper,my::map<netCDFStream::UniqueVariableEntry>& unique_variable_table)
{
  unsigned char *buffer=nullptr;
  int buf_len=0;
  netCDFStream::UniqueVariableEntry ve;
  GRIBMessage *msg=nullptr;
  Grid *grid;
  short edition;
  std::string trange,paramID;
  float *gridpoints=nullptr;
  size_t num_gridpoints=0,npoints;
  short n,m,max_x;
  int l;
  bool is_layer,greenwich_appears_twice=false;

  for (auto& key : unique_variable_table.keys()) {
    unique_variable_table.found(key,ve);
    ostream.initialize_non_record_data(ve.key);
    for (auto& data : ve.data->data) {
	edition=extract_record(fp,data,&buffer,buf_len);
	if (edition == 1) {
	  if (msg == nullptr) {
	    msg=new GRIBMessage;
	  }
	}
	else if (edition == 2) {
	  if (msg == nullptr) {
	    msg=new GRIB2Message;
	  }
	}
	else {
	  std::cerr << "Error: GRIB edition '" << edition << "' is not supported" << std::endl;
	  exit(1);
	}
	msg->fill(buffer,false);
	for (l=0; l < static_cast<int>(msg->number_of_grids()); ++l) {
	  grid=msg->grid(l);
	  auto precision_set=pow(10.,reinterpret_cast<GRIBGrid *>(grid)->decimal_scale_factor());
	  trange="";
	  if (edition == 2) {
	    trange=get_grib2_time_range(*(reinterpret_cast<GRIB2Grid *>(grid)));
	  }
	  paramID=build_grib_parameter_ID(reinterpret_cast<GRIBGrid *>(grid),parameter_mapper,is_layer,trange);
	  if (paramID == ve.key) {
	    if ( (npoints=grid->dimensions().size) > num_gridpoints) {
		if (gridpoints != nullptr) {
		  delete[] gridpoints;
		}
		num_gridpoints=npoints;
		gridpoints=new float[num_gridpoints];
	    }
// check for greenwich meridian existing twice (i.e. 0 -> 360)
	    if (subset_definition.crosses_greenwich) {
		greenwich_appears_twice=false;
		if (floatutils::myequalf(grid->definition().slongitude,grid->definition().elongitude-360.,0.0001)) {
		  greenwich_appears_twice=true;
		}
	    }
	    npoints=0;
	    for (n=subset_definition.min_y; n <= subset_definition.max_y; n++) {
		if (subset_definition.crosses_greenwich) {
		  max_x= (greenwich_appears_twice) ? grid->dimensions().x-1 : grid->dimensions().x;
		  for (m=subset_definition.min_x; m < max_x; ++m) {
		    gridpoints[npoints++]=lround(grid->gridpoint(m,n)*precision_set)/precision_set;
		  }
		  for (m=0; m <= subset_definition.max_x; ++m) {
		    gridpoints[npoints++]=lround(grid->gridpoint(m,n)*precision_set)/precision_set;
		  }
		}
		else {
		  for (m=subset_definition.min_x; m <= subset_definition.max_x; ++m) {
		    gridpoints[npoints++]=lround(grid->gridpoint(m,n)*precision_set)/precision_set;
		  }
		}
	    }
	    ostream.write_partial_non_record_data(gridpoints,npoints);
	  }
	}
    }
  }
  delete[] gridpoints;
  delete[] buffer;
  delete msg;
}

void convert_grib_file_to_netcdf_as_record_variables(FILE *fp,OutputNetCDFStream& ostream,DateTime& ref_date_time,std::string& cell_methods,Grid::LLSubsetDefinition& subset_definition,xmlutils::ParameterMapper& parameter_mapper,std::string path_to_level_map,my::map<netCDFStream::UniqueVariableEntry>& unique_variable_table)
{
  unsigned char *buffer=nullptr;
  int buf_len=0;
  GRIBMessage *msg=nullptr;
  Grid *grid=nullptr;
  short edition;
  netCDFStream::UniqueVariableEntry ve;
  std::deque<std::list<netCDFStream::UniqueVariableDataEntry>::iterator> it,end;
  gridToNetCDF::GridData grid_data;
  size_t format;
  int n,m,l,x;
  std::string trange,paramID;
  bool is_layer,no_more_records=false;

  grid_data.ref_date_time=ref_date_time;
  grid_data.cell_methods=cell_methods;
  grid_data.subset_definition=subset_definition;
  grid_data.wrote_header=true;
  grid_data.num_parameters_in_file=0;
  for (auto& key : unique_variable_table.keys()) {
    unique_variable_table.found(key,ve);
    it.emplace_back(ve.data->data.begin());
    end.emplace_back(ve.data->data.end());
    grid_data.num_parameters_in_file+=ve.data->unique_levels.size();
  }
  while (1) {
    x=0;
    for (auto& key : unique_variable_table.keys()) {
	unique_variable_table.found(key,ve);
	if (it[x] == end[x]) {
	  no_more_records=true;
	  break;
	}
	for (n=0; n < static_cast<int>(ve.data->unique_levels.size()); ++n) {
	  edition=extract_record(fp,*(it[x]),&buffer,buf_len);
	  if (edition == 1) {
	    if (msg == nullptr) {
		msg=new GRIBMessage;
	    }
	    format=Grid::gribFormat;
	  }
	  else if (edition == 2) {
	    if (msg == nullptr) {
		msg=new GRIB2Message;
	    }
	    format=Grid::grib2Format;
	  }
	  else {
	    std::cerr << "Error: GRIB edition '" << edition << "' is not supported" << std::endl;
	    exit(1);
	  }
	  msg->fill(buffer,false);
	  l=msg->number_of_grids();
	  for (m=0; m < l; ++m) {
	    grid=msg->grid(m);
	    trange="";
	    if (edition == 2) {
		trange=get_grib2_time_range(*(reinterpret_cast<GRIB2Grid *>(grid)));
	    }
	    if (l > 1) {
		paramID=build_grib_parameter_ID(reinterpret_cast<GRIBGrid *>(grid),parameter_mapper,is_layer,trange);
	    }
	    if (l == 1 || paramID == ve.key) {
		convert_grid_to_netcdf(grid,format,&ostream,grid_data,path_to_level_map);
	    }
	  }
	  ++(it[x]);
	}
	++x;
    }
    if (no_more_records) {
	break;
    }
  }
  delete[] buffer;
  delete msg;
}

void convert_grib_file_to_netcdf(std::string filename,OutputNetCDFStream& ostream,DateTime& ref_date_time,std::string& cell_methods,Grid::LLSubsetDefinition& subset_definition,xmlutils::ParameterMapper& parameter_mapper,std::string path_to_level_map,my::map<netCDFStream::UniqueVariableEntry>& unique_variable_table,int record_flag)
{
  auto fp=fopen(filename.c_str(),"r");
  if (fp == nullptr) {
    std::cerr << "Error: unable to open " << filename << std::endl;
    exit(1);
  }
  if (record_flag == 0) {
    convert_grib_file_to_netcdf_as_non_record_variables(fp,ostream,subset_definition,parameter_mapper,unique_variable_table);
  }
  else if (record_flag == 1) {
    convert_grib_file_to_netcdf_as_record_variables(fp,ostream,ref_date_time,cell_methods,subset_definition,parameter_mapper,path_to_level_map,unique_variable_table);
  }
  else {
    std::cerr << "Error: record flag '" << record_flag << "' is not defined" << std::endl;
    exit(1);
  }
  fclose(fp);
}

} // end namespace gridToNetCDF
