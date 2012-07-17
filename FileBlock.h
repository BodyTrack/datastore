#ifndef INCLUDE_FILEBLOCK_H
#define INCLUDE_FILEBLOCK_H

#include "MappedFile.h"
#include "simple_shared_ptr.h"

class FileBlock {
public:
  FileBlock();
  static FileBlock create(const char *filename, size_t len);
  static FileBlock open(const char *filename, size_t len);
  char *mem() { return m_mapped_file->mem(); }
private:
  simple_shared_ptr<MappedFile> m_mapped_file;
};

#endif
