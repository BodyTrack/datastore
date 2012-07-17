#ifndef INCLUDE_TILE_INDEX_H
#define INCLUDE_TILE_INDEX_H

// System
#include <assert.h>
#include <limits.h>
#include <limits>
#include <math.h>

// Local
#include "Range.h"
#include "utils.h"
#include "sizes.h"

struct TileIndex {
  int32 level;
  int64 offset;

  TileIndex(int level_init, long long offset_init) :
    level(level_init), offset(offset_init) {}

  TileIndex() : level(0), offset(-1LL) {}

  static TileIndex nonnegative_all() {
    return TileIndex(INT_MAX, 0);
  }

  static TileIndex negative_all() {
    return TileIndex(INT_MAX, -1);
  }

  static TileIndex null() {
    return TileIndex(INT_MIN, 0);
  }

  static int32_t lowest_level() {
    return INT_MIN;
  }

  bool is_nonnegative_all() const {
    return level == INT_MAX && offset == 0;
  }

  bool is_negative_all() const {
    return level == INT_MAX && offset == -1;
  }

  bool is_all() const {
    return level == INT_MAX;
  }

  bool is_null() const {
    return level == INT_MIN && offset == 0;
  }

  double start_time() const {
    if (is_null()) return std::numeric_limits<double>::max();
    if (is_negative_all()) return -std::numeric_limits<double>::max();
    if (is_nonnegative_all()) return 0;
    return duration()*offset;
  }

  double end_time() const {
    if (is_null()) return -std::numeric_limits<double>::max();
    if (is_negative_all()) return 0;
    if (is_nonnegative_all()) return std::numeric_limits<double>::max();
    return duration()*(offset+1);
  }

  bool contains_time(double time) const {
    return start_time() <= time && time < end_time();
  }

  double position(double time) const {
    return (time - start_time()) / duration();
  }

  double duration() const {
    return level_to_duration(level);
  }

  static double level_to_duration(int level) {
    return pow(2.0, level);
  }
  
  static double duration_to_level(double duration) {
    return log2(duration);
  }

  static double level_to_bin_secs(int level) {
    assert(0);
  }

  std::string to_string() const {
    if (is_null()) return "[null]";
    if (is_nonnegative_all()) return "[nonnegative_all]";
    if (is_negative_all()) return "[negative_all]";
    return string_printf("%d.%lld", level, offset);
  }

  static TileIndex index_at_level_containing(int level, double time) {
    return TileIndex(level, (long long)floor(time / level_to_duration(level)));
  }

  /// Select level according to max_time-min_time, selecting tile that contains min_time, then move to parents until max_time is contained
  /// Only works if min and max times are both negative, or nonnegative
  static TileIndex index_containing(Range times) {
    assert(times.min < times.max);
    assert(times.min >= 0 || times.max < 0);
    int level = (int)floor(log2(times.max - times.min));
    TileIndex ret = index_at_level_containing(level, times.min);
    while (!ret.contains_time(times.max)) ret = ret.parent();
    //fprintf(stderr, "index_containing(%g, %g)=%s\n", times.min, times.max, ret.to_string().c_str());
    return ret;
  }

  bool operator==(const TileIndex &rhs) const {
    return level == rhs.level && offset == rhs.offset;
  }

  bool operator!=(const TileIndex &rhs) const {
    return !(*this == rhs);
  }

  /// Less than.  Tiles are ordered first by level, then by offset
  bool operator<(const TileIndex &rhs) const {
    return level == rhs.level ? offset < rhs.offset : level < rhs.level;
  }

  // Is TileIndex ancestor of child?
  // \return true if TileIndex is parent of child, or a higher ancestor.  false if not.  false if identical to child
  bool is_ancestor_of(const TileIndex &child) const {
    return (level > child.level) && (offset == (child.offset >> (unsigned)(level-child.level)));
  }

  /// Return parent
  /// \return parent of TileIndex
  TileIndex parent() const {
    if (is_all()) return null();
    return TileIndex(level+1, offset>>1);
  }

  /// Return left child
  /// \return left child of TileIndex
  TileIndex left_child() const {
    if (is_all()) return null();
    return TileIndex(level-1, offset*2);
  }

  /// Return right child
  /// \return right child of TileIndex
  TileIndex right_child() const {
    if (is_all()) return null();
    return TileIndex(level-1, offset*2+1);
  }

  TileIndex sibling() const {
    return TileIndex(level, offset ^ 1);
  }
  
};

#endif
