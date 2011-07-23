#ifndef INCLUDE_CHANNEL_INFO_H
#define INCLUDE_CHANNEL_INFO_H

// BOOST
#include <boost/cstdint.hpp>

// Local includes
#include "TileIndex.h"

struct ChannelInfo {
  boost::uint32_t magic;
  enum {
    MAGIC = 0x68437442 // Magic('BtCh')
  };
  boost::uint32_t version;
  double min_time;
  double max_time;
  TileIndex nonnegative_root_tile_index;
  TileIndex negative_root_tile_index;
};

#endif
