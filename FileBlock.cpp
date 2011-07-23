// Self
#include "FileBlock.h"

FileBlock::FileBlock() {}

static FileBlock FileBlock::create(const char *filename, size_t len) {
  FileBlock ret;
  ret.m_block.reset(new MappedFile(filename, len, O_RDWR | O_CREAT));
  return ret;
}

static FileBlock FileBlock::open(const char *filename, size_t len) {
  FileBlock ret;
  ret.m_block.reset(new MappedFile(filename, len, O_RDWR));
  return ret;
}
