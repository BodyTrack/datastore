#ifndef INCLUDE_LOG_H
#define INCLUDE_LOG_H

// C++
#include <string>

void log_f(const char *fmt, ...);
void set_log_prefix(const std::string &prefix);
std::string recorded_log();

#endif
