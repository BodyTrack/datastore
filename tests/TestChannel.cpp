// C
#include <assert.h>
#include <stdlib.h>

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
    assert(ch.read_tile(TileIndex::nonnegative_all(), tile));
    fprintf(stderr, "  read %zd samples in %lld msec\n", num_samples, millitime() - begin);
  }

  assert(tile.double_samples == data);
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
  assert(data == read_data);
  fprintf(stderr, "test_samples_multiple_tiles(%zd) succeeded\n", num_samples);
}

int main(int argc, char **argv) {
  int tile_size = 1024*1024*6/4;
  system("rm -rf channelstore_test.kvs");
  system("mkdir channelstore_test.kvs");
  FilesystemKVS kvs("channelstore_test.kvs");
  kvs.set_verbosity(0);

  Channel ch(kvs, 2, "a.b", tile_size);
  {
    Channel::Locker locker(ch);
  }

  // Test ChannelInfo
  
  ChannelInfo info;
  assert(!ch.read_info(info));
  info.magic = ChannelInfo::MAGIC;
  info.version = 0x00010000;
  info.min_time = 1;
  info.max_time = 2;
  info.nonnegative_root_tile_index = TileIndex::nonnegative_all();
  info.negative_root_tile_index = TileIndex::negative_all();
  ch.write_info(info);
  ChannelInfo info2;
  assert(ch.read_info(info2));
  assert(info.magic == ChannelInfo::MAGIC);
  assert(info.version == 0x00010000);
  assert(info.min_time == 1);
  assert(info.max_time == 2);
  assert(info.nonnegative_root_tile_index == info.nonnegative_root_tile_index);
  assert(info.negative_root_tile_index == info.negative_root_tile_index);

  // Test paths
  assert(ch.tile_key(TileIndex(3,4)) == "2.a.b.3.4");
  
  // Test read / write tile

  Tile t1, t2;
  t1.header.magic = Tile::MAGIC;
  t1.header.version = 0x00010000;
  t1.double_samples.push_back(DataSample<double>(1.11, 333.333));
  t1.double_samples.push_back(DataSample<double>(2.22, 555.555));
  
  assert(!ch.read_tile(TileIndex(10,20), t2));
  assert(!ch.has_tile(TileIndex(10,20)));
  assert(!ch.read_tile(TileIndex(10,21), t2));
  assert(!ch.has_tile(TileIndex(10,21)));
  ch.write_tile(TileIndex(10,20), t1);
  assert(ch.read_tile(TileIndex(10,20), t2));
  assert(ch.has_tile(TileIndex(10,20)));
  
  assert(t2.header.magic == Tile::MAGIC);
  assert(t2.header.version == 0x00010000);
  assert(t2.double_samples.size() == 2);
  assert(t2.double_samples[0] == DataSample<double>(1.11, 333.333));
  assert(t2.double_samples[1] == DataSample<double>(2.22, 555.555));

  ch.delete_tile(TileIndex(10,20));
  assert(!ch.read_tile(TileIndex(10,20), t2));
  assert(!ch.has_tile(TileIndex(10,20)));
  assert(!ch.read_tile(TileIndex(10,21), t2));
  assert(!ch.has_tile(TileIndex(10,21)));

  Channel ch2(kvs, 2, "a.c", tile_size);

  test_samples_single_tile(ch2, 1309780800.0, 1);
  test_samples_single_tile(ch2, 1309780800.0, 10);
  test_samples_single_tile(ch2, 1309780800.0, 100);
  test_samples_single_tile(ch2, 1309780800.0, 1000);
  test_samples_single_tile(ch2, 1309780800.0, 10000);

  test_samples_multiple_tiles(ch2, 1309780800.0, 100000);
  test_samples_multiple_tiles(ch2, 1309780800.0, 100000);
  test_samples_multiple_tiles(ch2, 1309780800.0, 110000);
  test_samples_multiple_tiles(ch2, 1309780800.0, 1000000);
  //test_samples_multiple_tiles(ch2, 1309780800.0, 3000000);
  
  fprintf(stderr, "Tests succeeded\n");
  return 0;
};


