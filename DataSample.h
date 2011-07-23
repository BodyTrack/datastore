#ifndef INCLUDE_DATA_SAMPLE_H
#define INCLUDE_DATA_SAMPLE_H

template <class V>
struct DataSample {
  DataSample(double time_init, V value_init, float weight_init, float variance_init) :
    time(time_init), value(value_init), weight(weight_init),  variance(variance) {}
  DataSample(double time_init, V value_init) :
    time(time_init), value(value_init), weight(1),  variance(0) {}
  DataSample() : time(0), value(0), weight(1), variance(0) {}
  double time;
  V value;
  float weight;
  float variance;
  bool operator==(const DataSample<V> &rhs) const {
    return time == rhs.time && value == rhs.value && weight == rhs.weight && variance == rhs.variance;
  }
  static bool time_lessthan(const DataSample<V> &a, const DataSample<V> &b) {
    return a.time < b.time;
  }
};

#endif

