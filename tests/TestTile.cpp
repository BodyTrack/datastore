// System
#include <assert.h>
#include <stdio.h>

// Module to test
#include "Tile.h"

int main(int argc, char **argv)
{
  Tile t1, t2;
  std::string binary;
  t1.header.magic = Tile::MAGIC;
  t1.header.version = 0x00010000;
  t1.double_samples.push_back(DataSample<double>(1.11, 333.333));
  t1.double_samples.push_back(DataSample<double>(2.22, 555.555));
  
  t1.to_binary(binary);
  assert(binary.length() == 4 + 4 + 4 + 24*2 + 4 + 4);
  t2.from_binary(binary);
  assert(t2.header.magic == Tile::MAGIC);
  assert(t2.header.version == 0x00010000);
  assert(t2.double_samples.size() == 2);
  assert(t2.double_samples[0] == DataSample<double>(1.11, 333.333));
  assert(t2.double_samples[1] == DataSample<double>(2.22, 555.555));

  // Done
  fprintf(stderr, "Tests succeeded\n");
  return 0;
}
