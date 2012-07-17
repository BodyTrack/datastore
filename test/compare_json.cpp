#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>

#include <json/json.h>

Json::Value read_json_from_file(const std::string &filename)
{
  bool ret;
  Json::Reader reader;
  Json::Value json;
  if (filename == "-") {
    ret= reader.parse(std::cin, json);
  } else {
    std::ifstream instream(filename.c_str());
    ret= reader.parse(instream, json);
  }
  if (!ret) {
    fprintf(stderr, "Failed to parse JSON from '%s'\n", filename.c_str());
    exit(1);
  }
  return json;
}

void usage()
{
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "compare_json [--ignore-log] file1.json file2.json\n");
  exit(1);
}

int main(int argc, char **argv)
{
  std::vector<std::string> files;

  bool ignore_log = false;

  for (int i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "--ignore-log")) {
      ignore_log = true;
    } else if (strlen(argv[i]) > 1 && argv[i][0] == '-') {
      usage();
    } else {
      files.push_back(argv[i]);
    }
  }
  if (files.size() != 2) usage();
  Json::Value a = read_json_from_file(files[0]);
  Json::Value b = read_json_from_file(files[1]);
  if (ignore_log) {
    if (a.isObject()) a.removeMember("log");
    if (b.isObject()) b.removeMember("log");
  }
  if (a == b) {
    fprintf(stderr, "JSON same\n");
    return 0;
  } else {
    fprintf(stderr, "JSON differs\n");
    return 1;
  }
}
