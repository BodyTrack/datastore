// System
#include <assert.h>
#include <string.h>

// Local
#include "utils.h"
// Self
#include "Tile.h"

void Tile::to_binary(std::string &dest) const {
  boost::uint32_t double_samples_len = sizeof(DataSample<double>) * double_samples.size();
  dest.resize(sizeof(header) + sizeof(double_samples_len) + double_samples_len);
  char *destptr = &dest[0];
  memcpy(destptr, (void*)&header, sizeof(header)); destptr += sizeof(header);
  memcpy(destptr, (void*)&double_samples_len, sizeof(double_samples_len)); destptr += sizeof(double_samples_len);
  
  for (unsigned i = 0; i < double_samples.size(); i++) {
    memcpy(destptr, (void*)&double_samples[i], sizeof(DataSample<double>)); destptr += sizeof(DataSample<double>);
  }
}

size_t Tile::binary_length() const {
  boost::uint32_t double_samples_len = sizeof(DataSample<double>) * double_samples.size();
  return sizeof(header) + sizeof(double_samples_len) + double_samples_len;
}

void Tile::from_binary(const std::string &src) {
  boost::uint32_t double_samples_len;
  assert(src.length() >= sizeof(header) + sizeof(double_samples_len));
  const char *srcptr = &src[0];
  memcpy((void*)&header, srcptr, sizeof(header)); srcptr += sizeof(header);
  memcpy((void*)&double_samples_len, srcptr, sizeof(double_samples_len)); srcptr += sizeof(double_samples_len);
  assert(src.length() >= sizeof(header) + sizeof(double_samples_len) + double_samples_len);
  assert(double_samples_len % sizeof(DataSample<double>) == 0);

  double_samples.resize(double_samples_len / sizeof(DataSample<double>));
  for (unsigned i = 0; i < double_samples.size(); i++) {
    memcpy((void*)&double_samples[i], srcptr, sizeof(DataSample<double>)); srcptr += sizeof(DataSample<double>);
  }
}

void Tile::insert_double_samples(const DataSample<double> *begin, const DataSample<double> *end) {
  std::vector<DataSample<double> > tmp(double_samples.size() + (end-begin));
  DataSample<double> *begin2 = &double_samples[0];
  DataSample<double> *end2 = &double_samples[double_samples.size()];
  DataSample<double> *out = &tmp[0];

  // Merge
  while (!(begin == end && begin2 == end2)) {
    if (begin == end) *out++ = *begin2++;
    else if (begin2 == end2 || begin->time < begin2->time) *out++ = *begin++;
    else if (begin2->time < begin->time) *out++ = *begin2++;
    else {
      // Duplicate entry
      *out++ = *begin++;
      *begin2++;
    }
  }

  double_samples.resize(out - &tmp[0]);
  std::copy(&tmp[0], out, &double_samples[0]);
}

