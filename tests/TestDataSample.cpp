// C++
#include <vector>


// Local
#include "DataSample.h"
#include "utils.h"

void accumulate2(DataAccumulator<double> &a, const std::vector<DataSample<double> > &samples) {
  DataAccumulator<double> an[2];
  for (unsigned i = 0; i < samples.size(); i++) an[i%2] += samples[i];
  a += an[0];
  a += an[1];
}

void accumulate2b(DataAccumulator<double> &a, const std::vector<DataSample<double> > &samples) {
  DataAccumulator<double> an[2];
  for (unsigned i = 0; i < samples.size(); i++) an[i%2] += samples[i];
  a += an[0].get_sample();
  a += an[1].get_sample();
}

void accumulate(DataAccumulator<double> &a, const std::vector<DataSample<double> > &samples) {
  for (unsigned i = 0; i < samples.size(); i++) a += samples[i];
}

int main(int argc, char **argv)
{

  std::vector<DataSample<double> > samples;
  samples.push_back(DataSample<double>(0, 0));
  samples.push_back(DataSample<double>(10, 1));
  samples.push_back(DataSample<double>(20, 2));
  samples.push_back(DataSample<double>(30, 3));
  samples.push_back(DataSample<double>(40, 4));
  {
    DataAccumulator<double> a;
    accumulate(a, samples);
    
    tassert_approx_equals(a.get_sample().time, 20);
    tassert_approx_equals(a.get_sample().value, 2.0);
    tassert_approx_equals(a.get_sample().stddev, sqrt(2.0));
    tassert_approx_equals(a.get_sample().weight, 5.0);
  }

  {
    DataAccumulator<double> a;
    accumulate2(a, samples);
    
    tassert_approx_equals(a.get_sample().time, 20);
    tassert_approx_equals(a.get_sample().value, 2);
    tassert_approx_equals(a.get_sample().stddev, sqrt(2.0));
    tassert_approx_equals(a.get_sample().weight, 5);
  }

  {
    DataAccumulator<double> a;
    accumulate2b(a, samples);
    
    tassert_approx_equals(a.get_sample().time, 20);
    tassert_approx_equals(a.get_sample().value, 2);
    tassert_approx_equals(a.get_sample().stddev, sqrt(2.0));
    tassert_approx_equals(a.get_sample().weight, 5);
  }
  return 0;
}
