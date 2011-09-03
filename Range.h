#ifndef RANGE_H
#define RANGE_H

// C++
#include <algorithm>
#include <limits>
#include <stdexcept>

// Local
#include "utils.h"

class Range {
public:
  double min;
  double max;
public:
  Range(double min, double max) : min(min), max(max) {}
  Range() { clear(); }
  static Range all() { 
    return Range(-std::numeric_limits<double>::max(), std::numeric_limits<double>::max()); 
  }
  static Range none() { return Range(); }
  void clear() {
    min = std::numeric_limits<double>::max();
    max = -std::numeric_limits<double>::max();
  }
  bool empty() const { return min > max; }
  void add(double x) {
    min = std::min(min, x);
    max = std::max(max, x);
  }
  void add(const Range &x) {
    min = std::min(min, x.min);
    max = std::max(max, x.max);
  }
  bool operator==(const Range &rhs) const { return min == rhs.min && max == rhs.max; }
  std::string to_string(const char *fmt="%g") const {
    return empty() ? "[null]" : string_printf((std::string(fmt)+":"+fmt).c_str(), min, max);
  }
  bool includes(double a) { return min <= a && a <= max; }
  bool intersects(const Range &a) {
    if (empty()) return false;
    if (a.empty()) return false;
    if (a.max < min) return false;
    if (max < a.min) return false;
    return true;
  }
};

#endif
