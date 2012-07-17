#ifndef INCLUDE_BINARYIO_H
#define INCLUDE_BINARYIO_H

// C++
#include <cstring>
#include <string>
#include <vector>

// Local
#include "DataSample.h"
#include "utils.h"
#include "sizes.h"

class BinaryWriter {
private:
  unsigned char *m_ptr, *m_end;
public:
  BinaryWriter(unsigned char *start, unsigned char *end);
  bool eof() const;

  // vector<DataSample<string> >
  void write(const std::vector<DataSample<std::string> > &samples);
  static size_t write_length(const std::vector<DataSample<std::string> > &samples);

  // string
  void write(const std::string &text);
  static size_t write_length(const std::string &text);
  static size_t write_string_length(size_t len);

  // vector<T>
  template <class T>
  void write(const std::vector<T> &v) {
    write((uint32) (v.size() * sizeof(T)));
    for (unsigned i = 0; i < v.size(); i++) write(v[i]);
  }
  template <class T>
  static size_t write_length(const std::vector<T> &v) {
    size_t ret = write_length((uint32)0);
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

  ////
  void write_bytes(const void *p, size_t len);
  void write_zeros(size_t len);
};

class BinaryReader {
private:
  const unsigned char *m_ptr, *m_end;
public:
  BinaryReader(const unsigned char *start, const unsigned char *end);
  bool eof() const;

  // vector<DataSample<string> >
  void read(std::vector<DataSample<std::string> > &samples);

  // string
  void read(std::string &text);

  // vector<T>
  template <class T>
  void read(std::vector<T> &v) {
    uint32 len;
    read(len);
    tassert(len % sizeof(T) == 0);
    v.resize(len / sizeof(T));
    for (unsigned i = 0; i < v.size(); i++) read(v[i]);
  }
  

  // T
  template <class T>
  void read(T &v) {
    tassert(m_ptr + sizeof(v) <= m_end);
    memcpy((void*)&v, m_ptr, sizeof(v));
    m_ptr += sizeof(v);
  }

  ////

  void read_bytes(void *dest, size_t len);
  void skip_bytes(size_t len);
};

#endif
