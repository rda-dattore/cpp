#ifndef BITMAP_H
#define   BITMAP_H

#include <string>
#include <vector>
#include <map>

namespace bitmap {

extern void compress(const std::string& uncompressed_bitmap, std::string&
    compressed_bitmap);
extern void compress_values(const std::vector<size_t>& values, std::string&
    compressed_bitmap);
extern void expand_box1d_bitmap(const std::string& box1d_bitmap, char
    **expanded_box1d_bitmap);
extern void further_compress(const std::string& compressed_bitmap, std::string&
    further_compressed_bitmap);
extern void uncompress(const std::string& compressed_bitmap, size_t&
    first_value, std::string& uncompressed_bitmap);
extern void uncompress_values(const std::string& compressed_bitmap, std::vector<
    size_t>& values);

extern std::string add_box1d_bitmaps(const std::string& box1d_bitmap1, const
    std::string& box1d_bitmap2);

extern bool contains_value(const std::string& compressed_bitmap, size_t
    value_to_find);

namespace longitudeBitmap {

struct bitmap_gap {
  bitmap_gap() : length(0), start(-1), end(-1) { }

  int length, start, end;
};

void west_east_bounds(const float *longitude_bitmap, size_t& west_index, size_t&
    east_index);

} // end namespace longitudeBitmap

} // end namespace bitmap

#endif
