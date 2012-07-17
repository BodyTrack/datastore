// System
#include <assert.h>
#include <set>
#include <stdexcept>
#include <stdio.h>
#include <string.h>

// Local
#include "Log.h"
#include "TileIndex.h"
#include "utils.h"

// Self
#include "Channel.h"


int Channel::total_tiles_read;
int Channel::total_tiles_written;
int Channel::verbosity;

/// Create channel reference to KVS
/// \param owner_id  Owner of channel
/// \param name      Full name of channel (may be of form device_nickname.channel_name)
Channel::Channel(KVS &kvs, int owner_id, const std::string &name, size_t max_tile_size)
  : m_kvs(kvs), m_owner_id(owner_id), m_name(name), m_max_tile_size(max_tile_size) {
  if (!sizes_are_valid()) throw std::runtime_error("Wrongly-sized type");
}

/// Lock channel upon construction; if currently locked, construction will block until lock is available
/// \param ch        Channel to lock
Channel::Locker::Locker(const Channel &ch) : m_ch(ch), m_locker(ch.m_kvs, ch.metainfo_key()) {
  if (verbosity) log_f("Channel: locking %s", ch.descriptor().c_str());
}

Channel::Locker::~Locker() {
  if (verbosity) log_f("Channel: unlocking %s", m_ch.descriptor().c_str());
}

/// Read channel metainformation from KVS
/// \param  info Returns metainformation, if read
/// \return true if channel exists in KVS and read successful;  false if channel does not exist in KVS
/// Channel exists if metainfo_key (.info) exists and is of non-zero size.  File may exist and be of zero size
/// if the channel is locked before it is created.
bool Channel::read_info(ChannelInfo &info) const {
  std::string info_str;
  if (m_kvs.get(metainfo_key(), info_str) && info_str != "") {
    assert(info_str.length() == sizeof(ChannelInfo));
    memcpy((void*)&info, (void*)info_str.c_str(), sizeof(info));
    assert(info.magic == ChannelInfo::MAGIC);
    if (verbosity) log_f("Channel: read_info %s: root tile=%s", descriptor().c_str(), info.nonnegative_root_tile_index.to_string().c_str());
    return true;
  } else {
    return false;
  }
}

/// Write channel metainformation to KVS
/// \param  info New metainformation for channel
/// Creates channel in KVS if it doesn't already exist
void Channel::write_info(const ChannelInfo &info) {
  assert(!info.nonnegative_root_tile_index.is_null());
  std::string info_str((char*)&info, (char*)((&info)+1));
  m_kvs.set(metainfo_key(), info_str);
  if (verbosity) log_f("Channel: write_info %s : root tile=%s", descriptor().c_str(), info.nonnegative_root_tile_index.to_string().c_str());
}

bool Channel::has_tile(TileIndex ti) const {
  return m_kvs.has_key(tile_key(ti));
}

bool Channel::read_tile(TileIndex ti, Tile &tile) const {
  std::string binary;
  if (!m_kvs.get(tile_key(ti), binary)) return false;
  total_tiles_read++;
  tile.from_binary(binary);
  if (verbosity) log_f("Channel: read_tile %s %s: %s", 
                       descriptor().c_str(), ti.to_string().c_str(), tile.summary().c_str());
  return true;
}

void Channel::write_tile(TileIndex ti, const Tile &tile) {
  std::string binary;
  tile.to_binary(binary);
  //assert(binary.size() <= m_max_tile_size);
  m_kvs.set(tile_key(ti), binary);
  total_tiles_written++;
  if (verbosity) log_f("Channel: write_tile %s %s: %s", 
                       descriptor().c_str(), ti.to_string().c_str(), tile.summary().c_str());
}

bool Channel::delete_tile(TileIndex ti) {
  return m_kvs.del(tile_key(ti));
  if (verbosity) log_f("Channel: delete_tile %s %s", 
                       descriptor().c_str(), ti.to_string().c_str());
}

void Channel::create_tile(TileIndex ti) {
  Tile tile;
  write_tile(ti, tile);
}

double Channel::level_from_rate(double samples_per_second) const {
  double tile_length = BT_CHANNEL_DOUBLE_SAMPLES / samples_per_second;
  return TileIndex::duration_to_level(tile_length);
}

void Channel::add_data(const std::vector<DataSample<double> > &data, DataRanges *channel_ranges) {
  add_data_internal(data, channel_ranges);
}

void Channel::add_data(const std::vector<DataSample<std::string> > &data, DataRanges *channel_ranges) {
  add_data_internal(data, channel_ranges);
}

/// Add data to channel
/// \param data Data to add;  must be sorted in ascending time
/// Locking:  This method acquires a lock to channel as needed to guarantee update is successful in an environment
/// where multiple simultaneous updates are happening via add_data from multiple processes.
template <class T>
void Channel::add_data_internal(const std::vector<DataSample<T> > &data, DataRanges *channel_ranges) {
  if (!data.size()) return;
  // Sanity check
  if (data[0].time < 0) throw std::runtime_error("Unimplemented feature: adding data with negative time");
  for (unsigned i = 0; i < data.size()-1; i++) {
    if (data[i].time > data[i+1].time) throw std::runtime_error("Attempt to add data that is not sorted by ascending time");
  }

  //	regenerate = empty set
  Locker lock(*this);  // Lock self and hold lock until exiting this method
  std::set<TileIndex> to_regenerate;

  ChannelInfo info;
  bool success = read_info(info);
  if (!success) {
    // New channel
    info.magic = ChannelInfo::MAGIC;
    info.version = 0x00010000;
    info.times = Range(data[0].time, data.back().time);
    info.nonnegative_root_tile_index = TileIndex::nonnegative_all();
    create_tile(TileIndex::nonnegative_all());
    info.negative_root_tile_index = TileIndex::null();
  } else {
    info.times.add(Range(data[0].time, data.back().time));
    // If we're not the all-tile, see if we need to move the root upwards
    if (info.nonnegative_root_tile_index != TileIndex::nonnegative_all()) {
      TileIndex new_nonnegative_root_tile_index = TileIndex::index_containing(info.times);
      if (new_nonnegative_root_tile_index.level > info.nonnegative_root_tile_index.level) {
	// Root index changed.  Confirm new root is parent or more distant ancestor of old root
	assert(new_nonnegative_root_tile_index.is_ancestor_of(info.nonnegative_root_tile_index));
	// Trigger regeneration from old root's parent, up through new root
	to_regenerate.insert(info.nonnegative_root_tile_index.parent());
	move_root_upwards(new_nonnegative_root_tile_index, info.nonnegative_root_tile_index);
	info.nonnegative_root_tile_index = new_nonnegative_root_tile_index;
      }
    }
  }

  unsigned i=0;

  while (i < data.size()) {
    TileIndex ti= find_lowest_child_overlapping_time(info.nonnegative_root_tile_index, data[i].time);
    assert(!ti.is_null());

    Tile tile;
    assert(read_tile(ti, tile));
    const DataSample<T> *begin = &data[i];
    while (i < data.size() && ti.contains_time(data[i].time)) i++;
    const DataSample<T> *end = &data[i];
    tile.insert_samples(begin, end);
    
    TileIndex new_root = split_tile_if_needed(ti, tile);
    if (new_root != TileIndex::null()) {
      assert(ti == TileIndex::nonnegative_all());
      if (verbosity) log_f("Channel: %s changing root from %s to %s",
                           descriptor().c_str(), ti.to_string().c_str(),
                           new_root.to_string().c_str());
      info.nonnegative_root_tile_index = new_root;
      delete_tile(ti); // Delete old root
      ti = new_root;
    }
    write_tile(ti, tile);
    if (ti == info.nonnegative_root_tile_index && channel_ranges) { *channel_ranges = tile.ranges; }
    if (ti != info.nonnegative_root_tile_index) to_regenerate.insert(ti.parent());
  }
  
  // Regenerate from lowest level to highest
  while (!to_regenerate.empty()) {
    TileIndex ti = *to_regenerate.begin();
    to_regenerate.erase(to_regenerate.begin());
    Tile regenerated, children[2];
    assert(read_tile(ti.left_child(), children[0]));
    assert(read_tile(ti.right_child(), children[1]));
    create_parent_tile_from_children(ti, regenerated, children);
    write_tile(ti, regenerated);
    if (ti == info.nonnegative_root_tile_index && channel_ranges) { *channel_ranges = regenerated.ranges; }
    if (ti != info.nonnegative_root_tile_index) to_regenerate.insert(ti.parent());
  }
  write_info(info);
}

void Channel::read_data(std::vector<DataSample<double> > &data, double begin, double end) const {
  double time = begin;
  data.clear();

  Locker lock(*this);  // Lock self and hold lock until exiting this method

  ChannelInfo info;
  bool success = read_info(info);
  if (!success) {
    // Channel doesn't yet exist;  no data
    if (verbosity) log_f("read_data: can't read info");
    return;
  }
  bool first_tile = true;

  while (time < end) {
    TileIndex ti = find_lowest_child_overlapping_time(info.nonnegative_root_tile_index, time);
    if (ti.is_null()) {
      // No tiles; no more data
      if (verbosity) log_f("read_data: can't read tile");
      return;
    }

    Tile tile;
    assert(read_tile(ti, tile));
    unsigned i = 0;
    if (first_tile) {
      // Skip any samples before requested time
      for (; i < tile.double_samples.size() && tile.double_samples[i].time < begin; i++);
    }

    for (; i < tile.double_samples.size() && tile.double_samples[i].time < end; i++) {
      data.push_back(tile.double_samples[i]);
    }
    time = ti.end_time();
  }
}

template <class T>
void split_samples(const std::vector<DataSample<T> > &from, double split_time, Tile &to_a, Tile &to_b) {
  size_t split_index;
  for (split_index = 0; split_index < from.size() && from[split_index].time < split_time; split_index++) {}

  to_a.insert_samples(&from[0], &from[split_index]);
  to_b.insert_samples(&from[split_index], &from[from.size()]);
}

/// Split tile if needed (if it's too large)
/// If we're looking to split an "all" tile (negative_all or nonnegative_all), select a new root tile.
/// \param ti Tile Index to split if needed
/// \param tile tile to split if needed (tile that's pointed to by ti.  might not be written yet to the datastore)
/// \return Normally returns TileIndex::null(), but in case of splitting "all" tile, returns new root

TileIndex Channel::split_tile_if_needed(TileIndex ti, Tile &tile) {
  TileIndex new_root_index = TileIndex::null();
  if (tile.binary_length() <= m_max_tile_size) return new_root_index;
  Tile children[2];
  TileIndex child_indexes[2];
  if (verbosity) log_f("split_tile_if_needed: splitting tile %s", ti.to_string().c_str());

  // If we're splitting an "all" tile, it means that until now the channel has only had one tile's worth of
  // data, and that a proper root tile location couldn't be selected.  Select a new root tile now.
  if (ti.is_nonnegative_all()) {
    // TODO: this breaks if all samples are at one time
    ti = new_root_index = TileIndex::index_containing(Range(tile.first_sample_time(), tile.last_sample_time()));
    if (verbosity) log_f("split_tile_if_needed: Moving root tile to %s", ti.to_string().c_str());
  }
  
  child_indexes[0]= ti.left_child();
  child_indexes[1]= ti.right_child();

  double split_time = ti.right_child().start_time();
  split_samples(tile.double_samples, split_time, children[0], children[1]);
  split_samples(tile.string_samples, split_time, children[0], children[1]);

  for (int i = 0; i < 2; i++) {
    assert(!has_tile(child_indexes[i]));
    assert(split_tile_if_needed(child_indexes[i], children[i]) == TileIndex::null());
    write_tile(child_indexes[i], children[i]);
  }
  create_parent_tile_from_children(ti, tile, children);
  return new_root_index;
}


template <class T>
void combine_samples(unsigned int n_samples,
                     TileIndex parent_index,
                     std::vector<DataSample<T> > &parent,
                     const std::vector<DataSample<T> > &left_child,
                     const std::vector<DataSample<T> > &right_child)
{
  std::vector<DataAccumulator<T> > bins(n_samples);

  const std::vector<DataSample<T> > *children[2];
  children[0]=&left_child; children[1]=&right_child;

  int n=0;
  for (unsigned j = 0; j < 2; j++) {
    const std::vector<DataSample<T> > &child = *children[j];
    for (unsigned i = 0; i < child.size(); i++) {
      // Version 1: bin samples into correct bin
      // Version 2: try gaussian or lanczos(1) or 1/4 3/4 3/4 1/4
      
      const DataSample<T> &sample = child[i];
      assert(parent_index.contains_time(sample.time));
      unsigned bin = (unsigned) floor(parent_index.position(sample.time) * n_samples);
      assert(bin < n_samples);
      n++;
      bins[bin] += sample;
      assert(bins[bin].weight>0);
    }
  }

  n = 0;
  int m=0;
  parent.clear();
  for (unsigned i = 0; i < bins.size(); i++) {
    if (bins[i].weight > 0) {
      parent.push_back(bins[i].get_sample());
      assert(parent.size());
      n++;
    } else {
      m++;
    }
  }
  if (left_child.size() || right_child.size()) assert(parent.size());
}

void Channel::create_parent_tile_from_children(TileIndex parent_index, Tile &parent, Tile children[]) {
  // Subsample the children to create the parent
  // when do we want to show original values?
  // when do we want to do a real low-pass filter?
  // do we need to filter more than just the child tiles? e.g. gaussian beyond the tile border
  if (verbosity) log_f("Channel: creating parent %s from children %s, %s",
                       parent_index.to_string().c_str(),
                       parent_index.left_child().to_string().c_str(),
                       parent_index.right_child().to_string().c_str());

  combine_samples(BT_CHANNEL_DOUBLE_SAMPLES, parent_index, parent.double_samples, children[0].double_samples, children[1].double_samples);
  if (children[0].double_samples.size() + children[1].double_samples.size()) assert(parent.double_samples.size());

  combine_samples(BT_CHANNEL_STRING_SAMPLES, parent_index, parent.string_samples, children[0].string_samples, children[1].string_samples);
  if (children[0].string_samples.size() + children[1].string_samples.size()) assert(parent.string_samples.size());

  parent.ranges = children[0].ranges;
  parent.ranges.add(children[1].ranges);
}

void Channel::move_root_upwards(TileIndex new_root_index, TileIndex old_root_index) {
  Tile old_root_tile;
  Tile empty_tile;
  assert(read_tile(old_root_index, old_root_tile));
  TileIndex ti = old_root_index;
  while (ti != new_root_index) {
    write_tile(ti.sibling(), empty_tile);
    write_tile(ti.parent(), old_root_tile);
    ti = ti.parent();
  }
}

// If ti exists, read it
// Otherwise, if any ancestors of ti exist, read the closest one
// Otherwise, if ti is an ancestor of the datastore's root, return the datastore's root
// Otherwise, don't return a tile
// When returning a tile, returns true and leaves tile in ret and index of tile in ret_index
// When not returnng a tile, returns false

bool Channel::read_tile_or_closest_ancestor(TileIndex ti, TileIndex &ret_index, Tile &ret) const {
  Locker lock(*this);  // Lock self and hold lock until exiting this method
  ChannelInfo info;
  bool success = read_info(info);
  if (!success) {
    if (verbosity) log_f("read_tile_or_closest_ancestor: can't read info");
    return false;
  }
  TileIndex root = info.nonnegative_root_tile_index;

  if (ti.is_ancestor_of(root)) {
    ret_index = root;
  } else {
    if (ti != root && !root.is_ancestor_of(ti)) {
      // Tile isn't under root
      return false;
    }
    
    assert(tile_exists(root));
    ret_index = root;
    while (ret_index != ti) {
      TileIndex child = ti.start_time() < ret_index.left_child().end_time() ? ret_index.left_child() : ret_index.right_child();
      if (!tile_exists(child)) break;
      ret_index = child;
    }
  }
  // ret_index now holds closest ancestor to ti (or ti itself if it exists)  
    
  assert(read_tile(ret_index, ret));
  return true;
}

void Channel::read_bottommost_tiles_in_range(Range times,
                                             bool (*callback)(const Tile &t, Range times)) const {
  ChannelInfo info;
  bool success = read_info(info);
  if (!success) return;
  if (!info.times.intersects(times)) return;

  double time = times.min;
  TileIndex ti = TileIndex::null();
  while (time < times.max) {
    if (ti.is_null()) {
      ti = find_lowest_child_overlapping_time(info.nonnegative_root_tile_index, times.min);
    } else {
      ti = find_lowest_successive_tile(info.nonnegative_root_tile_index, ti);
    }
    if (ti.is_null() || ti.start_time() >= times.max) break;

    Tile t;
    assert(read_tile(ti, t));
    if (!(*callback)(t, times)) break;
  }
}

void Channel::read_tiles_in_range(Range times,
                   	          bool (*callback)(const Tile &t, Range times),
				  int desired_level) const {
  ChannelInfo info;
  bool success = read_info(info);
  if (!success) return;
  if (!info.times.intersects(times)) return;

  double time = times.min;
  TileIndex ti = TileIndex::null();
  while (time < times.max) {
    if (ti.is_null()) {
      ti = find_child_overlapping_time(info.nonnegative_root_tile_index, times.min, desired_level);
    } else {
      ti = find_successive_tile(info.nonnegative_root_tile_index, ti, desired_level);
    }
    if (ti.is_null() || ti.start_time() >= times.max) break;

    Tile t;
    assert(read_tile(ti, t));
    if (!(*callback)(t, times)) break;
  }
}

std::string Channel::descriptor() const {
  return string_printf("%d/%s", m_owner_id, m_name.c_str());
}

bool not_all_digits_filter(const char *subdir) {
  while (*subdir) {
    if (!isdigit(*subdir)) return true;
    subdir++;
  }
  return false;
}

void Channel::get_subchannel_names(KVS &kvs, int owner_id, 
				   const std::string &prefix, 
				   std::vector<std::string> &names, 
				   unsigned int nlevels) {
  std::string kvs_prefix = string_printf("%d", owner_id);
  if (prefix != "") kvs_prefix += "." + prefix;
  std::vector<std::string> keys;
  kvs.get_subkeys(kvs_prefix, keys, nlevels, not_all_digits_filter);
  for (unsigned i = 0; i < keys.size(); i++) {
    if (filename_suffix(keys[i]) == "info") {
      std::string name_with_uid = filename_sans_suffix(keys[i]);
      tassert(strchr(name_with_uid.c_str(), '.'));
      names.push_back(1+strchr(name_with_uid.c_str(), '.'));
    }
  }
}

TileIndex Channel::find_lowest_child_overlapping_time(TileIndex ti, double t) const {
  assert(!ti.is_null());
  // Start at root tile and move downwards

  while (1) {
    // Select correct child
    TileIndex child = t < ti.left_child().end_time() ? ti.left_child() : ti.right_child();
    
    if (child.is_null() || !tile_exists(child)) break;
    ti = child;
  }

  return ti;
}

TileIndex Channel::find_child_overlapping_time(TileIndex ti, double t, int desired_level) const {
  assert(!ti.is_null());
  // Start at root tile and move downwards

  while (ti.level > desired_level) {
    // Select correct child
    TileIndex child = t < ti.left_child().end_time() ? ti.left_child() : ti.right_child();
    
    if (child.is_null() || !tile_exists(child)) break;
    ti = child;
  }

  return ti;
}

TileIndex Channel::find_lowest_successive_tile(TileIndex root, TileIndex ti) const {
  // Move upwards until parent has a different end time
  while (1) {
    if (ti.parent().is_null()) return TileIndex::null();
    if (ti.parent().end_time() != ti.end_time()) break;
    ti = ti.parent();
    if (ti.level >= root.level) {
      // No more underneath the root
      return TileIndex::null();
    }
  }
  // We are now the left child of our parent;  skip to the right child
  ti = ti.sibling();

  return find_lowest_child_overlapping_time(ti, ti.start_time());
}

TileIndex Channel::find_successive_tile(TileIndex root, TileIndex ti, int desired_level) const {
  // Move upwards until parent has a different end time
  while (1) {
    if (ti.parent().is_null()) return TileIndex::null();
    if (ti.parent().end_time() != ti.end_time()) break;
    ti = ti.parent();
    if (ti.level >= root.level) {
      // No more underneath the root
      return TileIndex::null();
    }
  }
  // We are now the left child of our parent;  skip to the right child
  ti = ti.sibling();

  return find_child_overlapping_time(ti, ti.start_time(), desired_level);
}

std::string Channel::dump_tile_summaries() const {
  Locker lock(*this);  // Lock self and hold lock until exiting this method
  ChannelInfo info;
  bool success = read_info(info);
  if (success) {
    return dump_tile_summaries_internal(info.nonnegative_root_tile_index, 0);
  } else {
    return "";
  }
}

std::string Channel::dump_tile_summaries_internal(TileIndex ti, int level) const {
  Tile tile;
  if (!read_tile(ti, tile)) return "";
  std::string ret = string_printf("%*s%d.%d: %zd samples\n", level, "", ti.level, ti.offset, tile.double_samples.size());
  return ret + dump_tile_summaries_internal(ti.left_child(), level+1) + dump_tile_summaries_internal(ti.right_child(), level+1);
}

std::string Channel::key_prefix() const {
  return string_printf("%d.%s", m_owner_id, m_name.c_str());
}

std::string Channel::metainfo_key() const {
  return key_prefix() + ".info";
}

std::string Channel::tile_key(TileIndex ti) const {
  return string_printf("%s.%d.%lld", key_prefix().c_str(), ti.level, ti.offset);
}

bool Channel::tile_exists(TileIndex ti) const {
  return m_kvs.has_key(tile_key(ti));
}


