// C
#include <stdio.h>

// Local
#include "utils.h"

// Self
#include "Log.h"

void log_f(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  std::string msg = string_vprintf(fmt, args);
  va_end(args);
  fprintf(stderr, "%.6f %s\n", doubletime(), msg.c_str());
}
