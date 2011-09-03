#ifndef INCLUDE_DATA_SAMPLE_H
#define INCLUDE_DATA_SAMPLE_H

// C
#include <math.h>

// C++
#include <string>

// JSON
#include <json/json.h>

// Local
#include "Range.h"

template <class V>
struct DataSample {
  DataSample(double time_init, V value_init, float weight_init, float stddev_init) :
    time(time_init), value(value_init), weight(weight_init),  stddev(stddev_init) {}
  DataSample(double time_init, V value_init) :
    time(time_init), value(value_init), weight(1),  stddev(0) {}
  DataSample() : time(0), value(V()), weight(1), stddev(0) {}
  double time;
  V value;
  float weight;
  float stddev;
  bool operator==(const DataSample<V> &rhs) const {
    return time == rhs.time && value == rhs.value && weight == rhs.weight && stddev == rhs.stddev;
  }
  static bool time_lessthan(const DataSample<V> &a, const DataSample<V> &b) {
    return a.time < b.time;
  }
};

template <class V>
struct DataAccumulator {
  DataAccumulator() : timesum(0), sum(V()), sumsq(V()), weight(0) {}

  DataAccumulator &operator+=(const DataAccumulator<V> &x) {
    plus_equals(*this, x);
    return *this;
  }

  DataAccumulator &operator+=(const DataSample<V> &x) {
    plus_equals(*this, x);
    return *this;
  }

  DataSample<V> get_sample() const {
    DataSample<V> ret;
    get(*this, ret);
    return ret;
  }

  double timesum;
  V sum;
  V sumsq;
  double weight;

private:
  static void plus_equals(DataAccumulator<double> &lhs, const DataAccumulator<double> &rhs) {
    lhs.timesum += rhs.timesum;
    lhs.sum += rhs.sum;
    lhs.sumsq += rhs.sumsq;
    lhs.weight += rhs.weight;
  }
  
  static void plus_equals(DataAccumulator<double> &lhs, const DataSample<double> &rhs) {
    if (rhs.weight == 0) return;
    lhs.timesum += rhs.time * rhs.weight;
    lhs.sum += rhs.value * rhs.weight;
    
    // variance = sumsq/weight - mean^2
    // sumsq/weight = variance + mean^2
    // sumsq = weight*variance + sum*mean = weight(variance + mean*mean) = weight(stddev*stddev + mean*mean)
    double rhs_sumsq = rhs.weight * (rhs.stddev * rhs.stddev + rhs.value * rhs.value);
    
    lhs.sumsq += rhs_sumsq;
    lhs.weight += rhs.weight;
  }
  
  static void plus_equals(DataAccumulator<std::string> &lhs, const DataSample<std::string> &rhs) {
    lhs.timesum += rhs.time * rhs.weight;
    if (lhs.weight==0) {
      lhs.sum=rhs.value;
    } else if (lhs.sum != rhs.value) {
      lhs.sum="<multiple>"; // TODO: should we do some hashing to speed keyword search?
    }
    lhs.weight += rhs.weight;
  }

  static void get(const DataAccumulator<double> &a, DataSample<double> &ret) {
    double mean = a.sum/a.weight;
    double variance = a.sumsq/a.weight - mean*mean;
    ret = DataSample<double>(a.timesum/a.weight, mean, a.weight, sqrt(variance));
  }
  
  static void get(const DataAccumulator<std::string> &a, DataSample<std::string> &ret) {
    ret = DataSample<std::string>(a.timesum/a.weight, a.sum, a.weight, 0);
  }
};

struct DataRanges {
  Range times;
  Range double_samples;
  void clear() { times.clear(); double_samples.clear(); }
  bool operator==(const DataRanges &rhs) const {
    return times==rhs.times && double_samples==rhs.double_samples;
  }
  void add(const DataRanges &rhs) {
    times.add(rhs.times);
    double_samples.add(rhs.double_samples);
  }
  Json::Value to_json() const {
    Json::Value ret(Json::objectValue);
    if (!times.empty()) {
      ret["min_time"] = times.min;
      ret["max_time"] = times.max;
    }
    if (!double_samples.empty()) {
      ret["min_value"] = double_samples.min;
      ret["max_value"] = double_samples.max;
    }
    return ret;
  }
};
  
#endif
