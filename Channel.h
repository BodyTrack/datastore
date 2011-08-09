#ifndef CHANNEL_INCLUDE_H
#define CHANNEL_INCLUDE_H

// C++
#include <string>
#include <vector>

// Local includes
#include "ChannelInfo.h"
#include "DataSample.h"
#include "KVS.h"
#include "Tile.h"

/// \class Channel Channel.h
///
/// Implements channel reference to KVS.
///
/// It's OK for there to be multiple Channel instances in multiple processes referring to the same channel in the KVS.

// TODO: compute these instead of hardcoding
#define BT_CHANNEL_MAX_TILE_SIZE (1024*1024)
#define BT_CHANNEL_DOUBLE_SAMPLES 32768
#define BT_CHANNEL_STRING_SAMPLES 8192

class Channel {
public:
  Channel(KVS &kvs, int owner_id, const std::string &name, size_t max_tile_size=BT_CHANNEL_MAX_TILE_SIZE);
  /// \class Locker Channel.h
  ///
  /// Lock channel upon construction and unlock channel upon destruction.
  ///
  /// Lock is an advisory lock only;  various calls to modify channel will be allowed whether Channel is locked or not, and regardless
  /// of which process might own the lock.  In order to implement proper synchronization, the caller for Channel should obtain lock for Channel
  /// when appropriate.
  class Locker {
  public:
    Locker(const Channel &ch);
  private:
    const Channel &m_ch;
    KVSLocker m_locker;
  };

  /// Read channel metainformation
  /// \param  info Returns metainformation, if read
  /// \return true if channel exists and read successful;  false if channel does not exist
  bool read_info(ChannelInfo &info) const;
  void write_info(const ChannelInfo &info);

  bool has_tile(TileIndex ti) const;
  bool read_tile(TileIndex ti, Tile &tile) const;
  void write_tile(TileIndex ti, const Tile &tile);
  bool delete_tile(TileIndex ti);
  void create_tile(TileIndex ti);
  
  void add_data(const std::vector<DataSample<double> > &data);
  void add_data(const std::vector<DataSample<std::string> > &data);
  void read_data(std::vector<DataSample<double> > &data, double begin, double end) const;
  
  std::string tile_key(TileIndex ti) const;
  bool tile_exists(TileIndex ti) const;
  std::string dump_tile_summaries() const;

  bool read_tile_or_closest_ancestor(TileIndex ti, TileIndex &ret_index, Tile &ret) const;
  void read_bottommost_tiles_in_range(double min_time, double max_time, bool (*callback)(const Tile &t, double min_time, double max_time)) const;

private:
  KVS &m_kvs;
  int m_owner_id;
  std::string m_name;
  size_t m_max_tile_size;
  std::string dump_tile_summaries_internal(TileIndex ti=TileIndex::null(), int level=0) const;

  std::string key_prefix() const;
  std::string metainfo_key() const;

  TileIndex find_lowest_child_overlapping_time(TileIndex ti, double t) const;
  TileIndex find_lowest_successive_tile(TileIndex root, TileIndex ti) const;
  
  TileIndex split_tile_if_needed(TileIndex ti, Tile &tile);
  void create_parent_tile_from_children(TileIndex ti, Tile &parent, Tile children[]);
  void move_root_upwards(TileIndex new_root, TileIndex old_root);
  template <class T>
  void add_data_internal(const std::vector<DataSample<T> > &data);
};

/// \class ChannelLocker Channel.h
///
/// Lock channel upon construction and unlock channel upon destruction.
///
/// Lock is an advisory lock only;  various calls to modify channel will be allowed whether Channel is locked or not, and regardless
/// of which process might own the lock.  In order to implement proper synchronization, the caller for Channel should obtain lock for Channel
/// when appropriate.
class ChannelLocker {
public:
  /// 
  ChannelLocker(Channel &ch);
  /// Unlock channel upon destruction
  ~ChannelLocker();
private:
  Channel &m_ch;
};

#endif
