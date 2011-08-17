// C++
#include <iostream>
#include <string>
#include <vector>

// C
#include <math.h>
#include <stdlib.h>
#include <stdio.h>

// Local
#include "Binrec.h"
#include "Channel.h"
#include "FilesystemKVS.h"
#include "ImportBT.h"
#include "Log.h"
#include "utils.h"

void usage()
{
  std::cerr << "Usage:\n";
  std::cerr << "gettile store.kvs UID devicenickname.channel level offset\n";
  std::cerr << "Exiting...\n";
  exit(1);
}

int main(int argc, char **argv)
{
  long long begin_time = millitime();
  char **argptr = argv+1;
  
  if (!*argptr) usage();
  std::string storename = *argptr++;
  
  if (!*argptr) usage();
  int uid = atoi(*argptr++);

  set_log_prefix(string_printf("%d %d ", getpid(), uid));
  
  if (!*argptr) usage();
  std::string full_channel_name = *argptr++;

  if (!*argptr) usage();
  int tile_level = atoi(*argptr++);

  if (!*argptr) usage();
  long long tile_offset = atoll(*argptr++);

  if (*argptr) usage();

	
  // Desired level and offset
  // Translation between tile request and tilestore:
  // tile: level 0 is 512 samples in 512 seconds
  // store: level 0 is 65536 samples in 1 second
  // for tile level 0, we want to get store level 14, which is 65536 samples in 16384 seconds

  // Levels differ by 9 between client and server
  TileIndex client_tile_index = TileIndex(tile_level+9, tile_offset);

  {
    std::string arglist;
    for (int i = 0; i < argc; i++) {
      if (i) arglist += " ";
      arglist += std::string("'")+argv[i]+"'";
    }
    log_f("gettile START: %s (time %.9f-%.9f)",
	  arglist.c_str(), client_tile_index.start_time(), client_tile_index.end_time());
  }
    
  FilesystemKVS store(storename.c_str());

  // 5th ancestor
  TileIndex requested_index = client_tile_index.parent().parent().parent().parent().parent();

  Tile tile;
  TileIndex actual_index;
  Channel ch(store, uid, full_channel_name);
  bool success = ch.read_tile_or_closest_ancestor(requested_index, actual_index, tile);

  std::vector<DataSample<double> > samples;
  
  if (!success) {
    log_f("gettile: no tile found for %s", requested_index.to_string().c_str());
  } else {
    log_f("gettile: requested %s: found %s", requested_index.to_string().c_str(), actual_index.to_string().c_str());
    for (unsigned i = 0; i < tile.double_samples.size(); i++) {
      DataSample<double> &sample=tile.double_samples[i];
      if (client_tile_index.contains_time(sample.time)) samples.push_back(sample);
    }
  }

  bool binned = false;

  if (samples.size() > 512) {
    // Bin
    binned = true;
    std::vector<DataAccumulator<double> > bins(512);
    for (unsigned i = 0; i < samples.size(); i++) {
      DataSample<double> &sample=samples[i];
      bins[floor(client_tile_index.position(sample.time)*512)] += sample;
    }
    samples.clear();
    for (unsigned i = 0; i < bins.size(); i++) {
      if (bins[i].weight > 0) samples.push_back(bins[i].get_sample());
    }
  }
  
  log_f("gettile: finished in %lld msec", millitime() - begin_time);

  double line_break_threshold = client_tile_index.duration() / 512.0 * 4.0; // 4*binsize
  if (!binned && samples.size() > 1) {
    // Find the median distance between samples
    std::vector<double> spacing(samples.size()-1);
    for (size_t i = 0; i < samples.size()-1; i++) {
      spacing[i] = samples[i+1].time - samples[i].time;
    }
    std::sort(spacing.begin(), spacing.end());
    double median_spacing = spacing[samples.size()/2];
    // Set line_break_threshold to larger of 4*median_spacing and 4*bin_size
    line_break_threshold = std::max(line_break_threshold, median_spacing * 4);
  }
  
  if (samples.size()) {
    log_f("gettile: outputting %zd samples", samples.size());
    printf("{");
    printf("\"level\":%d", tile_level);
    printf(",");
    printf("\"offset\":%lld", tile_offset);
    printf(",");
    printf("\"fields\":[\"time\",\"mean\",\"stddev\",\"count\"]");
    printf(",");
    printf("\"data\":[");
    double previous_sample_time = client_tile_index.start_time();
    for (unsigned i = 0; i < samples.size(); i++) {
      if (i) putchar(',');
      // TODO: improve linebreak calculations:
      // 1) observe channel specs line break size from database (expressed in time;  some observations have long time periods and others short)
      // 2) insert breaks at beginning or end of tile if needed
      // 3) should client be the one to decide where line breaks are (if we give it the threshold?)
      if (samples[i].time - previous_sample_time > line_break_threshold) {
	// Insert line break, which has value -1e+308
	printf("[%f,-1e308,0,1],", 0.5*(samples[i].time+previous_sample_time));
      }
      previous_sample_time = samples[i].time;
      // TODO: fix so we never see NAN here!
      printf("[%f,%g,%g,%g]", samples[i].time, samples[i].value, isnan(samples[i].stddev) ? 0 : samples[i].stddev, samples[i].weight);
    }
    if (client_tile_index.end_time() - previous_sample_time > line_break_threshold) {
      printf("[%f,-1e308,0,1],", 0.5*(previous_sample_time + client_tile_index.end_time()));
    }
      
    printf("]");
    printf("}");
  } else {
    log_f("gettile: no samples");
    printf("{}");
  }

  return 0;
}
