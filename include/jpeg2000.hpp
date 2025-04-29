// FILE: jpeg2000.hpp

#ifndef JPEG2000_H
#define   JPEG2000_H

#include <iostream>
#include <sstream>
#include <vector>

class JPEG2000CodeStream
{
public:
  struct ResolutionLevel {
    ResolutionLevel() : num_subbands(0),ncb_x(0),ncb_y(0) {}

    short num_subbands,ncb_x,ncb_y;
  };

  JPEG2000CodeStream() : Xsiz(0),Ysiz(0),XTsiz(0),YTsiz(0),Ssiz(0),Scod(0),pc_siz_x(32768),pc_siz_y(32768),code_block_siz_x(0),code_block_siz_y(0),res_levels(),Psot(0),nguard(0),subband_exp_bs(),error_ss(),is_valid_code_stream(false) {}
  operator bool() { return is_valid_code_stream; }
  void decode(const unsigned char *code_stream,size_t len);
  std::string error() const { return error_ss.str(); }

private:
  void decode_COD_segment(const unsigned char *code_stream,size_t& offset,std::stringstream& error_ss);
  void decode_COM_segment(const unsigned char *code_stream,size_t& offset,std::stringstream& error_ss);
  void decode_QCD_segment(const unsigned char *code_stream,size_t& offset,std::stringstream& error_ss);
  void decode_SIZ_segment(const unsigned char *code_stream,size_t& offset,std::stringstream& error_ss);
  void decode_SOD_segment(const unsigned char *code_stream,size_t& offset,std::stringstream& error_ss);
  void decode_SOT_segment(const unsigned char *code_stream,size_t& offset,std::stringstream& error_ss);

  size_t Xsiz,Ysiz,XTsiz,YTsiz;
  short Ssiz;
  short Scod;
  int pc_siz_x,pc_siz_y,code_block_siz_x,code_block_siz_y;
  std::vector<ResolutionLevel> res_levels;
  int Psot;
  short nguard;
  std::vector<short> subband_exp_bs;
  std::stringstream error_ss;
  bool is_valid_code_stream;
};

namespace jpeg2000utils {

size_t ones_in_tag_tree(size_t index_x,size_t index_y,size_t dim_x,size_t dim_y);

} // end namespace jpeg2000utils

#endif
