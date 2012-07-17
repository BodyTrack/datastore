#include <assert.h>

#include "BinaryIO.h"

void decompose_string_samples(const std::vector<DataSample<std::string> > &samples,
                              std::vector<DataSample<uint32> > &indexes,
                              std::string &text)
{
  text = "";
  indexes.resize(samples.size());
  uint32 index = 0;
  for (unsigned i = 0; i < samples.size(); i++) {
    indexes[i].time = samples[i].time;
    indexes[i].value = index;
    indexes[i].weight = samples[i].weight;
    indexes[i].stddev = samples[i].stddev;
    index += samples[i].value.length();
  }
  text = "";
  text.reserve(index);
  for (unsigned i = 0; i < samples.size(); i++) {
    text += samples[i].value;
  }
};

void compose_string_samples(const std::vector<DataSample<uint32> > &indexes,
                            const std::string &text,
                            std::vector<DataSample<std::string> > &samples)
{
  samples.resize(indexes.size());
  // Add another sample at the end to get the index beyond the last char of text
  for (unsigned i = 0; i < samples.size(); i++) {
    samples[i].time = indexes[i].time;
    uint32 begin = indexes[i].value;
    uint32 end = i+1 < samples.size() ? indexes[i+1].value : text.length();
    samples[i].value = text.substr(begin, end-begin);
    samples[i].weight = indexes[i].weight;
    samples[i].stddev = indexes[i].stddev;
  }
};

BinaryWriter::BinaryWriter(unsigned char *start, unsigned char *end) :
  m_ptr(start), m_end(end) {}

bool BinaryWriter::eof() const { return m_ptr >= m_end; }

// vector<DataSample<string> >
void BinaryWriter::write(const std::vector<DataSample<std::string> > &samples) {
  std::vector<DataSample<uint32> > indexes;
  std::string text;
  decompose_string_samples(samples, indexes, text);
  write(indexes);
  write(text);
}

size_t BinaryWriter::write_length(const std::vector<DataSample<std::string> > &samples) {
  // Trading shorter code for less efficiency.  If we need to speed
  // this up we can calculate the length without computing the value
  std::vector<DataSample<uint32> > indexes;
  std::string text;
  decompose_string_samples(samples, indexes, text);
  return BinaryWriter::write_length(indexes) + BinaryWriter::write_length(text);
}


// string
void BinaryWriter::write(const std::string &text) {
  // align 4-byte
  const unsigned char *endptr = m_ptr + write_string_length(text.length());
  write((uint32) text.length());
  write_bytes(text.c_str(), text.length());
  write_zeros(endptr - m_ptr);
}

size_t BinaryWriter::write_length(const std::string &text) {
  return write_string_length(text.length());
}


size_t BinaryWriter::write_string_length(size_t len) {
  return write_length((uint32)0) + (len + 3) / 4 * 4;  // Pad to nearest 4-byte boundary
}

////

void BinaryWriter::write_bytes(const void *p, size_t len) {
  tassert(m_ptr + len <= m_end);
  memcpy(m_ptr, p, len);
  m_ptr += len;
}

void BinaryWriter::write_zeros(size_t len) {
  assert(m_ptr + len <= m_end);
  memset(m_ptr, 0, len);
  m_ptr += len;
}

///////////////////////////////

BinaryReader::BinaryReader(const unsigned char *start, const unsigned char *end) :
    m_ptr(start), m_end(end) {}

bool BinaryReader::eof() const { return m_ptr >= m_end; }

// vector<DataSample<string> >
void BinaryReader::read(std::vector<DataSample<std::string> > &samples) {
  std::vector<DataSample<uint32> > indexes;
  std::string text;
  read(indexes);
  read(text);
  compose_string_samples(indexes, text, samples);
}

// string
void BinaryReader::read(std::string &text) {
  // align 4-byte
  const unsigned char *startptr = m_ptr;
  uint32 len;
  read(len);
  const unsigned char *endptr = startptr + BinaryWriter::write_string_length(len);
  text.resize(len);
  read_bytes(&text[0], len);
  skip_bytes(endptr - m_ptr);
}

////

void BinaryReader::read_bytes(void *dest, size_t len) {
  assert(m_ptr + len <= m_end);
  memcpy(dest, m_ptr, len);
  m_ptr += len;
}

void BinaryReader::skip_bytes(size_t len) {
  assert(m_ptr + len <= m_end);
  m_ptr += len;
}


