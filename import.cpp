// C++
#include <iostream>
#include <string>
#include <vector>

// Local
#include "Binrec.h"
#include "FilesystemKVS.h"
#include "ImportBT.h"
#include "utils.h"

void usage()
{
  std::cerr << "Usage:\n";
  std::cerr << "import store.kvs uid file1.bt ... fileN.bt\n";
  std::cerr << "Exiting...\n";
  exit(1);
}

int main(int argc, char **argv)
{
  char **argptr = argv+1;

  if (!*argptr) usage();
  std::string storename = *argptr++;

  if (!*argptr) usage();
  int uid = atoi(*argptr++);
  if (uid <= 0) usage();
  
  std::vector<std::string> files(argptr, argv+argc);
  if (!files.size()) usage();

  for (unsigned i = 0; i < files.size(); i++) {
    if (!filename_exists(files[i])) {
      fprintf(stderr, "File %s doesn't exist\n", files[i].c_str());
      usage();
    }
  }

  fprintf(stderr, "Opening store %s\n", storename.c_str());
  FilesystemKVS store(storename.c_str());
  for (unsigned i = 0; i < files.size(); i++) {
    fprintf(stderr, "Importing %s into UID %d\n", files[i].c_str(), uid);
    try {
      import_bt_file(store, files[i], uid);
    } catch (const ParseError &p) {
      fprintf(stderr, "Error reading %s:\n%s\n", files[i].c_str(), p.what());
      fprintf(stderr, "Skipping %s:  no data imported.\n", files[i].c_str());
    }
  }
  fprintf(stderr, "Done\n");
  return 0;
}
