// C++
#include <iostream>
#include <set>
#include <string>
#include <vector>

// C
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

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

template <typename T>
void read_tile_samples(KVS &store, int uid, std::string full_channel_name, TileIndex requested_index, TileIndex client_tile_index, std::vector<DataSample<T> > &samples, bool &binned)
{
  Channel ch(store, uid, full_channel_name);
  Tile tile;
  TileIndex actual_index;
  bool success = ch.read_tile_or_closest_ancestor(requested_index, actual_index, tile);

  if (!success) {
    log_f("gettile: no tile found for %s", requested_index.to_string().c_str());
  } else {
    log_f("gettile: requested %s: found %s", requested_index.to_string().c_str(), actual_index.to_string().c_str());
    for (unsigned i = 0; i < tile.get_samples<T>().size(); i++) {
      DataSample<T> &sample=tile.get_samples<T>()[i];
      if (client_tile_index.contains_time(sample.time)) samples.push_back(sample);
    }
  }

  if (samples.size() <= 512) {
    binned = false;
  } else {
    // Bin
    binned = true;
    std::vector<DataAccumulator<T> > bins(512);
    for (unsigned i = 0; i < samples.size(); i++) {
      DataSample<T> &sample=samples[i];
      bins[(int)floor(client_tile_index.position(sample.time)*512)] += sample;
    }
    samples.clear();
    for (unsigned i = 0; i < bins.size(); i++) {
      if (bins[i].weight > 0) samples.push_back(bins[i].get_sample());
    }
  }
}

struct GraphSample {
  double time;
  bool has_value;
  double value;
  double stddev;
  double weight;
  bool has_comment;
  std::string comment;
  GraphSample(DataSample<double> &x) : time(x.time), has_value(true), value(x.value), stddev(x.stddev), weight(x.weight),
				       has_comment(false), comment("") {}
  GraphSample(DataSample<std::string> &x) : time(x.time), has_value(false), value(0), stddev(x.stddev), weight(x.weight),
					    has_comment(true), comment(x.value) {}
};

bool operator<(const GraphSample &a, const GraphSample &b) { return a.time < b.time; }

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

  std::vector<DataSample<double> > double_samples;
  std::vector<DataSample<std::string> > string_samples;
  std::vector<DataSample<std::string> > comments;

  bool doubles_binned, strings_binned, comments_binned;
  read_tile_samples(store, uid, full_channel_name, requested_index, client_tile_index, double_samples, doubles_binned);
  read_tile_samples(store, uid, full_channel_name, requested_index, client_tile_index, string_samples, strings_binned);
  read_tile_samples(store, uid, full_channel_name+"._comment", requested_index, client_tile_index, comments, comments_binned);
  string_samples.insert(string_samples.end(), comments.begin(), comments.end());
  std::sort(string_samples.begin(), string_samples.end(), DataSample<std::string>::time_lessthan);
  
  std::map<double, DataSample<double> > double_sample_map;
  for (unsigned i = 0; i < double_samples.size(); i++) {
    double_sample_map[double_samples[i].time] = double_samples[i]; // TODO: combine if two samples at same time?
  }
  std::set<double> has_string;
  for (unsigned i = 0; i < string_samples.size(); i++) {
    has_string.insert(string_samples[i].time);
  }

  std::vector<GraphSample> graph_samples;

  bool has_fifth_col = string_samples.size()>0;

  for (unsigned i = 0; i < string_samples.size(); i++) {
    if (double_sample_map.find(string_samples[i].time) != double_sample_map.end()) {
      GraphSample gs(double_sample_map[string_samples[i].time]);
      gs.has_comment = true;
      gs.comment = string_samples[i].value;
      graph_samples.push_back(gs);
    } else {
      graph_samples.push_back(GraphSample(string_samples[i]));
    }
  }

  for (unsigned i = 0; i < double_samples.size(); i++) {
    if (has_string.find(double_samples[i].time) == has_string.end()) {
      graph_samples.push_back(GraphSample(double_samples[i]));
    }
  }

  std::sort(graph_samples.begin(), graph_samples.end());

  double bin_width = client_tile_index.duration() / 512.0;
  
  double line_break_threshold = bin_width * 4.0;
  if (!doubles_binned && double_samples.size() > 1) {
    // Find the median distance between samples
    std::vector<double> spacing(double_samples.size()-1);
    for (size_t i = 0; i < double_samples.size()-1; i++) {
      spacing[i] = double_samples[i+1].time - double_samples[i].time;
    }
    std::sort(spacing.begin(), spacing.end());
    double median_spacing = spacing[spacing.size()/2];
    // Set line_break_threshold to larger of 4*median_spacing and 4*bin_width
    line_break_threshold = std::max(line_break_threshold, median_spacing * 4);
  }

  if (graph_samples.size()) {
    log_f("gettile: outputting %zd samples", graph_samples.size());
    Json::Value tile(Json::objectValue);
    tile["level"] = Json::Value(tile_level);
    // An aside about offset type and precision:
    // JSONCPP doesn't have a long long type;  to preserve full resolution we need to convert to double here.  As Javascript itself
    // will read this as a double-precision value, we're not introducing a problem.
    // For a detailed discussion, see https://sites.google.com/a/bodytrack.org/wiki/website/tile-coordinates-and-numeric-precision
    // Irritatingly, JSONCPP wants to add ".0" to the end of floating-point numbers that don't need it.  This is inconsistent
    // with Javascript itself and simply introduces extra bytes to the representation
    tile["offset"] = Json::Value((double)tile_offset);
    tile["fields"] = Json::Value(Json::arrayValue);
    tile["fields"].append(Json::Value("time"));
    tile["fields"].append(Json::Value("mean"));
    tile["fields"].append(Json::Value("stddev"));
    tile["fields"].append(Json::Value("count"));
    if (has_fifth_col) tile["fields"].append(Json::Value("comment"));
    Json::Value data(Json::arrayValue);

    double previous_sample_time = client_tile_index.start_time();
    bool previous_had_value = true;

    for (unsigned i = 0; i < graph_samples.size(); i++) {
      // TODO: improve linebreak calculations:
      // 1) observe channel specs line break size from database (expressed in time;  some observations have long time periods and others short)
      // 2) insert breaks at beginning or end of tile if needed
      // 3) should client be the one to decide where line breaks are (if we give it the threshold?)
      if (graph_samples[i].time - previous_sample_time > line_break_threshold ||
	  !graph_samples[i].has_value || !previous_had_value) {
	// Insert line break, which has value -1e+308
	Json::Value sample = Json::Value(Json::arrayValue);
	sample.append(Json::Value(0.5*(graph_samples[i].time+previous_sample_time)));
	sample.append(Json::Value(-1e308));
	sample.append(Json::Value(0));
	sample.append(Json::Value(0));
	if (has_fifth_col) sample.append(Json::Value()); // NULL
	data.append(sample);
      }
      previous_sample_time = graph_samples[i].time;
      previous_had_value = graph_samples[i].has_value;
      {	
	Json::Value sample = Json::Value(Json::arrayValue);
	sample.append(Json::Value(graph_samples[i].time));
	sample.append(Json::Value(graph_samples[i].has_value ? graph_samples[i].value : 0.0));
	// TODO: fix datastore so we never see NAN crop up here!
	sample.append(Json::Value(isnan(graph_samples[i].stddev) ? 0 : graph_samples[i].stddev));
	sample.append(Json::Value(graph_samples[i].weight));
	if (has_fifth_col) {
	  sample.append(graph_samples[i].has_comment ? Json::Value(graph_samples[i].comment) : Json::Value());
	}
	data.append(sample);
      }

    }
    if (client_tile_index.end_time() - previous_sample_time > line_break_threshold ||
	!previous_had_value) {
      // Insert line break, which has value -1e+308
      Json::Value sample = Json::Value(Json::arrayValue);
      sample.append(Json::Value(0.5*(previous_sample_time + client_tile_index.end_time())));
      sample.append(Json::Value(-1e308));
      sample.append(Json::Value(0));
      sample.append(Json::Value(0));
      if (has_fifth_col) sample.append(Json::Value()); // NULL
      data.append(sample);
    }
    tile["data"] = data;
    tile["sample_width"] = std::max(30.0, bin_width);
    printf("%s\n", rtrim(Json::FastWriter().write(tile)).c_str());
  } else {
    log_f("gettile: no samples");
    printf("{}");
  }
  log_f("gettile: finished in %lld msec", millitime() - begin_time);

  return 0;
}
