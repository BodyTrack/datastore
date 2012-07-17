#ifndef INCLUDE_CHANNEL_INFO_H
#define INCLUDE_CHANNEL_INFO_H


// Local includes
#include "Range.h"
#include "sizes.h"
#include "TileIndex.h"

struct ChannelInfo {
  uint32 magic;
  enum {
    MAGIC = 0x68437442 // Magic('BtCh')
  };
  uint32 version;
  Range times;
  TileIndex nonnegative_root_tile_index;
  TileIndex negative_root_tile_index;
};

#endif
