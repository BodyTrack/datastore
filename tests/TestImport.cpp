#include "ImportBT.h"
#include "FilesystemKVS.h"

int main(int argc, char **argv)
{
  //int tile_size = 1024*1024*6/4;
  system("rm -rf import_test.kvs");
  system("mkdir import_test.kvs");
  FilesystemKVS store("import_test.kvs");

  int uid = 1;
//  import_bt_file(store, "../bt/front-porch-test/4dbc84fc.bt", uid);
//  import_bt_file(store, "../bt/ar_basestation/4cfcff7d.bt", uid);
  import_bt_file(store, "../bt/front-porch-test/4dbc7356.bt", uid);
  return 0;
}
