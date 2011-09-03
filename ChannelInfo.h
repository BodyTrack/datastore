#ifndef INCLUDE_CHANNEL_INFO_H
#define INCLUDE_CHANNEL_INFO_H

// BOOST
#include <boost/cstdint.hpp>

// Local includes
#include "Range.h"
#include "TileIndex.h"

struct ChannelInfo {
  boost::uint32_t magic;
  enum {
    MAGIC = 0x68437442 // Magic('BtCh')
  };
  boost::uint32_t version;
  Range times;
  TileIndex nonnegative_root_tile_index;
  TileIndex negative_root_tile_index;
};

#endif
