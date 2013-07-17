// C
#include <cassert>

// C++
#include <functional>
#include <iostream>
#include <map>
#include <numeric>
#include <sstream>
#include <string>

// Local
#include "Channel.h"
#include "DataSample.h"
#include "fft.h"
#include "FilesystemKVS.h"
#include "Log.h"
#include "MapSerializer.h"
#include "Range.h"
#include "Tile.h"
#include "TileIndex.h"
#include "utils.h"

#ifdef FFT_SUPPORT

// Declarations of values local to this app
// The most detailed tile we represent has roughly 320 samples,
// spaced at 10 Hz intervals
const int SMALLEST_TILE_SAMPLES = 512;
const int MIN_LEVEL = TileIndex::duration_to_level(
    SMALLEST_TILE_SAMPLES / MAX_SAMPLE_RATE_HERTZ);

// Declarations of functions local to this app
void read_all_samples(Range times, Channel &ch,
    std::vector<DataSample<double> > * samples);
void write_fft_tile(Channel &ch,
    const std::vector<DataSample<double> > &samples,
    TileIndex index);
std::string tile_properties(const int num_values,
    const std::vector<std::vector<double> > &shifted);

template<typename T>
unsigned count_elems(const std::vector<std::vector<T> > &v);

template<typename T>
std::string to_string(const T &item);

void usage(void);

#endif /* FFT_SUPPORT */

// Implementation

int main(int argc, char **argv) {
#ifdef FFT_SUPPORT
  long long begin_time = millitime();
  char **argptr = argv + 1;

  if (!*argptr) usage();
  std::string storename = *argptr++;

  if (!*argptr) usage();
  int uid = atoi(*argptr++);

  set_log_prefix(string_printf("%d %d ", getpid(), uid));

  if (!*argptr) usage();
  std::string full_channel_name = *argptr++;

  if (!*argptr) usage();
  long long starttime = atoll(*argptr++);

  if (!*argptr) usage();
  long long endtime = atoll(*argptr++);

  if (*argptr) usage();

  FilesystemKVS store(storename.c_str());

  // We read from device.channel, and write to device.channel.DFT
  Channel input_ch(store, uid, full_channel_name);
  Channel output_ch(store, uid, full_channel_name + ".DFT");

  // TODO: Use a more consistent top level of FFT
  // TODO: Use Channel::read_info to find out what the channel root
  // level is, and generate FFT tiles up to that level
  Range times(starttime, endtime);
  TileIndex range_container = TileIndex::index_containing(times);
  log_f("Smallest tile that contains the time range %s "
      "has level %d and offset %lld",
      times.to_string().c_str(),
      range_container.level,
      range_container.offset);
  Range needed_times(min_time_required(range_container),
      max_time_required(range_container));

  if (range_container.level < MIN_LEVEL) {
    log_f("Time %s is too narrow: should be at least %d seconds",
        times.to_string().c_str(),
        TileIndex::level_to_duration(MIN_LEVEL));
    log_f("Exiting without writing tiles...");
    return 1; // Warning
  }

  std::vector<DataSample<double> > * samples =
    new std::vector<DataSample<double> >;
  read_all_samples(needed_times, input_ch, samples);
  log_f("%d samples found between in time range %s", samples->size(),
      needed_times.to_string().c_str());

  if (samples->empty()) {
    log_f("No samples found in the time range %s",
        times.to_string().c_str());
    log_f("Exiting without writing tiles...");
    return 1; // Warning
  }

  if (samples->size() == 1) {
    log_f("Only one sample found in the time range %s",
        times.to_string().c_str());
    log_f("Exiting without writing tiles...");
    return 1; // Warning
  }

  // Now we have the necessary data, so we build and save FFT tiles,
  // flattened into a one-dimensional vector
  for (int32 level = MIN_LEVEL; level <= range_container.level; level++) {
    int64 start_offset = TileIndex::index_at_level_containing(level,
        range_container.start_time()).offset;
    int64 end_offset = TileIndex::index_at_level_containing(level,
        range_container.end_time()).offset;

    for (int64 offset = start_offset; offset <= end_offset; offset++) {
      TileIndex index(level, offset);
      write_fft_tile(output_ch, *samples, index);
    }
  }

  // Clean up
  delete samples;
  log_f("genfft: finished in %lld msec", millitime() - begin_time);
  return 0;
#else /* FFT_SUPPORT */
  std::cerr << "Error: FFT support disabled" << std::endl;
  std::cerr << "  Either modify the code or recompile with FFT support" << std::endl;
  std::cerr << "  To add FFT support when compiling with the standard Makefile, define the" << std::endl;
  std::cerr << "  FFT_SUPPORT symbol e.g. make genfft FFT_SUPPORT=1" << std::endl;
  return -2;
#endif /* FFT_SUPPORT */
}

#ifdef FFT_SUPPORT

void read_all_samples(Range times, Channel &ch,
    std::vector<DataSample<double> > * samples) {
  std::function<bool (const Tile &, Range)> add_to_samples(
    [=] (const Tile &t, Range times) -> bool {
      for (unsigned i = 0; i < t.double_samples.size(); i++)
        samples->push_back(t.double_samples[i]);
      return true;
    });
  ch.read_bottommost_tiles_in_range(times, add_to_samples);
}

void write_fft_tile(Channel &ch,
    const std::vector<DataSample<double> > &samples,
    TileIndex index) {
  std::vector<std::vector<double> > * fft =
      new std::vector<std::vector<double> >;
  std::vector<std::vector<double> > * shifted =
      new std::vector<std::vector<double> >;

  // Compute the FFT
  windowed_fft(samples, index, *fft);

  // Subsample and otherwise shrink to something presentable
  int num_values;
  present_fft(*fft, *shifted, num_values);

  Tile tile;
  int nitems = count_elems(*shifted);
  assert (nitems > 0);
  DataSample<double> * flattened_samples = (DataSample<double> *)
      calloc(nitems, sizeof(DataSample<double>));
  if (!flattened_samples)
    throw std::runtime_error("Failed to allocate space for samples");
  unsigned curr_item = 0;
  for (unsigned window_id = 0; window_id < shifted->size(); window_id++) {
    for (unsigned item_idx = 0; item_idx < (*shifted)[window_id].size(); item_idx++) {
      // Evenly space all the values through the tile as we flatten
      // the vector, making sure not to go beyond the end of the tile
      double time = index.start_time()
          + index.duration() * curr_item / nitems;
      flattened_samples[curr_item] = DataSample<double>(time,
          (*shifted)[window_id][item_idx],
          1.0, 1.0); // Weight and stdev don't matter
      curr_item++;
    }
  }
  tile.insert_samples(&flattened_samples[0],
      &flattened_samples[nitems]); // End is one past last valid sample
  free(flattened_samples);

  // Write the serialized tile index
  DataSample<std::string> serialized_metadata_array[1];
  serialized_metadata_array[0] =
      DataSample<std::string>(index.start_time(),
          tile_properties(num_values, *shifted),
          1.0, 1.0);
  tile.insert_samples(&serialized_metadata_array[0],
      &serialized_metadata_array[1]); // End is one past last valid sample

  ch.write_tile(index, tile);

  delete fft;
  delete shifted;
}

template<typename T>
unsigned count_elems(const std::vector<std::vector<T> > &v) {
  int nitems = 0;
  for (unsigned idx = 0; idx < v.size(); idx++)
    nitems += v[idx].size();
  return nitems;
}

std::string tile_properties(const int num_values,
    const std::vector<std::vector<double> > &shifted) {
  std::map<std::string, std::string> m;
  m["num_values"] = to_string(num_values);
  m["nwindows"] = to_string(shifted.size());
  m["window_size"] = to_string(shifted[0].size());
  MapSerializer ms;
  return ms.serialize(m);
}

template<typename T>
std::string to_string(const T &item) {
  std::stringstream ss;
  ss << item;
  return ss.str();
}

void usage(void) {
  std::cerr << "Usage:" << std::endl;
  std::cerr << "genfft store.kvs UID devicenickname.channel starttime endtime" << std::endl;
  std::cerr << "Exiting..." << std::endl;
  exit(1);
}

#endif /* FFT_SUPPORT */

