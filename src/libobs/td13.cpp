#include <iomanip>
#include <surface.hpp>
#include <library.hpp>
#include <strutils.hpp>
#include <utils.hpp>

int InputTD13SurfaceReportStream::ignore()
{
return -1;
}

int InputTD13SurfaceReportStream::peek()
{
return -1;
}

int InputTD13SurfaceReportStream::read(unsigned char *buffer,size_t buffer_length)
{
  int num_chars,len;

  if (ics == NULL) {
    std::cerr << "Error: no open InputTD13SurfaceReportStream" << std::endl;
    return -1;
  }
  if (file_buf_pos == file_buf_len) {
std::cerr << "reading next block" << std::endl;
    if ( (num_chars=ics->read(buffer,buffer_length)) < 0) {
	return num_chars;
    }
std::cerr << "length is " << num_chars << std::endl;
    charconversions::ebcdic_to_ascii(reinterpret_cast<char *>(buffer),reinterpret_cast<char *>(buffer),num_chars);
std::cerr << "converted from EBCDIC" << std::endl;
  }
  strutils::strget(reinterpret_cast<char *>(&buffer[file_buf_pos+17]),len,3);
  file_buf_pos+=len;
  ++num_read;
  return len;
}

TD13SurfaceReport::TD13SurfaceReport()
{
}

void TD13SurfaceReport::fill(const unsigned char *stream_buffer,bool fill_header_only)
{
}

void TD13SurfaceReport::print(std::ostream& outs) const
{
}

void TD13SurfaceReport::print_header(std::ostream& outs,bool verbose) const
{
  outs.setf(std::ios::fixed);
  outs.precision(2);
  outs << "ID: " << location_.ID << "  DATE: " << date_time_.to_string("%Y-%m-%d") << "  LAT: " << std::setw(5) << location_.latitude << "  LON: " << std::setw(6) << location_.longitude << "  ELEV: " << std::setw(5) << location_.elevation << std::endl;
}
