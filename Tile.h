#ifndef TILE_INCLUDE_H
#define TILE_INCLUDE_H

#include <string>
#include <math.h>

#include "MysqlQuery.h"

class Tile {
 public:
  enum {TILE_BIN_NUM = 512};
  long long id;
  std::string ch_name;
  int level;
  long long offset;
  long long validation_recid;
  std::string json;
  int user_id;

  Tile(MysqlRow *row);
  
  static double offset_at_level_to_unixtime(long long offset, int level) {
    return level_to_duration(level)*offset;
  }

  static double level_to_duration(int level) {
    return level_to_bin_secs(level)*TILE_BIN_NUM;
  }

  static double level_to_bin_secs(int level) {
    return pow(2.0, level);
  }
};

#endif
