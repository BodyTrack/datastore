// C++
#include <iostream>
#include <set>
#include <string>
#include <vector>

// C
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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
  std::cerr << "info store.kvs [-r] [-v] uid [--prefix channel_prefix] [--min-time t] [--max-time t]\n";
  std::cerr << "\n";
  std::cerr << "If channel_prefix is omitted and empty string and -r is given, give info on all channels for uid\n";
  std::cerr << "examples:\n";
  std::cerr << "info production.kvs -r 2 '' \n";
  std::cerr << "info production.kvs -r 2 myandroid\n";
  std::cerr << "info production.kvs 2 myandroid.comments\n";
  std::cerr << "info production.kvs 2 myandroid.comments --begin-time 1315082409 --end-time 1315095000\n";
  std::cerr << "\n";
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
      bins[floor(client_tile_index.position(sample.time)*512)] += sample;
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


double parse_time(const char *str)
{
  char *endptr;
  double t = strtod(str, &endptr);
  if (endptr != str+strlen(str)) {
    fprintf(stderr, "Cannot parse '%s' as time;  please use double-precision epoch time\n",
	    str);
    usage();
  }
  return t;
}

Range *gcic_found_times, *gcic_found_values;
long long gcic_nsamples;


bool get_channel_info_callback(const Tile &tile, Range requested_times)
{
  for (unsigned i = 0; i < tile.double_samples.size(); i++) {
    if (requested_times.includes(tile.double_samples[i].time)) {
      gcic_found_times->add(tile.double_samples[i].time);
      gcic_found_values->add(tile.double_samples[i].value);
      gcic_nsamples++;
    }
  }
  for (unsigned i = 0; i < tile.string_samples.size(); i++) {
    if (requested_times.includes(tile.string_samples[i].time)) {
      gcic_found_times->add(tile.string_samples[i].time);
      gcic_nsamples++;
    }
  }
  return true;
}

void get_channel_info(KVS &store, int uid, const std::string &channel_name, 
		      Range times, Range &found_times, Range &found_values)
{
  Channel ch(store, uid, channel_name);
  Channel::Locker locker(ch);
  if (times == Range::all()) {
    ChannelInfo info;
    if (!ch.read_info(info)) {
      log_f("Channel %s: no info", channel_name.c_str());
      return;
    }
    Tile root;
    if (!ch.read_tile(info.nonnegative_root_tile_index, root)) {
      log_f("Channel %s: cannot read root tile", channel_name.c_str());
      return;
    }
    found_times = root.ranges.times;
    found_values = root.ranges.double_samples;
  } else {
    gcic_found_times = &found_times;
    gcic_found_values = &found_values;
    gcic_nsamples = 0;
    // Look for ~1K samples
    int desired_level = ch.level_from_rate(1000 / (times.max - times.min));
    ch.read_tiles_in_range(times, get_channel_info_callback, desired_level);
    //ch.read_bottommost_tiles_in_range(times, get_channel_info_callback);
    log_f("Channel %s: read %lld samples", channel_name.c_str(), gcic_nsamples);
  }
}


int main(int argc, char **argv)
{
  long long begin_perf_time = millitime();
  
  std::string storename = "";
  std::string channel_prefix = "";
  int uid = -1;
  bool recurse = false;
  Range requested_times = Range::all();
  int argno = 0;
  int verbose = 0;

  char **argptr = argv+1;
  while (*argptr) {
    std::string arg(*argptr++);
    if (arg == "-r") recurse = true;
    else if (arg == "-v" && *argptr) verbose++;
    else if (arg == "--prefix" && *argptr) channel_prefix = *argptr++;
    else if (arg == "--min-time" && *argptr) requested_times.min = parse_time(*argptr++);
    else if (arg == "--max-time" && *argptr) requested_times.max = parse_time(*argptr++);
    else if (arg.length() > 0 && arg[0] == '-') usage();
    else switch (++argno) {
      case 1: storename = arg; break;
      case 2: uid = atoi(arg.c_str()); break;
      default: usage(); break;
      }
  }
  set_log_prefix(string_printf("%d %d ", getpid(), uid));

  if (storename == "") usage();
  if (uid < 1) usage();
  
  {
    std::string arglist;
    for (int i = 0; i < argc; i++) {
      if (i) arglist += " ";
      arglist += std::string("'")+argv[i]+"'";
    }
    log_f("info START: %s", arglist.c_str());
  }

  FilesystemKVS store(storename.c_str());
  if (verbose) store.set_verbosity(1);

  std::vector<std::string> subchannel_names;
  long long begin_channel_time = millitime();
  if (recurse) {
    Channel::get_subchannel_names(store, uid, channel_prefix, subchannel_names);
  } else {
    subchannel_names.push_back(channel_prefix);
  }
  log_f("info: Found %zd channels in %lld msec", 
	subchannel_names.size(),
	millitime() - begin_channel_time);
  
  Json::Value info(Json::objectValue);
  Json::Value channel_specs(Json::objectValue);
  
  Range all_found_times;
  for (unsigned i = 0; i < subchannel_names.size(); i++) {
    Range found_times, found_values;
    get_channel_info(store, uid, subchannel_names[i], requested_times, found_times, found_values);
    Json::Value channel_bounds(Json::objectValue);
    if (!found_times.empty()) {
      all_found_times.add(found_times);
      channel_bounds["min_time"] = found_times.min;
      channel_bounds["max_time"] = found_times.max;
    }
    if (!found_values.empty()) {
      channel_bounds["min_value"] = found_values.min;
      channel_bounds["max_value"] = found_values.max;
    }
    if (channel_bounds.size()) {
      channel_specs[subchannel_names[i]] = Json::Value(Json::objectValue);
      channel_specs[subchannel_names[i]]["channel_bounds"]=channel_bounds;
    }
  }
  info["channel_specs"]=channel_specs;
  if (!all_found_times.empty()) {
    info["min_time"]=all_found_times.min;
    info["max_time"]=all_found_times.max;
  }
  
  std::string response = rtrim(Json::FastWriter().write(info));
  printf("%s\n", response.c_str());
  log_f("info: sending: %s", response.c_str());
  
//  // Desired level and offset
//  // Translation between tile request and tilestore:
//  // tile: level 0 is 512 samples in 512 seconds
//  // store: level 0 is 65536 samples in 1 second
//  // for tile level 0, we want to get store level 14, which is 65536 samples in 16384 seconds
//
//  // Levels differ by 9 between client and server
//  TileIndex client_tile_index = TileIndex(tile_level+9, tile_offset);
//
//  {
//    std::string arglist;
//    for (int i = 0; i < argc; i++) {
//      if (i) arglist += " ";
//      arglist += std::string("'")+argv[i]+"'";
//    }
//    log_f("gettile START: %s (time %.9f-%.9f)",
//	  arglist.c_str(), client_tile_index.start_time(), client_tile_index.end_time());
//  }
//    
//
//  // 5th ancestor
//  TileIndex requested_index = client_tile_index.parent().parent().parent().parent().parent();
//
//  std::vector<DataSample<double> > double_samples;
//  std::vector<DataSample<std::string> > string_samples;
//  std::vector<DataSample<std::string> > comments;
//
//  bool doubles_binned, strings_binned, comments_binned;
//  read_tile_samples(store, uid, full_channel_name, requested_index, client_tile_index, double_samples, doubles_binned);
//  read_tile_samples(store, uid, full_channel_name, requested_index, client_tile_index, string_samples, strings_binned);
//  read_tile_samples(store, uid, full_channel_name+"._comment", requested_index, client_tile_index, comments, comments_binned);
//  string_samples.insert(string_samples.end(), comments.begin(), comments.end());
//  std::sort(string_samples.begin(), string_samples.end(), DataSample<std::string>::time_lessthan);
//  
//  std::map<double, DataSample<double> > double_sample_map;
//  for (unsigned i = 0; i < double_samples.size(); i++) {
//    double_sample_map[double_samples[i].time] = double_samples[i]; // TODO: combine if two samples at same time?
//  }
//  std::set<double> has_string;
//  for (unsigned i = 0; i < string_samples.size(); i++) {
//    has_string.insert(string_samples[i].time);
//  }
//
//  std::vector<GraphSample> graph_samples;
//
//  bool has_fifth_col = string_samples.size()>0;
//
//  for (unsigned i = 0; i < string_samples.size(); i++) {
//    if (double_sample_map.find(string_samples[i].time) != double_sample_map.end()) {
//      GraphSample gs(double_sample_map[string_samples[i].time]);
//      gs.has_comment = true;
//      gs.comment = string_samples[i].value;
//      graph_samples.push_back(gs);
//    } else {
//      graph_samples.push_back(GraphSample(string_samples[i]));
//    }
//  }
//
//  for (unsigned i = 0; i < double_samples.size(); i++) {
//    if (has_string.find(double_samples[i].time) == has_string.end()) {
//      graph_samples.push_back(GraphSample(double_samples[i]));
//    }
//  }
//
//  std::sort(graph_samples.begin(), graph_samples.end());
//  
//  double line_break_threshold = client_tile_index.duration() / 512.0 * 4.0; // 4*binsize
//  if (!doubles_binned && double_samples.size() > 1) {
//    // Find the median distance between samples
//    std::vector<double> spacing(double_samples.size()-1);
//    for (size_t i = 0; i < double_samples.size()-1; i++) {
//      spacing[i] = double_samples[i+1].time - double_samples[i].time;
//    }
//    std::sort(spacing.begin(), spacing.end());
//    double median_spacing = spacing[spacing.size()/2];
//    // Set line_break_threshold to larger of 4*median_spacing and 4*bin_size
//    line_break_threshold = std::max(line_break_threshold, median_spacing * 4);
//  }
//
//  if (graph_samples.size()) {
//    log_f("gettile: outputting %zd samples", graph_samples.size());
//    Json::Value tile(Json::objectValue);
//    tile["level"] = Json::Value(tile_level);
//    // An aside about offset type and precision:
//    // JSONCPP doesn't have a long long type;  to preserve full resolution we need to convert to double here.  As Javascript itself
//    // will read this as a double-precision value, we're not introducing a problem.
//    // For a detailed discussion, see https://sites.google.com/a/bodytrack.org/wiki/website/tile-coordinates-and-numeric-precision
//    // Irritatingly, JSONCPP wants to add ".0" to the end of floating-point numbers that don't need it.  This is inconsistent
//    // with Javascript itself and simply introduces extra bytes to the representation
//    tile["offset"] = Json::Value((double)tile_offset);
//    tile["fields"] = Json::Value(Json::arrayValue);
//    tile["fields"].append(Json::Value("time"));
//    tile["fields"].append(Json::Value("mean"));
//    tile["fields"].append(Json::Value("stddev"));
//    tile["fields"].append(Json::Value("count"));
//    if (has_fifth_col) tile["fields"].append(Json::Value("comment"));
//    Json::Value data(Json::arrayValue);
//
//    double previous_sample_time = client_tile_index.start_time();
//    bool previous_had_value = true;
//
//    for (unsigned i = 0; i < graph_samples.size(); i++) {
//      // TODO: improve linebreak calculations:
//      // 1) observe channel specs line break size from database (expressed in time;  some observations have long time periods and others short)
//      // 2) insert breaks at beginning or end of tile if needed
//      // 3) should client be the one to decide where line breaks are (if we give it the threshold?)
//      if (graph_samples[i].time - previous_sample_time > line_break_threshold ||
//	  !graph_samples[i].has_value || !previous_had_value) {
//	// Insert line break, which has value -1e+308
//	Json::Value sample = Json::Value(Json::arrayValue);
//	sample.append(Json::Value(0.5*(graph_samples[i].time+previous_sample_time)));
//	sample.append(Json::Value(-1e308));
//	sample.append(Json::Value(0));
//	sample.append(Json::Value(0));
//	if (has_fifth_col) sample.append(Json::Value()); // NULL
//	data.append(sample);
//      }
//      previous_sample_time = graph_samples[i].time;
//      previous_had_value = graph_samples[i].has_value;
//      {	
//	Json::Value sample = Json::Value(Json::arrayValue);
//	sample.append(Json::Value(graph_samples[i].time));
//	sample.append(Json::Value(graph_samples[i].has_value ? graph_samples[i].value : 0.0));
//	// TODO: fix datastore so we never see NAN crop up here!
//	sample.append(Json::Value(isnan(graph_samples[i].stddev) ? 0 : graph_samples[i].stddev));
//	sample.append(Json::Value(graph_samples[i].weight));
//	if (has_fifth_col) {
//	  sample.append(graph_samples[i].has_comment ? Json::Value(graph_samples[i].comment) : Json::Value());
//	}
//	data.append(sample);
//      }
//
//    }
//    if (client_tile_index.end_time() - previous_sample_time > line_break_threshold ||
//	!previous_had_value) {
//      // Insert line break, which has value -1e+308
//      Json::Value sample = Json::Value(Json::arrayValue);
//      sample.append(Json::Value(0.5*(previous_sample_time + client_tile_index.end_time())));
//      sample.append(Json::Value(-1e308));
//      sample.append(Json::Value(0));
//      sample.append(Json::Value(0));
//      if (has_fifth_col) sample.append(Json::Value()); // NULL
//      data.append(sample);
//    }
//    tile["data"] = data;
//    printf("%s\n", rtrim(Json::FastWriter().write(tile)).c_str());
//  } else {
//    log_f("gettile: no samples");
//    printf("{}");
//  }
  log_f("info: finished in %lld msec.  read %d tiles", 
	millitime() - begin_perf_time, Channel::total_tiles_read);
  
  return 0;
}
