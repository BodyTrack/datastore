// C++
#include <iostream>
#include <string>
#include <vector>

// C
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>

// Local
#include "Binrec.h"
#include "FilesystemKVS.h"
#include "ImportBT.h"
#include "ImportJson.h"
#include "utils.h"

void usage()
{
  std::cerr << "Usage:\n";
  std::cerr << "import store.kvs uid device-nickname file1.bt ... fileN.bt\n";
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
  
  if (!*argptr) usage();
  std::string dev_nickname = *argptr++;

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
    std::string filename = files[i];
    fprintf(stderr, "Importing %s into UID %d\n", filename.c_str(), uid);
    
    ParseInfo info;
    if (!strcasecmp(filename_suffix(filename).c_str(), "bt")) {
      import_bt_file(store, files[i], uid, dev_nickname, info);
    } else if (!strcasecmp(filename_suffix(filename).c_str(), "json")) {
      import_json_file(store, files[i], uid, dev_nickname, info);
    } else {
      printf("{\"failure\":\"Unrecognized filename suffix %s\"}\n", filename_suffix(filename).c_str());
      continue;
    }
    printf("{");
    printf("\"successful_records\":%d", info.good_records);
    printf(",");
    printf("\"failed_records\":%d", info.bad_records);
    if (!isinf(info.min_time)) {
      printf(",");
      printf("\"min_time\":%.9f", info.min_time);
    }
    if (!isinf(info.max_time)) {
      printf(",");
      printf("\"max_time\":%.9f", info.max_time);
    }
    printf(",");
    printf("\"channel_specs\":%s", rtrim(Json::FastWriter().write( info.channel_specs )).c_str());
    printf("}\n");
  }
  fprintf(stderr, "Done\n");
  return 0;
}
