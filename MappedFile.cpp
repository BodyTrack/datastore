#include "MappedFile.h"

MappedFile::MappedFile(const char *filename, size_t len, int flags) {
  m_len = len;
  m_fd = open(filename, flags);
  if (m_fd == -1) {
    throw std::runtime_error("open");
  }

  const unsigned char *m_mem = (unsigned char*)
    mmap(NULL, m_len, PROT_READ|PROT_WRITE,
         MAP_SHARED|MAP_POPULATE, m_fd, 0);
  if (m_mem == (unsigned char*)-1) {
    close(m_fd);
    throw std::runtime_error("mmap");
  }
}

MappedFile::~MappedFile() {
  munmap(m_mem, m_len);
  close(m_fd);
}
