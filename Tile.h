#ifndef TILE_INCLUDE_H
#define TILE_INCLUDE_H

// System
#include <string>
#include <vector>

// Local
#include "DataSample.h"
#include "Range.h"
#include "sizes.h"

class Tile {
 public:
  Tile() { header.magic = MAGIC; header.version = 0x00010000; }
  struct Header {
    uint32 magic;
    uint32 version;
  } header;
  DataRanges ranges;
  enum {
    MAGIC = 0x69547442 // Magic('BtTi')
  };
  std::vector<DataSample<double> > double_samples;
  std::vector<DataSample<std::string> > string_samples;
  void to_binary(std::string &ret) const;
  size_t binary_length() const;
  void from_binary(const std::string &binary);
  void insert_samples(const DataSample<double> *begin, const DataSample<double> *end);
  void insert_samples(const DataSample<std::string> *begin, const DataSample<std::string> *end);
  template <class T> std::vector<DataSample<T> > &get_samples();
  double first_sample_time() const;
  double last_sample_time() const;
  std::string summary() const;
  
private:
  size_t double_samples_length() const;
  size_t string_samples_length() const;
};

template <>
inline std::vector<DataSample<double> > &Tile::get_samples<double>() { return double_samples; }
template <>
inline std::vector<DataSample<std::string> > &Tile::get_samples<std::string>() { return string_samples; }

#endif
