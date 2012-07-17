// C++
#include <vector>

// C
#include <stdio.h>

// Local
#include "utils.h"

// Self
#include "Log.h"

static std::string log_prefix;

static std::vector<std::string> log_record;

bool record_log = true;


void log_f(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  std::string msg = string_vprintf(fmt, args);
  va_end(args);
  msg = string_printf("%.6f %s%s\n", doubletime(), log_prefix.c_str(), msg.c_str());
  fprintf(stderr, "%s", msg.c_str());
  if (record_log) {
    // Add mutex here for multithreading
    log_record.push_back(msg);
  }
}

void set_log_prefix(const std::string &prefix) {
  log_prefix = prefix;
}

std::string recorded_log() {
  // Add mutex here for multithreading
  size_t len = 0;
  for (unsigned i = 0; i < log_record.size(); i++) len += log_record[i].length();
  std::string ret;
  ret.reserve(len);
  for (unsigned i = 0; i < log_record.size(); i++) ret += log_record[i];
  return ret;
}
