// System
#include <assert.h>
#include <stdio.h>

// Local
#include "utils.h"

// Module to test
#include "Tile.h"

void test_double_samples()
{
  Tile t1, t2;
  std::string binary;
  t1.header.magic = Tile::MAGIC;
  t1.header.version = 0x00010000;
  tassert_equals(t1.ranges.times.empty(), true);
  tassert_equals(t1.ranges.double_samples.empty(), true);
  std::vector<DataSample<double> > samples;
  samples.push_back(DataSample<double>(1.11, 333.333));
  samples.push_back(DataSample<double>(2.22, 555.555));
  t1.insert_samples(&samples[0], &samples[samples.size()]);
  tassert_equals(t1.ranges.times.empty(), false);
  tassert_equals(t1.ranges.double_samples.empty(), false);
  tassert_approx_equals(t1.ranges.times.min, 1.11);
  tassert_approx_equals(t1.ranges.times.max, 2.22);
  tassert_approx_equals(t1.ranges.double_samples.min, 333.333);
  tassert_approx_equals(t1.ranges.double_samples.max, 555.555);
  
  t1.to_binary(binary);
  t2.from_binary(binary);
  tassert_equals(t2.header.magic, Tile::MAGIC);
  tassert_equals(t2.header.version, 0x00010000);
  tassert_equals(t2.double_samples.size(), 2);
  tassert_equals(t2.string_samples.size(), 0);
  tassert(t2.double_samples[0] == DataSample<double>(1.11, 333.333));
  tassert(t2.double_samples[1] == DataSample<double>(2.22, 555.555));
  tassert_equals(t2.ranges.times.empty(), false);
  tassert_equals(t2.ranges.double_samples.empty(), false);
  tassert_approx_equals(t2.ranges.times.min, 1.11);
  tassert_approx_equals(t2.ranges.times.max, 2.22);
  tassert_approx_equals(t2.ranges.double_samples.min, 333.333);
  tassert_approx_equals(t2.ranges.double_samples.max, 555.555);
}

void test_string_samples()
{
  Tile t1, t2;
  std::string binary;
  t1.header.magic = Tile::MAGIC;
  t1.header.version = 0x00010000;
  tassert_equals(t1.ranges.times.empty(), true);
  tassert_equals(t1.ranges.double_samples.empty(), true);
  std::vector<DataSample<std::string> > samples;
  samples.push_back(DataSample<std::string>(1.11, "abc"));
  samples.push_back(DataSample<std::string>(2.22, "def"));
  t1.insert_samples(&samples[0], &samples[samples.size()]);
  tassert_equals(t1.ranges.times.empty(), false);
  tassert_equals(t1.ranges.double_samples.empty(), true);
  tassert_approx_equals(t1.ranges.times.min, 1.11);
  tassert_approx_equals(t1.ranges.times.max, 2.22);
  
  t1.to_binary(binary);
  t2.from_binary(binary);
  tassert_equals(t2.header.magic, Tile::MAGIC);
  tassert_equals(t2.header.version, 0x00010000);
  tassert_equals(t2.double_samples.size(), 0);
  tassert_equals(t2.string_samples.size(), 2);
  tassert(t2.string_samples[0] == DataSample<std::string>(1.11, "abc"));
  tassert(t2.string_samples[1] == DataSample<std::string>(2.22, "def"));
  tassert_equals(t2.ranges.times.empty(), false);
  tassert(t2.ranges.double_samples.empty());
  tassert_equals(t2.ranges.double_samples.empty(), true);
  tassert_approx_equals(t2.ranges.times.min, 1.11);
  tassert_approx_equals(t2.ranges.times.max, 2.22);
}

int main(int argc, char **argv)
{
  test_double_samples();
  test_string_samples();
  
  // Done
  fprintf(stderr, "Tests succeeded\n");
  return 0;
}
