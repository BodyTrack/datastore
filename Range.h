#ifndef RANGE_H
#define RANGE_H

// C++
#include <algorithm>
#include <limits>
#include <stdexcept>

// Local
#include "utils.h"

class Range {
//private:
public:
  double m_min;
  double m_max;
public:
  Range(double min, double max) : m_min(min), m_max(max) {}
  Range() { clear(); }
  void clear() {
    m_min = std::numeric_limits<double>::max();
    m_max = std::numeric_limits<double>::min();
  }
  bool empty() const { return m_min > m_max; }
  double min() const { if (empty()) throw std::runtime_error("Range is null"); return m_min; }
  double max() const { if (empty()) throw std::runtime_error("Range is null"); return m_max; }
  void add(double x) {
    m_min = std::min(m_min, x);
    m_max = std::max(m_max, x);
  }
  void add(const Range &x) {
    m_min = std::min(m_min, x.m_min);
    m_max = std::max(m_max, x.m_max);
  }
  bool operator==(const Range &rhs) const { return m_min == rhs.m_min && m_max == rhs.m_max; }
  std::string to_string() const {
    return empty() ? "[null]" : string_printf("%g:%g", m_min, m_max);
  }
};

#endif
