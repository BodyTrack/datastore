// C
#include <assert.h>
#include <stdlib.h>

// Local
#include "FilesystemKVS.h"

// Module to test
#include "ChannelStore.h"

int main(int argc, char **argv) {
  system("rm -rf channelstore_test.kvs");
  system("mkdir channelstore_test.kvs");
  FilesystemKVS kvs("channelstore_test.kvs");
  ChannelStore cs(kvs);

  boost::shared_ptr<Channel> ch = cs.find_channel("a.b");
  assert(ch.get() == NULL);

  ch = cs.find_or_create_channel("a.b");
  assert(ch.get() != NULL);
  assert(cs.find_channel("a.b") == ch);
  assert(cs.find_or_create_channel("a.b") == ch);
  Channel *ch_badptr = ch.get();
  ch.reset();
  ch = cs.find_channel("a.b");
  assert(ch.get() != ch_badptr);
  fprintf(stderr, "Tests succeeded\n");
  return 0;
};


