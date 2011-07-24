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

template <class V>
struct DataAccumulator {
  DataAccumulator() : time(0), value(0), weight(0), variance(0) {}

  DataAccumulator &operator+=(DataSample<V> &x) {
    time += x.time * x.weight;
    value += x.value * x.weight;
    weight += x.weight;
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
    return DataSample<V>(time/weight, value/weight, weight, variance/weight);
  }

  double time;
  V value;
  float weight;
  float variance;
};
  
#endif

