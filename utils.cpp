// C
#include <sys/stat.h>
#include <sys/time.h>
#include <stdio.h>

// C++
#include <iostream>

// Self
#include "utils.h"

std::string string_vprintf(const char *fmt, va_list args)
{
  std::string ret;
  int size= 1000;
  while (1) {
    ret.resize(size);
#if defined(_WIN32)
    int nwritten= _vsnprintf(&ret[0], size-1, fmt, args);
#else
    int nwritten= vsnprintf(&ret[0], size-1, fmt, args);
#endif
    // Some c libraries return -1 for overflow, some return
    // a number larger than size-1
    if (nwritten >= 0 && nwritten < size-2) {
      if (ret[nwritten-1] == 0) nwritten--;
      ret.resize(nwritten);
      return ret;
    }
    size *= 2;
  }
}

std::string string_printf(const char *fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  std::string ret= string_vprintf(fmt, args);
  va_end(args);
  return ret;
}

#ifdef _WIN32
#define stat _stat
#define fstat _fstat
#endif

bool
filename_exists(const std::string &filename)
{
  struct stat statbuf;
  int ret= stat(filename.c_str(), &statbuf);
  return (ret == 0);
}

unsigned long long microtime() {
#ifdef WIN32
  return ((long long)GetTickCount() * 1000);
#else
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return ((long long) tv.tv_sec * (long long) 1000000 +
          (long long) tv.tv_usec);
#endif
}

unsigned long long millitime() { return microtime() / 1000; }

double doubletime() { return microtime() / 1000000.0; }

std::string rtrim(const std::string &x) {
  return x.substr(0, x.find_last_not_of(" \r\n\t\f")+1);
}

