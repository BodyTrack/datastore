#ifndef SIZES_H
#define SIZES_H

typedef int int32;
typedef unsigned int uint32;
typedef long long int64;
typedef unsigned long long uint64;

inline bool sizes_are_valid() {
  return 
    sizeof(int32) == 4 &&
    sizeof(uint32) == 4 &&
    sizeof(int64) == 8 &&
    sizeof(uint64) == 8
    ;
}
    
#endif
