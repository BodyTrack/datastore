// C
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

// BOOST
#include <boost/thread/thread.hpp>

// Local
#include "FilesystemKVS.h"

// Module to test
#include "Channel.h"

void test_samples_single_tile(Channel &ch, double begin_time, size_t num_samples)
{
  fprintf(stderr, "test_samples_single_tile(%zd):\n", num_samples);
  std::vector<DataSample<double> > data(num_samples);
  {
    long long begin = millitime();
    for (size_t i = 0; i < num_samples; i++) {
      data[i] = DataSample<double>(begin_time + i, 1 + i);
    }
    ch.add_data(data);
    fprintf(stderr, "  inserted %zd samples in %lld msec\n", num_samples, millitime() - begin);
  }
  
  Tile tile;
  {
    long long begin = millitime();
    tassert(ch.read_tile(TileIndex::nonnegative_all(), tile));
    fprintf(stderr, "  read %zd samples in %lld msec\n", num_samples, millitime() - begin);
  }

  tassert(tile.double_samples == data);
}

void test_subsampling(KVS &kvs)
{
  Channel ch(kvs, 2, "a.d");
  size_t num_samples=100000;
  std::vector<DataSample<double> > data(num_samples);
  for (size_t i = 0; i < num_samples; i++) {
    data[i] = DataSample<double>(i+1, 33);
  }
  ch.add_data(data);
  // Fetch top-level tile
  Tile tile;
  tassert(ch.read_tile(TileIndex(17, 0), tile));

  DataAccumulator<double> a;
  
  for (size_t i = 0; i < tile.double_samples.size(); i++) {
    a += tile.double_samples[i];
    tassert(fabs(tile.double_samples[i].value - 33) < 1e-10);
  }
  tassert_approx_equals(a.get_sample().time, 100001*.5);
  tassert_approx_equals(a.get_sample().weight, num_samples);
  tassert_approx_equals(a.get_sample().value, 33);
  tassert_approx_equals(a.get_sample().stddev, 0);
  tassert_equals(tile.ranges.times.min, 1);
  tassert_equals(tile.ranges.times.max, 100000);
  tassert_equals(tile.ranges.double_samples.min, 33);
  tassert_equals(tile.ranges.double_samples.max, 33);
}

void test_subsampling_stddev(KVS &kvs)
{
  Channel ch(kvs, 2, "a.d.stddev");
  size_t num_samples=100000;
  std::vector<DataSample<double> > data(num_samples);
  for (size_t i = 0; i < num_samples; i++) {
    data[i] = DataSample<double>(i+1, i%10);
  }
  ch.add_data(data);
  // Fetch top-level tile
  Tile tile;
  tassert(ch.read_tile(TileIndex(17, 0), tile));

  DataAccumulator<double> a;
  
  for (size_t i = 0; i < tile.double_samples.size(); i++) {
    a += tile.double_samples[i];
  }
  tassert_approx_equals(a.get_sample().time, 100001*.5);
  tassert_approx_equals(a.get_sample().weight, num_samples);
  tassert_approx_equals(a.get_sample().value, 4.5);
  tassert_approx_equals(a.get_sample().stddev, sqrt(33.0)/2.0);
  tassert_equals(tile.ranges.times.min, 1);
  tassert_equals(tile.ranges.times.max, 100000);
  tassert_equals(tile.ranges.double_samples.min, 0);
  tassert_equals(tile.ranges.double_samples.max, 9);
}


void test_subsampling_string(KVS &kvs)
{
  Channel ch(kvs, 2, "a.d.string");
  size_t num_samples=100000;
  std::vector<DataSample<std::string> > data(num_samples);
  for (size_t i = 0; i < num_samples; i++) {
    data[i] = DataSample<std::string>(i+1, "foo bar barf yoyodyne lalalala");
  }
  ch.add_data(data);
  // Fetch top-level tile
  Tile tile;
  tassert(ch.read_tile(TileIndex(17, 0), tile));
  double total_weight = 0;
  for (size_t i = 0; i < tile.string_samples.size(); i++) {
    DataSample<std::string> &sample = tile.string_samples[i];
    total_weight += sample.weight;
    tassert(sample.value == "foo bar barf yoyodyne lalalala");
  }
  tassert_approx_equals(total_weight, num_samples);
  tassert_equals(tile.ranges.times.min, 1);
  tassert_equals(tile.ranges.times.max, 100000);
  tassert(tile.ranges.double_samples.empty());
}

void test_samples_multiple_tiles(Channel &ch, double begin_time, size_t num_samples)
{
  fprintf(stderr, "test_samples_multiple_tiles(%zd):\n", num_samples);
  std::vector<DataSample<double> > data(num_samples);
  for (size_t i = 0; i < num_samples; i++) {
    data[i] = DataSample<double>(begin_time + i, 1 + i);
  }
  {
    long long begin = millitime();
    ch.add_data(data);
    fprintf(stderr, "  inserted %zd samples in %lld msec\n", num_samples, millitime() - begin);
  }

  std::vector<DataSample<double> > read_data;
  {
    long long begin = millitime();
    ch.read_data(read_data, data[0].time, data.back().time+1);
    fprintf(stderr, "  read %zd samples in %lld msec\n", num_samples, millitime() - begin);
  }
  tassert(data == read_data);
  fprintf(stderr, "test_samples_multiple_tiles(%zd) succeeded\n", num_samples);
}

void test_add_data_thread(std::vector<DataSample<double> > data)
{
  FilesystemKVS kvs("channelstore_test.kvs");
  Channel ch(kvs, 2, "threadtest");
  ch.add_data(data);
}

void test_subsampling_threads()
{
  int nthreads = 100;
  int samples_per_thread = 1000;
  std::vector<boost::thread*> threads;
  for (int thread = 0; thread < nthreads; thread++) {
    std::vector<DataSample<double> > data(samples_per_thread);
    for (int i = 0; i < samples_per_thread; i++) {
      data[i] = DataSample<double>(i+thread*samples_per_thread, 33);
    }
    threads.push_back(new boost::thread(test_add_data_thread, data));
  }
  for (unsigned int i=0; i<threads.size(); i++) {
    threads[i]->join();
    delete threads[i];
  }
  
  // Fetch top-level tile
  Tile tile;
  FilesystemKVS kvs("channelstore_test.kvs");
  Channel ch(kvs, 2, "threadtest");
  
  tassert(ch.read_tile(TileIndex(17, 0), tile));

  DataAccumulator<double> a;
  
  for (size_t i = 0; i < tile.double_samples.size(); i++) {
    a += tile.double_samples[i];
    tassert(fabs(tile.double_samples[i].value - 33) < 1e-10);
  }
  tassert_approx_equals(a.get_sample().time, 99999*.5);
  tassert_approx_equals(a.get_sample().weight, nthreads * samples_per_thread);
  tassert_approx_equals(a.get_sample().value, 33);
  tassert_approx_equals(a.get_sample().stddev, 0);
}

int main(int argc, char **argv) {
  system("rm -rf channelstore_test.kvs");
  system("mkdir channelstore_test.kvs");
  FilesystemKVS kvs("channelstore_test.kvs");
  kvs.set_verbosity(0);

  Channel ch(kvs, 2, "a.b");
  {
    Channel::Locker locker(ch);
  }

  // Test level_from_rate
  tassert_approx_equals(ch.level_from_rate(   0.5), 16);
  tassert_approx_equals(ch.level_from_rate(     1), 15);
  tassert_approx_equals(ch.level_from_rate(  8192),  2);
  tassert_approx_equals(ch.level_from_rate( 16384),  1);
  tassert_approx_equals(ch.level_from_rate( 32768),  0);
  tassert_approx_equals(ch.level_from_rate( 65536), -1);
  tassert_approx_equals(ch.level_from_rate(131072), -2);

  // Test ChannelInfo

  {
    ChannelInfo info;
    tassert(!ch.read_info(info));
    info.magic = ChannelInfo::MAGIC;
    info.version = 0x00010000;
    info.times.min = 1;
    info.times.max = 2;
    info.nonnegative_root_tile_index = TileIndex::nonnegative_all();
    info.negative_root_tile_index = TileIndex::negative_all();
    ch.write_info(info);
  }
  {
    ChannelInfo info2;
    tassert(ch.read_info(info2));
    tassert_equals(info2.magic, ChannelInfo::MAGIC);
    tassert_equals(info2.version, 0x00010000);
    tassert_equals(info2.times.min, 1);
    tassert_equals(info2.times.max, 2);
    tassert(info2.nonnegative_root_tile_index == TileIndex::nonnegative_all());
    tassert(info2.negative_root_tile_index == TileIndex::negative_all());
  }

  // Test paths
  tassert(ch.tile_key(TileIndex(3,4)) == "2.a.b.3.4");
  
  // Test read / write double samples to tile

  {
    Tile t1, t2;
    t1.header.magic = Tile::MAGIC;
    t1.header.version = 0x00010000;
    t1.double_samples.push_back(DataSample<double>(1.11, 333.333));
    t1.double_samples.push_back(DataSample<double>(2.22, 555.555));
    
    tassert(!ch.read_tile(TileIndex(10,20), t2));
    tassert(!ch.has_tile(TileIndex(10,20)));
    tassert(!ch.read_tile(TileIndex(10,21), t2));
    tassert(!ch.has_tile(TileIndex(10,21)));
    ch.write_tile(TileIndex(10,20), t1);
    tassert(ch.read_tile(TileIndex(10,20), t2));
    tassert(ch.has_tile(TileIndex(10,20)));
    
    tassert_equals(t2.header.magic, Tile::MAGIC);
    tassert_equals(t2.header.version, 0x00010000);
    tassert_equals(t2.double_samples.size(), 2);
    tassert_equals(t2.string_samples.size(), 0);
    tassert(t2.double_samples[0] == DataSample<double>(1.11, 333.333));
    tassert(t2.double_samples[1] == DataSample<double>(2.22, 555.555));
    
    ch.delete_tile(TileIndex(10,20));
    tassert(!ch.read_tile(TileIndex(10,20), t2));
    tassert(!ch.has_tile(TileIndex(10,20)));
    tassert(!ch.read_tile(TileIndex(10,21), t2));
    tassert(!ch.has_tile(TileIndex(10,21)));
  }

  // Test read / write string samples to a tile

  {
    Tile t1, t2;
    t1.header.magic = Tile::MAGIC;
    t1.header.version = 0x00010000;
    t1.string_samples.push_back(DataSample<std::string>(1.11, "abc"));
    t1.string_samples.push_back(DataSample<std::string>(2.22, "defghi"));
    
    tassert(!ch.read_tile(TileIndex(10,20), t2));
    tassert(!ch.has_tile(TileIndex(10,20)));
    tassert(!ch.read_tile(TileIndex(10,21), t2));
    tassert(!ch.has_tile(TileIndex(10,21)));
    ch.write_tile(TileIndex(10,20), t1);
    tassert(ch.read_tile(TileIndex(10,20), t2));
    tassert(ch.has_tile(TileIndex(10,20)));
    
    tassert_equals(t2.header.magic, Tile::MAGIC);
    tassert_equals(t2.header.version, 0x00010000);
    tassert_equals(t2.double_samples.size(), 0);
    tassert_equals(t2.string_samples.size(), 2);
    tassert(t2.string_samples[0] == DataSample<std::string>(1.11, "abc"));
    tassert(t2.string_samples[1] == DataSample<std::string>(2.22, "defghi"));
    
    ch.delete_tile(TileIndex(10,20));
    tassert(!ch.read_tile(TileIndex(10,20), t2));
    tassert(!ch.has_tile(TileIndex(10,20)));
    tassert(!ch.read_tile(TileIndex(10,21), t2));
    tassert(!ch.has_tile(TileIndex(10,21)));
  }



  Channel ch2(kvs, 2, "a.c");

  test_samples_single_tile(ch2, 1309780800.0, 1);
  test_samples_single_tile(ch2, 1309780800.0, 10);
  test_samples_single_tile(ch2, 1309780800.0, 100);
  test_samples_single_tile(ch2, 1309780800.0, 1000);
  test_samples_single_tile(ch2, 1309780800.0, 10000);

  test_samples_multiple_tiles(ch2, 1309780800.0, 100000);
  test_samples_multiple_tiles(ch2, 1309780800.0, 100000);
  test_samples_multiple_tiles(ch2, 1309780800.0, 110000);
  //test_samples_multiple_tiles(ch2, 1309780800.0, 1000000);
  //test_samples_multiple_tiles(ch2, 1309780800.0, 3000000);
  
  test_subsampling(kvs);
  test_subsampling_stddev(kvs);
  test_subsampling_string(kvs);

  test_subsampling_threads();

  fprintf(stderr, "Tests succeeded\n");
  return 0;
};


