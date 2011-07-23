#ifndef INCLUDE_FILEBLOCK_H
#define INCLUDE_FILEBLOCK_H

#include <boost/shared_ptr.h>

#include "MappedFile.h"

class FileBlock {
public:
  FileBlock();
  static FileBlock create(const char *filename, size_t len);
  static FileBlock open(const char *filename, size_t len);
  char *mem() { return m_mapped_file->mem(); }
private:
  boost::shared_ptr<MappedFile> m_mapped_file;
};

#endif
