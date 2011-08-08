// C++
#include <iostream>
#include <string>
#include <vector>

// C
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

// Local
#include "Channel.h"
#include "FilesystemKVS.h"
#include "utils.h"

void usage()
{
  std::cerr << "Usage:\n";
  std::cerr << "import store.kvs uid dev_nickname.ch_name [min-time [max-time]]\n";
  std::cerr << "Exiting...\n";
  exit(1);
}

// TODO: export CSV or JSON instead of tab-delimited
bool dump_samples(const Tile &tile, double min_time, double max_time)
{
  unsigned double_index = 0, string_index = 0;

  while (1) {
    bool select_double;
    if (double_index >= tile.double_samples.size()) {
      // no more double samples
      if (string_index >= tile.string_samples.size()) {
        // no more samples at all; done
      break;
      }
      select_double = false;
    } else if (string_index >= tile.string_samples.size()) {
      // no more string samples
      select_double = true;
    } else {
      // Select the earliest sample from double or string
      select_double = tile.double_samples[double_index].time <= tile.string_samples[string_index].time;
    }
    
    if (select_double) {
      DataSample<double> sample = tile.double_samples[double_index++];
      if (sample.time < min_time || sample.time >= max_time) break;
      printf("%.9f\t%g\n", sample.time, sample.value);
    } else {
      DataSample<std::string> sample = tile.string_samples[string_index++];
      if (sample.time < min_time || sample.time >= max_time) break;
      printf("%.9f\t%s\n", sample.time, sample.value.c_str());
    }
  }
  return true;
}

int main(int argc, char **argv)
{
  char **argptr = argv+1;

  if (!*argptr) usage();
  std::string storename = *argptr++;

  if (!*argptr) usage();
  int uid = atoi(*argptr++);
  if (uid <= 0) usage();
  
  if (!*argptr) usage();
  std::string channel_full_name = *argptr++;

  double min_time = -std::numeric_limits<double>::max();
  if (*argptr) min_time = atof(*argptr++);

  double max_time = std::numeric_limits<double>::max();
  if (*argptr) min_time = atof(*argptr++);

  if (*argptr) usage();

  fprintf(stderr, "Opening store %s\n", storename.c_str());
  FilesystemKVS store(storename.c_str());

  Channel ch(store, uid, channel_full_name);

  ch.read_bottommost_tiles_in_range(min_time, max_time, dump_samples);
  return 0;
}


