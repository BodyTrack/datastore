// C
#include <unistd.h>
#include <string.h>

#include "FileBlock.h"

int main(int argc, char **argv)
{
  unlink("foo.block");
  FileBlock fb = FileBlock::create("foo.block", 1024*1024);
  strcpy(fb.mem, "hello world!");
  return 0;
}
