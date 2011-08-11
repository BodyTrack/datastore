// System
#include <assert.h>
#include <string.h>
#include <limits>

// Local
#include "utils.h"
// Self
#include "Tile.h"

void decompose_string_samples(const std::vector<DataSample<std::string> > &samples,
                              std::vector<DataSample<boost::uint32_t> > &indexes,
                              std::string &text)
{
  text = "";
  indexes.resize(samples.size());
  boost::uint32_t index = 0;
  for (unsigned i = 0; i < samples.size(); i++) {
    indexes[i].time = samples[i].time;
    indexes[i].value = index;
    index += samples[i].value.length();
  }
  text = "";
  text.reserve(index);
  for (unsigned i = 0; i < samples.size(); i++) {
    text += samples[i].value;
  }
};

void compose_string_samples(const std::vector<DataSample<boost::uint32_t> > &indexes,
                            const std::string &text,
                            std::vector<DataSample<std::string> > &samples)
{
  samples.resize(indexes.size());
  // Add another sample at the end to get the index beyond the last char of text
  for (unsigned i = 0; i < samples.size(); i++) {
    samples[i].time = indexes[i].time;
    boost::uint32_t begin = indexes[i].value;
    boost::uint32_t end = i+1 < samples.size() ? indexes[i+1].value : text.length();
    samples[i].value = text.substr(begin, end-begin);
  }
};

class BinaryWriter {
private:
  unsigned char *m_ptr, *m_end;
public:
  BinaryWriter(unsigned char *start, unsigned char *end) :
    m_ptr(start), m_end(end) {}

  // vector<DataSample<string> >
  void write(const std::vector<DataSample<std::string> > &samples) {
    std::vector<DataSample<boost::uint32_t> > indexes;
    std::string text;
    decompose_string_samples(samples, indexes, text);
    write(indexes);
    write(text);
  }
  static size_t write_length(const std::vector<DataSample<std::string> > &samples) {
    // Trading shorter code for less efficiency.  If we need to speed
    // this up we can calculate the length without computing the value
    std::vector<DataSample<boost::uint32_t> > indexes;
    std::string text;
    decompose_string_samples(samples, indexes, text);
    return BinaryWriter::write_length(indexes) + BinaryWriter::write_length(text);
  }

  // string
  void write(const std::string &text) {
    // align 4-byte
    const unsigned char *endptr = m_ptr + write_string_length(text.length());
    write((boost::uint32_t) text.length());
    write_bytes(text.c_str(), text.length());
    write_zeros(endptr - m_ptr);
  }
  static size_t write_length(const std::string &text) {
    return write_string_length(text.length());
  }
  static size_t write_string_length(size_t len) {
    return write_length((boost::uint32_t)0) + (len + 3) / 4 * 4;  // Pad to nearest 4-byte boundary
  }

  // vector<T>
  template <class T>
  void write(const std::vector<T> &v) {
    write((boost::uint32_t) (v.size() * sizeof(T)));
    for (unsigned i = 0; i < v.size(); i++) write(v[i]);
  }
  template <class T>
  static size_t write_length(const std::vector<T> &v) {
    size_t ret = write_length((boost::uint32_t)0);
    for (unsigned i = 0; i < v.size(); i++) ret += write_length(v[i]);
    return ret;
  }

  // T
  template <class T>
  void write(const T &v) {
    write_bytes((void*)&v, write_length(v));
  }
  template <class T>
  static size_t write_length(const T &v) {
    return sizeof(v);
  }

  void write_bytes(const void *p, size_t len) {
    assert(m_ptr + len <= m_end);
    memcpy(m_ptr, p, len);
    m_ptr += len;
  }

  void write_zeros(size_t len) {
    assert(m_ptr + len <= m_end);
    memset(m_ptr, 0, len);
    m_ptr += len;
  }
};

class BinaryReader {
private:
  const unsigned char *m_ptr, *m_end;
public:
  BinaryReader(const unsigned char *start, const unsigned char *end) :
    m_ptr(start), m_end(end) {}
  bool eof() const { return m_ptr >= m_end; }

  // vector<DataSample<string> >
  void read(std::vector<DataSample<std::string> > &samples) {
    std::vector<DataSample<boost::uint32_t> > indexes;
    std::string text;
    read(indexes);
    read(text);
    compose_string_samples(indexes, text, samples);
  }

  // string
  void read(std::string &text) {
    // align 4-byte
    const unsigned char *endptr = m_ptr + BinaryWriter::write_string_length(text.length());
    boost::uint32_t len;
    read(len);
    text.resize(len);
    read_bytes(&text[0], len);
    skip_bytes(endptr - m_ptr);
  }

  // vector<T>
  template <class T>
  void read(std::vector<T> &v) {
    boost::uint32_t len;
    read(len);
    assert(len % sizeof(T) == 0);
    v.resize(len / sizeof(T));
    for (unsigned i = 0; i < v.size(); i++) read(v[i]);
  }
  

  // T
  template <class T>
  void read(T &v) {
    assert(m_ptr + sizeof(v) <= m_end);
    memcpy((void*)&v, m_ptr, sizeof(v));
    m_ptr += sizeof(v);
  }

  void read_bytes(void *dest, size_t len) {
    assert(m_ptr + len <= m_end);
    memcpy(dest, m_ptr, len);
    m_ptr += len;
  }

  void skip_bytes(size_t len) {
    assert(m_ptr + len <= m_end);
    m_ptr += len;
  }
};

void Tile::to_binary(std::string &dest) const {
  dest.resize(binary_length());

  BinaryWriter writer((unsigned char*)&dest[0], (unsigned char*)&dest[dest.length()]);

  writer.write(header);
  writer.write(double_samples);
  writer.write(string_samples);
}

size_t Tile::binary_length() const {
  return BinaryWriter::write_length(header)
    + BinaryWriter::write_length(double_samples)
    + BinaryWriter::write_length(string_samples);
}

void Tile::from_binary(const std::string &src) {
  BinaryReader reader((const unsigned char*)&src[0], (const unsigned char*)&src[src.length()]);

  reader.read(header);
  reader.read(double_samples);
  if (!reader.eof()) reader.read(string_samples);
}

template <class T>
void insert_samples_helper(const DataSample<T> *begin, const DataSample<T> *end, std::vector<DataSample<T> > &dest)
{
  // Assert sortedness
  for (unsigned i = 1; i < end-begin; i++) assert(begin[i-1].time <= begin[i].time);
  
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
      *begin2++;
    }
  }

  dest.resize(out - &tmp[0]);
  std::copy(&tmp[0], out, &dest[0]);
}
  
void Tile::insert_samples(const DataSample<double> *begin, const DataSample<double> *end) {
  insert_samples_helper(begin, end, double_samples);
}

void Tile::insert_samples(const DataSample<std::string> *begin, const DataSample<std::string> *end) {
  insert_samples_helper(begin, end, string_samples);
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
  return string_printf("[ndoubles %zd weight %f; nstrings %zd weight %f]", 
		       double_samples.size(), doubles_weight,
		       string_samples.size(), strings_weight);
}
