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
  std::cerr << "copy fromstore.kvs uid dev_nickname.ch_name tostore.kvs [uid] [dev_nickname.ch_name]\n";
  std::cerr << "Exiting...\n";
  exit(1);
}

bool dry_run = false;

Channel *to_ch_ptr = NULL;
size_t total_double_samples;
size_t total_string_samples;

bool copy_samples(const Tile &tile, double min_time, double max_time)
{
  std::vector<DataSample<double> > double_samples;
  std::vector<DataSample<std::string> > string_samples;

  for (unsigned i = 0; i < tile.double_samples.size(); i++) {
    if (min_time <= tile.double_samples[i].time && tile.double_samples[i].time < max_time) {
      double_samples.push_back(tile.double_samples[i]);
    }
  }
  if (!dry_run) to_ch_ptr->add_data(double_samples);

  for (unsigned i = 0; i < tile.string_samples.size(); i++) {
    if (min_time <= tile.string_samples[i].time && tile.string_samples[i].time < max_time) {
      string_samples.push_back(tile.string_samples[i]);
    }
  }
  if (!dry_run) to_ch_ptr->add_data(string_samples);
  
  fprintf(stderr, "Added %zd double samples, %zd string samples\n", double_samples.size(), string_samples.size());
  total_double_samples += double_samples.size();
  total_string_samples += string_samples.size();

  return true;
}

int main(int argc, char **argv)
{
  char **argptr = argv+1;
  long long begin_time = millitime();

  if (!*argptr) usage();
  std::string from_store_name = *argptr++;

  if (!*argptr) usage();
  int from_uid = atoi(*argptr++);
  if (from_uid <= 0) usage();
  set_log_prefix(string_printf("%d %d ", getpid(), from_uid));

  {
    std::string arglist;
    for (int i = 0; i < argc; i++) {
      if (i) arglist += " ";
      arglist += std::string("'")+argv[i]+"'";
    }
    log_f("export START: %s", arglist.c_str());
  }
  
  if (!*argptr) usage();
  std::string from_channel_full_name = *argptr++;

  double min_time = -std::numeric_limits<double>::max();
  //if (*argptr) min_time = atof(*argptr++);

  double max_time = std::numeric_limits<double>::max();
  //if (*argptr) min_time = atof(*argptr++);

  if (!*argptr) usage();
  std::string to_store_name = *argptr++;

  int to_uid = from_uid;
  std::string to_channel_full_name = from_channel_full_name;

  if (*argptr) usage();

  FilesystemKVS from_store(from_store_name.c_str());
  Channel from_ch(from_store, from_uid, from_channel_full_name);

  FilesystemKVS to_store(to_store_name.c_str());
  Channel to_ch(to_store, to_uid, to_channel_full_name);

  log_f("copying from %s %d %s to %s %d %s", 
	from_store_name.c_str(), from_uid, from_channel_full_name.c_str(),
	to_store_name.c_str(), to_uid, to_channel_full_name.c_str());
  
  to_ch_ptr = &to_ch;

  {
    Channel::Locker locker(from_ch);
    from_ch.read_bottommost_tiles_in_range(min_time, max_time, copy_samples);
  }

  log_f("copy: FINISHED in %lld msec.  copied %zd doubles and %zd strings", 
	millitime() - begin_time, total_double_samples, total_string_samples);
  
  return 0;
}


