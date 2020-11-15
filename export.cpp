//Example:
//From Chris Bartley to Everyone: (3:19 PM)
///datastore/data-prod/3/feed_26
//From Chris Bartley to Everyone: (3:34 PM)
///datastore/bin/export /datastore/data-prod --csv --start 1604638800 3 feed_26.OZONE_PPM

// C++
#include <cfloat>
#include <iostream>
#include <string>
#include <vector>
#include <chrono>

// C
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

// Date and timezone libraries, in local repo
#include "date/tz.h"

// Local
#include "Arglist.h"
#include "Channel.h"
#include "FilesystemKVS.h"
#include "Log.h"
#include "simple_shared_ptr.h"
#include "utils.h"

int double_precision_digits = 15;

void usage(const char *fmt, ...) {
  std::cerr << "\n";
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, fmt, args);
  va_end(args);
  std::cerr << "\n";
  std::cerr << "\n";

  std::cerr << "Usage:\n";
  std::cerr << "export [flags] store.kvs uid dev_nickname.ch_name [dev_nickname.ch_name ...]\n";
  std::cerr << "export [flags] store.kvs uid.dev_nickname.ch_name [uid.dev_nickname.ch_name ...]\n";
  std::cerr << "   --start:           start time (floating-point epoch time).  Defaults to beginning of time.\n";
  std::cerr << "   --end:             end time (floating-point epoch time).  Defaults to end of time.\n";
  std::cerr << "   --csv:             export in CSV format\n";
  std::cerr << "   --json:            export in JSON format\n";
  std::cerr << "   --timezone:        Export ISO 8601 timestamps in specified timezone.\n";
  std::cerr << "                      If omitted, use floating-point epoch timestamps.\n";
  std::cerr << "                      Examples:  America/New-York, Europe/London, UTC\n";
  std::cerr << "   --full-precision:  export full-precision data\n";
  std::cerr << "                      IEEE 754 double-precision values have ~15.6 digits of precision.\n";
  std::cerr << "                      Default output uses 15 digits of precision to avoid displaying\n";
  std::cerr << "                      more digits than actual precision.  --fullprecision forces 16-digit\n";
  std::cerr << "                      sometimes ending similarly to 999999 or 000001.\n";
  std::cerr << "                      output, to capture fully all the bits of precision, at the expense\n";
  std::cerr << "                      of showing an imprecise final digits, e.g. ending with 9999 or 0001\n";
  std::cerr << "Exiting...\n";
  exit(1);
}

bool dump_samples(const Tile &tile, Range requested_times) {
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
        printf("%.*g\t%*g\n", double_precision_digits, sample.time, double_precision_digits, sample.value);
    } else {
      DataSample<std::string> sample = tile.string_samples[string_index++];
      if (requested_times.includes(sample.time))
        printf("%.*g\t%s\n", double_precision_digits, sample.time, sample.value.c_str());
    }
  }
  return true;
}

void export_legacy(KVS &store, Range timerange, int uid, const std::vector<std::string> &channel_full_names) {
  for (unsigned i = 0; i < channel_full_names.size(); i++) {
    const std::string &channel_full_name = channel_full_names[i];
    if (i) printf("\f");
    printf("Time\t%s\n", channel_full_name.c_str());
    Channel ch(store, uid, channel_full_name);
    Channel::Locker locker(ch);
    ch.read_tiles_in_range(timerange, dump_samples, TileIndex::lowest_level());
  }
}

std::string quote_csv(std::string str) {
  bool needquote = false;
  for (unsigned i = 0; i < str.length(); i++) {
    if (isspace(str[i]) || str[i] == ',' || str[i] == '"') {
      needquote = true;
      break;
    }
  }
  if (needquote) {
    std::string ret = "\"";
    for (unsigned i = 0; i < str.length(); i++) {
      ret += str[i];
      if (str[i] == '"') ret += str[i]; // each single " becomes two
    }
    ret += "\"";
    return ret;
  } else {
    return str;
  }
}

std::string quote_json(std::string str) {
  std::string ret = "\"";
  for (unsigned i = 0; i < str.length(); i++) {
    // if this character is a double-quote, then escape it
    if (str[i] == '"') {
      ret += "\\\"";
    } else {
      ret += str[i];
    }
  }
  ret += "\"";

  return ret;
}

// TODO(rsargent): implement version that can subsample
// TODO(rsargent): move to ChannelReader.{cpp,h}
class ChannelReader {
private:
  std::string channel_name;
  simple_shared_ptr<Channel> channel;
  TileIndex ti; // null if current tile is invalid
  Tile tile;
  unsigned double_index, string_index;
  int desired_level;

public:
  ChannelReader(const std::string &channel_name, simple_shared_ptr<Channel> channel, double time)
    : channel_name(channel_name), channel(channel), desired_level(TileIndex::lowest_level()) {
    seek(time);
  }

  // Seek to first time >= new_time
  void seek(double new_time) {
    TileIndex root = root_tile();
    if (root.is_null()) {
      ti = TileIndex::null();
    } else {
      read_tile_or_successor(channel->find_child_overlapping_time(root, new_time, desired_level), root);
      while (time() < new_time) advance();
    }
  }

  // Returns NULL if no double_sample at current time, or if no more samples available
  DataSample<double> *double_sample() {
    if (!ti.is_null() && double_index < tile.double_samples.size()) {
      return &tile.double_samples[double_index];
    }
    return NULL;
  }

  // Returns NULL if no string_sample at current time, or if no more samples available
  DataSample<std::string> *string_sample() {
    if (!ti.is_null() && string_index < tile.string_samples.size()) {
      return &tile.string_samples[string_index];
    }
    return NULL;
  }

  // Returns DBL_MAX when no more samples available
  double time() {
    double t = DBL_MAX;
    if (double_sample()) t = std::min(t, double_sample()->time);
    if (string_sample()) t = std::min(t, string_sample()->time);
    return t;
  }

  // Advance to next available timestamp
  void advance() {
    if (ti.is_null()) {
      return;
    }

    double t = time();
    if (double_sample() && double_sample()->time == t) double_index++;
    if (string_sample() && string_sample()->time == t) string_index++;
    if (!double_sample() && !string_sample()) {
      // Advance to next tile
      TileIndex root = root_tile();
      if (root.is_null()) {
        ti = TileIndex::null();
      } else {
        read_tile_or_successor(channel->find_successive_tile(root, ti, desired_level), root);
      }
    }
  }

private:
  void read_tile_or_successor(TileIndex tile_index, TileIndex root) {
    while (1) {
      Channel::Locker lock(*channel);
      ti = tile_index;
      double_index = string_index = 0;
      if (!ti.is_null()) {
        if (!channel->read_tile(ti, tile)) {
          ti = TileIndex::null();
        } else {
          if (!double_sample() && !string_sample()) {
            // Empty tile?  skip to next
            tile_index = channel->find_successive_tile(root, tile_index, desired_level);
            continue;
          }
        }
      }
      return;
    }
  }

  TileIndex root_tile() {
    Channel::Locker lock(*channel);
    ChannelInfo info;
    if (!channel->read_info(info)) return TileIndex::null();
    return info.nonnegative_root_tile_index;
  }
};


// Write timestamp to stdout
// If timezone is not null, use it
// Otherwise, output as epoch timestamp in float

void output_timestamp(double epoch_time, const date::time_zone *timezone = NULL) {
  if (timezone) {
    std::chrono::duration<double> chrono_duration(epoch_time);
    auto datetime_with_zone = date::make_zoned(timezone, date::sys_time<std::chrono::duration<double>>(chrono_duration));
    std::cout << date::format("%Y-%m-%dT%H:%M:%S%Ez", datetime_with_zone);
  } else {
    printf("%.*g", double_precision_digits, epoch_time);
  }
}
  
void export_csv(KVS &store, Range timerange, int uid, const std::vector<std::string> &channel_full_names, const date::time_zone *timezone = NULL) {
  std::vector<simple_shared_ptr<ChannelReader> > readers;
  
  for (unsigned i = 0; i < channel_full_names.size(); i++) {
    simple_shared_ptr<Channel> ch;
    if (uid == -1) {
      ch.reset(new Channel(store, channel_full_names[i]));
    } else {
      ch.reset(new Channel(store, uid, channel_full_names[i]));
    }
    readers.push_back(simple_shared_ptr<ChannelReader>
                          (new ChannelReader(channel_full_names[i], ch, timerange.min)));
  }

  // Emit header
  if (timezone) {
    std::cout << quote_csv("Iso8601Time");
  } else {
    std::cout << quote_csv("Time");
  }
  for (unsigned i = 0; i < readers.size(); i++) {
    std::cout << ",";
    std::cout << quote_csv(channel_full_names[i]);
  }
  std::cout << "\n";

  while (1) {
    // Find next time -- min of all channels' next values
    double time = DBL_MAX;
    for (unsigned i = 0; i < readers.size(); i++) {
      time = std::min(time, readers[i]->time());
    }

    // If no more samples in range, we're done
    if (time > timerange.max || time == DBL_MAX) break;

    // Emit any samples that match this time.  Emit empty entries
    // for channels that don't have a for this time

    output_timestamp(time, timezone);
 
    for (unsigned i = 0; i < readers.size(); i++) {
      std::cout << ",";
      if (readers[i]->time() == time) {
        // If channel has a double and string sample at same time, just output the double
        if (readers[i]->double_sample()) {
          printf("%.*g", double_precision_digits, readers[i]->double_sample()->value);
        } else if (readers[i]->string_sample()) {
          std::cout << quote_csv(readers[i]->string_sample()->value).c_str();
        } else {
          abort();
        }
        readers[i]->advance();
      } else {
        // No sample at this time;  empty column
      }
    }
    std::cout << "\n";
  }
}

void export_json(KVS &store, Range timerange, int uid, const std::vector<std::string> &channel_full_names, const date::time_zone *timezone = NULL) {
  std::vector<simple_shared_ptr<ChannelReader> > readers;

  for (unsigned i = 0; i < channel_full_names.size(); i++) {
    simple_shared_ptr<Channel> ch;
    if (uid == -1) {
      ch.reset(new Channel(store, channel_full_names[i]));
    } else {
      ch.reset(new Channel(store, uid, channel_full_names[i]));
    }
    readers.push_back(simple_shared_ptr<ChannelReader>
                        (new ChannelReader(channel_full_names[i], ch, timerange.min)));
  }

  // Emit header
  std::cout << "{\"channel_names\":[";
  for (unsigned i = 0; i < readers.size(); i++) {
    std::cout << quote_json(channel_full_names[i]);
    if (i < readers.size() - 1) {
      std::cout << ",";
    }
  }
  std::cout << "],";

  // emit the data
  std::cout << "\"data\":[";

  bool hasPrintedAtLeastOneLine = false;
  while (1) {
    // Find next time -- min of all channels' next values
    double time = DBL_MAX;
    for (unsigned i = 0; i < readers.size(); i++) {
      time = std::min(time, readers[i]->time());
    }

    // If no more samples in range, we're done
    if (time > timerange.max || time == DBL_MAX) break;

    // Emit any samples that match this time.  Emit empty entries
    // for channels that don't have a for this time

    if (hasPrintedAtLeastOneLine) {
      std::cout << ",";
    }
    printf("\n[");
    if (timezone) printf("\"");
    output_timestamp(time, timezone);
    if (timezone) printf("\"");
    for (unsigned i = 0; i < readers.size(); i++) {
      std::cout << ",";
      if (readers[i]->time() == time) {
        // If channel has a double and string sample at same time, just output the double
        if (readers[i]->double_sample()) {
          printf("%.*g", double_precision_digits, readers[i]->double_sample()->value);
        } else if (readers[i]->string_sample()) {
          std::cout << quote_json(readers[i]->string_sample()->value).c_str();
        } else {
          abort();
        }
        readers[i]->advance();
      } else {
        // No sample at this time;  empty column
        std::cout << "null";
      }
    }
    std::cout << "]";
    hasPrintedAtLeastOneLine = true;
  }

  // close it off
  std::cout << "\n]}\n";
}

int execute(Arglist args) {
  std::string invocation = args.to_string();
  enum {
    LEGACY_FORMAT = 1,
    CSV_FORMAT,
    JSON_FORMAT
  } format = LEGACY_FORMAT;
  Range timerange = Range::all();

  std::string storename;
  int uid = -1;
  std::vector<std::string> channel_full_names;
  const date::time_zone *timezone = NULL;

  while (!args.empty()) {
    std::string arg = args.shift();
    if (arg == "--csv") {
      format = CSV_FORMAT;
    } else if (arg == "--json") {
      format = JSON_FORMAT;
    } else if (arg == "--start") {
      timerange.min = args.shift_double();
    } else if (arg == "--end") {
      timerange.max = args.shift_double();
    } else if (arg == "--full-precision") {
      double_precision_digits = 16;
    } else if (arg == "--timezone") {
      timezone = date::locate_zone(args.shift());
    } else if (Arglist::is_flag(arg)) {
      usage("Unknown flag '%s'", arg.c_str());
    } else if (storename == "") {
      storename = arg;
    } else if (uid == -1 && !channel_full_names.size()) {
      // This might be UID or a fully-specified channel name of the form UID.dev.ch
      if (strchr(arg.c_str(), '.')) {
        channel_full_names.push_back(arg);
      } else {
        uid = Arglist::parse_int(arg);
      }
    } else {
      channel_full_names.push_back(arg);
    }
  }

  if (storename == "") usage("Missing store");
  if (channel_full_names.size() == 0) usage("No channels specified");

  set_log_prefix(string_printf("%d %d ", getpid(), uid));

  log_f("export START: %s", invocation.c_str());

  FilesystemKVS store(storename.c_str());
  //store.set_verbosity(100);

  switch (format) {
    case LEGACY_FORMAT:
      export_legacy(store, timerange, uid, channel_full_names);
      break;
    case CSV_FORMAT:
      export_csv(store, timerange, uid, channel_full_names, timezone);
      break;
    case JSON_FORMAT:
      export_json(store, timerange, uid, channel_full_names, timezone);
      break;
    default:
      assert(0);
  }
  return 0;
}

int main(int argc, char **argv) {
  int exit_code = 1;
  try {
    exit_code = execute(Arglist(argv + 1, argv + argc));
  } catch (const std::exception &e) {
    log_f("export: caught exception '%s'", e.what());
  }
  return exit_code;
}
