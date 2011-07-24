// System
#include <assert.h>
#include <set>
#include <stdexcept>
#include <stdio.h>
#include <string.h>

// Local
#include "TileIndex.h"
#include "utils.h"

// Self
#include "Channel.h"

/// Create channel reference to KVS
/// \param owner_id  Owner of channel
/// \param name      Full name of channel (may be of form device_nickname.channel_name)
Channel::Channel(KVS &kvs, int owner_id, const std::string &name, size_t max_tile_size)
  : m_kvs(kvs), m_owner_id(owner_id), m_name(name), m_max_tile_size(max_tile_size) {}

/// Lock channel upon construction; if currently locked, construction will block until lock is available
/// \param ch        Channel to lock
Channel::Locker::Locker(const Channel &ch) : m_ch(ch), m_locker(ch.m_kvs, ch.metainfo_key()) {}

/// Read channel metainformation from KVS
/// \param  info Returns metainformation, if read
/// \return true if channel exists in KVS and read successful;  false if channel does not exist in KVS
/// Channel exists if metainfo_key (.info) exists and is of non-zero size.  File may exist and be of zero size
/// if the channel is locked before it is created.
bool Channel::read_info(ChannelInfo &info) const {
  std::string info_str;
  if (m_kvs.get(metainfo_key(), info_str) && info_str != "") {
    //fprintf(stderr, "got key %s, length %zd\n", metainfo_key().c_str(), info_str.length());
    assert(info_str.length() == sizeof(ChannelInfo));
    memcpy((void*)&info, (void*)info_str.c_str(), sizeof(info));
    assert(info.magic == ChannelInfo::MAGIC);
    return true;
  } else {
    return false;
  }
}

/// Write channel metainformation to KVS
/// \param  info New metainformation for channel
/// Creates channel in KVS if it doesn't already exist
void Channel::write_info(const ChannelInfo &info) {
  std::string info_str((char*)&info, (char*)((&info)+1));
  m_kvs.set(metainfo_key(), info_str);
}

bool Channel::has_tile(TileIndex ti) const {
  return m_kvs.has_key(tile_key(ti));
}

bool Channel::read_tile(TileIndex ti, Tile &tile) const {
  std::string binary;
  if (!m_kvs.get(tile_key(ti), binary)) return false;
  tile.from_binary(binary);
  return true;
}

void Channel::write_tile(TileIndex ti, const Tile &tile) {
  std::string binary;
  tile.to_binary(binary);
  assert(binary.size() <= m_max_tile_size);
  m_kvs.set(tile_key(ti), binary);
}

bool Channel::delete_tile(TileIndex ti) {
  return m_kvs.del(tile_key(ti));
}

void Channel::create_tile(TileIndex ti) {
  Tile tile;
  write_tile(ti, tile);
}

/// Add data to channel
/// \param data Data to add;  must be sorted in ascending time
/// Locking:  This method acquires a lock to channel as needed to guarantee update is successful in an environment
/// where multiple simultaneous updates are happening via add_data from multiple processes.
void Channel::add_data(const std::vector<DataSample<double> > &data) {
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
    info.min_time = data[0].time;
    info.max_time = data.back().time;
    info.nonnegative_root_tile_index = TileIndex::nonnegative_all();
    create_tile(TileIndex::nonnegative_all());
    info.negative_root_tile_index = TileIndex::null();
  } else {
    info.min_time = std::min(info.min_time, data[0].time);
    info.max_time = std::max(info.max_time, data.back().time);
    TileIndex new_nonnegative_root_tile_index = TileIndex::index_containing(info.min_time, info.max_time);
    if (new_nonnegative_root_tile_index.level > info.nonnegative_root_tile_index.level) {
      // Root index changed.  Confirm new root is parent or more distant ancestor of old root
      assert(new_nonnegative_root_tile_index.is_ancestor_of(info.nonnegative_root_tile_index));
      // Trigger regeneration from old root's parent, up through new root
      to_regenerate.insert(info.nonnegative_root_tile_index.parent());
      move_root_upwards(new_nonnegative_root_tile_index, info.nonnegative_root_tile_index);
      info.nonnegative_root_tile_index = new_nonnegative_root_tile_index;
    }
  }

  unsigned i=0;

  while (i < data.size()) {
    TileIndex ti= find_lowest_child_overlapping_time(info.nonnegative_root_tile_index, data[i].time);
    assert(!ti.is_null());

    Tile tile;
    assert(read_tile(ti, tile));
    //fprintf(stderr, "add_data: reading tile %s\n", ti.to_string().c_str());
    const DataSample<double> *begin = &data[i];
    while (i < data.size() && ti.contains_time(data[i].time)) i++;
    const DataSample<double> *end = &data[i];
    tile.insert_double_samples(begin, end);
    
    //fprintf(stderr, "add_data: added %zd samples\n", end-begin);
    TileIndex new_root = split_tile_if_needed(ti, tile);
    if (new_root != TileIndex::null()) {
      assert(ti == TileIndex::nonnegative_all());
      fprintf(stderr, "Changing root from %s to %s\n", ti.to_string().c_str(), new_root.to_string().c_str());
      info.nonnegative_root_tile_index = new_root;
      delete_tile(ti); // Delete old root
      ti = new_root;
    }
    //fprintf(stderr, "add_data: rewriting tile %s\n", ti.to_string().c_str());
    write_tile(ti, tile);
    if (ti != info.nonnegative_root_tile_index) to_regenerate.insert(ti.parent());
  }
  
  // Regenerate from lowest level to highest
  while (!to_regenerate.empty()) {
    TileIndex ti = *to_regenerate.begin();
    to_regenerate.erase(to_regenerate.begin());
    //fprintf(stderr, "add_data: regenerating tile %s\n", ti.to_string().c_str());
    Tile regenerated, children[2];
    assert(read_tile(ti.left_child(), children[0]));
    assert(read_tile(ti.right_child(), children[1]));
    create_parent_tile_from_children(ti, regenerated, children);
    write_tile(ti, regenerated);
    if (ti != info.nonnegative_root_tile_index) to_regenerate.insert(ti.parent());
  }
  write_info(info);
}

void Channel::read_data(std::vector<DataSample<double> > &data, double begin, double end) const {
  //fprintf(stdout, "read_data(%f,%f)\n", begin, end);
  double time = begin;
  data.clear();

  Locker lock(*this);  // Lock self and hold lock until exiting this method

  ChannelInfo info;
  bool success = read_info(info);
  if (!success) {
    // Channel doesn't yet exist;  no data
    fprintf(stderr, "read_data: can't read info\n");
    return;
  }
  bool first_tile = true;

  while (time < end) {
    TileIndex ti = find_lowest_child_overlapping_time(info.nonnegative_root_tile_index, time);
    if (ti.is_null()) {
      // No tiles; no more data
      fprintf(stderr, "read_data: can't read tile, done\n");
      return;
    }

    Tile tile;
    assert(read_tile(ti, tile));
    unsigned i = 0;
    if (first_tile) {
      // Skip any samples before requested time
      for (; i < tile.double_samples.size() && tile.double_samples[i].time < begin; i++);
    }

    //size_t before = data.size();
    for (; i < tile.double_samples.size() && tile.double_samples[i].time < end; i++) {
      data.push_back(tile.double_samples[i]);
    }
    //fprintf(stdout, "Got %zd samples from %s\n", data.size() - before, ti.to_string().c_str());
    time = ti.end_time();
  }
}

/// Split tile if needed (if it's too large)
/// If we're looking to split an "all" tile (negative_all or nonnegative_all), select a new root tile.
/// \param ti Tile Index to split if needed
/// \param tile tile to split if needed (tile that's pointed to by ti.  might not be written yet to the datastore)
/// \return Normally returns TileIndex::null(), but in case of splitting "all" tile, returns new root

TileIndex Channel::split_tile_if_needed(TileIndex ti, Tile &tile) {
  TileIndex new_root_index = TileIndex::null();
  if (tile.binary_length() <= m_max_tile_size) return new_root_index;
  //fprintf(stderr, "split_tile_if_needed: splitting tile %s\n", ti.to_string().c_str());
  Tile children[2];
  TileIndex child_indexes[2];

  // If we're splitting an "all" tile, it means that until now the channel has only had one tile's worth of
  // data, and that a proper root tile location couldn't be selected.  Select a new root tile now.
  if (ti.is_nonnegative_all()) {
    // TODO: this breaks if all samples are at one time
    ti = new_root_index = TileIndex::index_containing(tile.double_samples[0].time, tile.double_samples.back().time);
    fprintf(stderr, "Moving root tile to %s\n", ti.to_string().c_str());
  }
  
  child_indexes[0]= ti.left_child();
  child_indexes[1]= ti.right_child();

  double split_time = ti.right_child().start_time();
  size_t split_index;
  //fprintf(stderr, "split_index = 0, tile.double_samples[split_index].time = %g, split_time = %g\n",
  //        tile.double_samples[0].time, split_time);
  for (split_index = 0;
       split_index < tile.double_samples.size() && tile.double_samples[split_index].time < split_time;
       split_index++) {
    //fprintf(stderr, "split_index = %zd, tile.double_samples[split_index].time = %g, split_time = %g\n",
    //split_index, tile.double_samples[split_index].time, split_time);
  }
  //fprintf(stderr, "split_index is %zd\n", split_index);
  children[0].insert_double_samples(&tile.double_samples[0], &tile.double_samples[split_index]);
  children[1].insert_double_samples(&tile.double_samples[split_index], &tile.double_samples[tile.double_samples.size()]);
  //fprintf(stderr, "Splitting %zd samples at time %g into (%zd, %zd) samples\n",
  //        tile.double_samples.size(), split_time,
  //        children[0].double_samples.size(),  children[1].double_samples.size());
  
  for (int i = 0; i < 2; i++) {
    assert(!has_tile(child_indexes[i]));
    assert(split_tile_if_needed(child_indexes[i], children[i]) == TileIndex::null());
    write_tile(child_indexes[i], children[i]);
  }
  create_parent_tile_from_children(ti, tile, children);
  return new_root_index;
}

void Channel::create_parent_tile_from_children(TileIndex parent_index, Tile &parent, Tile children[]) {
  // Subsample the children to create the parent
  // when do we want to show original values?
  // when do we want to do a real low-pass filter?
  // do we need to filter more than just the child tiles? e.g. gaussian beyond the tile border

  unsigned n_samples = 50000; // TODO: compute this number instead of hardcoding
  parent.double_samples.resize(n_samples);
  assert(parent.binary_length() <= m_max_tile_size);

  double start_time = parent_index.start_time(), end_time = parent_index.end_time();
  for (unsigned i = 0; i < n_samples; i++) {
    parent.double_samples[i].time = start_time + (end_time-start_time) * (i + 0.5) / n_samples;
    parent.double_samples[i].value = 0;
    parent.double_samples[i].weight = 0;
    parent.double_samples[i].variance = 0;
  }

  for (unsigned j = 0; j < 2; j++) {
    Tile &child = children[j];
    for (unsigned i = 0; i < child.double_samples.size(); i++) {
      // Version 1: bin samples into correct bin
      // Version 2: try gaussian or lanczos(1) or 1/4 3/4 3/4 1/4
      // TODO: use DataAccumulator

      DataSample<double> &sample = child.double_samples[i];
      assert(start_time <= sample.time && sample.time < end_time);
      unsigned bin = floor((sample.time-start_time) * n_samples / (end_time - start_time));
      assert(bin < n_samples);
      parent.double_samples[bin].value += sample.value;
      parent.double_samples[bin].weight += sample.weight;
      // TODO: compute variance
    }
  }

  for (unsigned i = 0; i < parent.double_samples.size(); i++) {
    DataSample<double> &sample = parent.double_samples[i];
    if (sample.weight != 0) {
      sample.value /= sample.weight;
      sample.weight /= 2;
    }
  }
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

bool Channel::read_tile_or_closest_ancestor(TileIndex ti, TileIndex &ret_index, Tile &ret) const {
  Locker lock(*this);  // Lock self and hold lock until exiting this method
  ChannelInfo info;
  bool success = read_info(info);
  if (!success) return false;
  TileIndex root = info.nonnegative_root_tile_index;

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
  // ret_index now holds closest ancestor to ti (or ti itself if it exists)  
  assert(read_tile(ret_index, ret));
  return true;
}

TileIndex Channel::find_lowest_child_overlapping_time(TileIndex ti, double t) const {
  // Start at root tile and move downwards
  // There are no children of an "all" tile
  if (ti.is_nonnegative_all() || ti.is_negative_all()) return ti;

  while (1) {
    // Select correct child
    TileIndex child = t < ti.left_child().end_time() ? ti.left_child() : ti.right_child();
    if (!tile_exists(child)) break;
    ti = child;
  }

  return ti;
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


