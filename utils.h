#ifndef UTILS_INCLUDE_H
#define UTILS_INCLUDE_H

// C++
#include <string>
#include <stdarg.h>

std::string string_vprintf(const char *fmt, va_list args);
std::string string_printf(const char *fmt, ...);
bool filename_exists(const std::string &filename);
#define tassert(expr) ((expr)?0:(printf("%s:%u: failed tassert '%s'\n", __FILE__, __LINE__, #expr), abort(), 0))
unsigned long long microtime();
unsigned long long millitime();
double doubletime();

#endif

