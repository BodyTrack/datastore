#ifndef INCLUDE_DATA_SAMPLE_H
#define INCLUDE_DATA_SAMPLE_H

// C++
#include <string>

template <class V>
struct DataSample {
  DataSample(double time_init, V value_init, float weight_init, float variance_init) :
    time(time_init), value(value_init), weight(weight_init),  variance(variance) {}
  DataSample(double time_init, V value_init) :
    time(time_init), value(value_init), weight(1),  variance(0) {}
  DataSample() : time(0), value(V()), weight(1), variance(0) {}
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

template <class V>
struct DataAccumulator {
  DataAccumulator() : time(0), value(V()), weight(0), variance(0) {}
  
  DataAccumulator &operator+=(const DataSample<V> &x) {
    plus_equals(*this, x);
    // TODO: variance
    //http://www.emathzone.com/tutorials/basic-statistics/combined-variance.html
    //# Neither tp1 nor tp2 are nil, combine them
    //total_count = tp1.count + tp2.count
    //ave_time = ((tp1.time*tp1.count) + (tp2.time*tp2.count))/total_count
    //ave_mean = ((tp1.mean*tp1.count) + (tp2.mean*tp2.count))/total_count
    //
    //# Calculating combined "stddev" based on formulae at 
    //# http://www.emathzone.com/tutorials/basic-statistics/combined-variance.html
    //sc_2 = (tp1.count*(tp1.stderr + (tp1.mean - ave_mean)**2) + 
    //        tp2.count*(tp2.stderr + (tp2.mean - ave_mean)**2))/total_count
    //return(TilePoint.new(ave_time, ave_mean, Math.sqrt(sc_2), total_count))
    return *this;
  }

  DataSample<V> get_sample() const {
    DataSample<V> ret;
    get(*this, ret);
    return ret;
  }

  double time;
  V value;
  float weight;
  float variance;

private:
    static void plus_equals(DataAccumulator<double> &lhs, const DataSample<double> &rhs) {
    lhs.time += rhs.time * rhs.weight;
    lhs.value += rhs.value * rhs.weight;
    lhs.weight += rhs.weight;
  }
  
  static void plus_equals(DataAccumulator<std::string> &lhs, const DataSample<std::string> &rhs) {
    lhs.time += rhs.time * rhs.weight;
    if (lhs.weight==0) {
      lhs.value=rhs.value;
    } else if (lhs.value != rhs.value) {
      lhs.value="<multiple>"; // TODO: should we do some hashing to speed keyword search?
    }
    lhs.weight += rhs.weight;
  }

  static void get(const DataAccumulator<double> &a, DataSample<double> &ret) {
    ret = DataSample<double>(a.time/a.weight, a.value/a.weight, a.weight, a.variance/a.weight);
  }

  static void get(const DataAccumulator<std::string> &a, DataSample<std::string> &ret) {
    ret = DataSample<std::string>(a.time/a.weight, a.value, a.weight, a.variance/a.weight);
  }
};

#endif
