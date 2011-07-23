#ifndef INCLUDE_MAPPED_FILE_H
#define INCLUDE_MAPPED_FILE_H

class MappedFile {
  MappedFile(const char *filename, size_t len, int flags);
  unsigned char *mem() { return m_mem; }
  ~MappedFile();
private:
  MappedFile(const MappedFile &rhs);
  int m_fd;
  size_t m_len;
  unsigned char *m_mem;
};
  
