// C++
#include <iostream>
#include <string>
#include <vector>

// C
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

// Local
#include "Channel.h"
#include "FilesystemKVS.h"
#include "Log.h"
#include "utils.h"

void usage()
{
  std::cerr << "Usage:\n";
  std::cerr << "export store.kvs uid dev_nickname.ch_name [dev_nickname.ch_name ...]\n";
  std::cerr << "Exiting...\n";
  exit(1);
}

// TODO: export CSV or JSON instead of tab-delimited
bool dump_samples(const Tile &tile, Range requested_times)
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
      if (requested_times.includes(sample.time)) 
	printf("%.9f\t%g\n", sample.time, sample.value);
    } else {
      DataSample<std::string> sample = tile.string_samples[string_index++];
      if (requested_times.includes(sample.time))
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
  set_log_prefix(string_printf("%d %d ", getpid(), uid));

  {
    std::string arglist;
    for (int i = 0; i < argc; i++) {
      if (i) arglist += " ";
      arglist += std::string("'")+argv[i]+"'";
    }
    log_f("export START: %s", arglist.c_str());
  }
  
  std::vector<std::string> channel_full_names;
  
  while (*argptr) {
    channel_full_names.push_back(*argptr++);
  }
  if (!channel_full_names.size()) usage();

  Range times = Range::all();

  FilesystemKVS store(storename.c_str());

  for (unsigned i = 0; i < channel_full_names.size(); i++) {
    std::string &channel_full_name = channel_full_names[i];
    if (i) printf("\f");
    printf("Time\t%s\n", channel_full_name.c_str());
    Channel ch(store, uid, channel_full_name);
    Channel::Locker locker(ch);
    ch.read_tiles_in_range(times, dump_samples, TileIndex::lowest_level());
  }
  return 0;
}
