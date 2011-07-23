#ifndef TILE_INCLUDE_H
#define TILE_INCLUDE_H

// System
#include <string>
#include <vector>

// BOOST
#include <boost/cstdint.hpp>

// Local
#include "DataSample.h"

class Tile {
 public:
  Tile() { header.magic = MAGIC; header.version = 0x00010000; }
  struct {
    boost::uint32_t magic;
    boost::uint32_t version;
  } header;
  enum {
    MAGIC = 0x69547442 // Magic('BtTi')
  };
  std::vector<DataSample<double> > double_samples;
  void to_binary(std::string &ret) const;
  size_t binary_length() const;
  void from_binary(const std::string &binary);
  void insert_double_samples(const DataSample<double> *begin, const DataSample<double> *end);
};

#endif
