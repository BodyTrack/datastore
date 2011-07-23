#include <stdio.h>
int main(int argc, char **argv)
{
  unsigned int magic = *(unsigned int*)argv[1];
  printf("Magic('%s') = 0x%08x\n", argv[1], magic);
  return 0;
}
