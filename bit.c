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


// bit packing 


void pack_buffer(uint32_t* input, size_t input_len, uint32_t* output){
  // move 5 LSB from input into output into continuous buffer

  uint32_t input_bit = 7;
  uint32_t input_byte = 0; 
  for(int i =0; i <input_len; i++){
      uint32_t out_bit_mask = (input[i] & 0x1F);
      for(int j = 4; j >=0 ;j--){
        uint32_t mask = (out_bit_mask >> j) & 1;
        output[input_byte]  |= (mask << input_bit);
        input_bit--;
        if(input_bit < 0 ){
          input_byte++;
          input_bit = 7;
        }
      }

  }

}


typedef struct{
  uint8_t *buff;
  size_t buff_size;
} sensor_pack_t;

sensor_pack_t sensor_table = {
  .buff = NULL,
  .buff_size = 0
};