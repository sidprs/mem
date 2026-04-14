#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

bool has_consecutive_set_bits(u_int32_t n) {
  // n : 1011  - true
  //   : 1001  - false
  //   1011 & (0110) = 0010
  //   0100 0010 0000 0011 & (0010 0001 0000 0001) = 0000 0000 0000 0010
  return (n & (n << 1)) != 0;
}

uint32_t has_k_consecutive_set_bits(uint32_t n, int k) {
  uint32_t temp = n;
  uint32_t count = 0;
  while (n != 0) {
    n = n & (n << 1);
    count++;
  }
  return count;
}
