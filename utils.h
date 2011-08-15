#ifndef UTILS_INCLUDE_H
#define UTILS_INCLUDE_H

// C++
#include <string>

// C
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

std::string string_vprintf(const char *fmt, va_list args);
std::string string_printf(const char *fmt, ...);
bool filename_exists(const std::string &filename);
#define tassert(expr) ((void)((expr)?0:(printf("%s:%u: failed tassert '%s'\n", __FILE__, __LINE__, #expr), abort(), 0)))
#define tassert_approx_equals(a,b) ((void)(fabs((a)-(b))<1e-5?0:(printf("%s:%u: tassert_approx_equals %g != %g\n", __FILE__, __LINE__, (double)(a), (double)(b)), abort(), 0)))
#define tassert_equals(a,b) ((void)((a)==(b)?0:(printf("%s:%u: tassert_approx_equals %g != %g\n", __FILE__, __LINE__, (double)(a), (double)(b)), abort(), 0)))
unsigned long long microtime();
unsigned long long millitime();
double doubletime();
std::string rtrim(const std::string &x);

std::string filename_sans_directory(const std::string &filename);
std::string filename_directory(const std::string &filename);
std::string filename_sans_suffix(const std::string &filename);
std::string filename_suffix(const std::string &filename);

#endif

