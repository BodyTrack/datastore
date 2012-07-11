// System
#include <assert.h>
#include <string.h>
#include <limits>

// Local
#include "BinaryIO.h"
#include "utils.h"

// Self
#include "Tile.h"

void Tile::to_binary(std::string &dest) const {
  dest.resize(binary_length());

  BinaryWriter writer((unsigned char*)&dest[0], (unsigned char*)&dest[dest.length()]);

  writer.write(header);
  writer.write(double_samples);
  writer.write(string_samples);
  writer.write(ranges);
}

size_t Tile::binary_length() const {
  return BinaryWriter::write_length(header)
    + BinaryWriter::write_length(double_samples)
    + BinaryWriter::write_length(string_samples)
    + BinaryWriter::write_length(ranges);
}

void Tile::from_binary(const std::string &src) {
  BinaryReader reader((const unsigned char*)&src[0], (const unsigned char*)&src[src.length()]);

  reader.read(header);
  reader.read(double_samples);

  if (!reader.eof()) {
    reader.read(string_samples);
  } else {
    string_samples.clear();
  }

  if (!reader.eof()) {
    reader.read(ranges);
  } else {
    ranges.clear();
  }
}

template <class T>
void insert_samples_helper(const DataSample<T> *begin, const DataSample<T> *end, std::vector<DataSample<T> > &dest)
{
  // Assert sortedness
  for (int i = 1; i < end-begin; i++) assert(begin[i-1].time <= begin[i].time);
  
  std::vector<DataSample<T> > tmp(dest.size() + (end-begin));
  DataSample<T> *begin2 = &dest[0];
  DataSample<T> *end2 = &dest[dest.size()];
  DataSample<T> *out = &tmp[0];

  // Merge
  while (!(begin == end && begin2 == end2)) {
    if (begin == end) *out++ = *begin2++;
    else if (begin2 == end2 || begin->time < begin2->time) *out++ = *begin++;
    else if (begin2->time < begin->time) *out++ = *begin2++;
    else {
      // Duplicate entry.  Overwrite
      // TODO: do we always want to overwrite?
      *out++ = *begin++;
      begin2++;
    }
  }

  dest.resize(out - &tmp[0]);
  std::copy(&tmp[0], out, &dest[0]);
}
  
void Tile::insert_samples(const DataSample<double> *begin, const DataSample<double> *end) {
  insert_samples_helper(begin, end, double_samples);
  for (const DataSample<double> *s = begin; s < end; s++) {
    ranges.times.add(s->time);
    ranges.double_samples.add(s->value);
  }
}

void Tile::insert_samples(const DataSample<std::string> *begin, const DataSample<std::string> *end) {
  insert_samples_helper(begin, end, string_samples);
  for (const DataSample<std::string> *s = begin; s < end; s++) {
    ranges.times.add(s->time);
  }
}

double Tile::first_sample_time() const
{
  double ret = std::numeric_limits<double>::max();
  if (double_samples.size()) ret = std::min(ret, double_samples[0].time);
  if (string_samples.size()) ret = std::min(ret, string_samples[0].time);
  return ret;
}

double Tile::last_sample_time() const
{
  double ret = -std::numeric_limits<double>::max();
  if (double_samples.size()) ret = std::max(ret, double_samples.back().time);
  if (string_samples.size()) ret = std::max(ret, string_samples.back().time);
  return ret;
}

std::string Tile::summary() const
{
  double doubles_weight = 0, strings_weight = 0;
  for (unsigned i = 0; i < double_samples.size(); i++) doubles_weight += double_samples[i].weight;
  for (unsigned i = 0; i < string_samples.size(); i++) strings_weight += string_samples[i].weight;
  return string_printf("[ndoubles %zd weight %f; nstrings %zd weight %f; ranges times=%s doubles=%s]", 
		       double_samples.size(), doubles_weight,
		       string_samples.size(), strings_weight,
                       ranges.times.to_string("%.6f").c_str(), ranges.double_samples.to_string().c_str());
}
