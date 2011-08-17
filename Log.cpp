// C
#include <stdio.h>

// Local
#include "utils.h"

// Self
#include "Log.h"

static std::string log_prefix;

void log_f(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  std::string msg = string_vprintf(fmt, args);
  va_end(args);
  fprintf(stderr, "%.6f %s%s\n", 
	  doubletime(), log_prefix.c_str(), msg.c_str());
}

void set_log_prefix(const std::string &prefix) {
  log_prefix = prefix;
}
