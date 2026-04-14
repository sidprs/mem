#include <stdint.h>
#include <stddef.h>
#include <assert.h>
#include <stdio.h>


/***
 *  Bit Streaming  
 * 
 *  8-bit sensor readings 
 *  Extract the 5 LSB of each input bytes
 *  Pack them contigously in the output buffer
 * 
 *  Example:
 *  2 Bytes [ B1 ][ B2 ]  
 *  B1 = [ A B C D E F G H ]
 *  B2 = [ I J K L M N O P ]
 *              
 *           [ b1      ][ b2   ] [b2][           ]
 *  Output : [ D E F G H L M N ] [O P 0 0 0 0 0 0]
 * 
 */

/***
 *  @param input Pointer to the source of 8-bit sensor data
 *  @param input_length number of elements in the input array
 *      @return     Up to design 
 * 
 */

void pack_sensor_data(const uint8_t* input, size_t input_len, uint8_t* output, size_t output_len);

void pack_sensor_data(const uint8_t* input, size_t input_len, uint8_t* output, size_t output_len){

    uint8_t current_byte = 0;
    int8_t current_bit = 7;
    memset(output, 0, output_len);

    for(int i = 0; i < input_len ; i++){
        uint8_t post_max_bit = input[i] & 0x1F;

        for(int j = 4; j >= 0; j--){
            uint8_t bit_to_out = (post_max_bit >> j) & 1;
            output[current_byte] |= bit_to_out << current_bit;
            current_bit--;
            if(current_bit < 0 ) { 
                current_byte++; 
                current_bit = 7;
            }
        }
    }
}


/***
 *  RLE Bit Decoder
 *
 *  You receive a run-length encoded bitstream.
 *  Each "run" is encoded as 2 bytes:
 *
 *    [ COUNT ][ BYTE_VAL ]
 *
 *  COUNT  : number of times to repeat BYTE_VAL (1–255, never 0)
 *  BYTE_VAL : the byte value to repeat
 *
 *  BUT — the output is bit-packed:
 *  Only the 6 LSBs of each BYTE_VAL are written to the output buffer,
 *  packed contiguously (MSB of those 6 bits first).
 *
 *  Example:
 *    Input:  [ 0x03 ][ 0xC5 ]
 *    0xC5 = 1100 0101 → 6 LSBs = 00 0101
 *    COUNT = 3, so write "000101" three times back to back:
 *
 *    Output bits: 000101 000101 000101 00 (padded)
 *    = [ 0x14 ][ 0x51 ][ 0x40 ]
 *
 *  The output buffer is zero-initialized for you.
 *  Return the number of output bytes written (including any partial final byte).
 *  If the output buffer is too small, return -1.
 *
 *  @param input        Pointer to RLE-encoded input
 *  @param input_len    Must be even (pairs of bytes); if odd, return -1
 *  @param output       Pointer to zero-initialized output buffer
 *  @param output_len   Size of output buffer in bytes
 *  @return             Bytes written, or -1 on error
 */
int rle_bit_decode(const uint8_t* input, size_t input_len,
                   uint8_t* output, size_t output_len);


int rle_bit_decode(const uint8_t* input, size_t input_len, uint8_t* output, size_t output_len){

    uint64_t total_count = 0;
    for(int x = 0; x < input_len; x+=2){
        total_count += input[x];
    }


    size_t total_bits = total_count * 6;
    size_t required_bytes = (total_bits + 7) / 8;
    
    if (required_bytes > output_len) return -1;

    memset(output, 0, required_bytes);


    size_t current_byte = 0;
    int8_t current_bit = 7; 
    for(int i = 1; i < input_len; i+=2){
        uint8_t count = input[i -1];
        uint8_t mask = input[i];
        while(count != 0){ 
            uint8_t output_bit = (mask & 0x3F);
            for(int j = 5; j >=0 ; j--){
                uint8_t copy_to_out = (output_bit >> j) & 1;
                output[current_byte] |= (copy_to_out << current_bit);
                current_bit--;
                if(current_bit < 0 ) { 
                    current_byte++; 
                    current_bit = 7;
                }
            }
            count--;
        }
    }
    return (int)required_bytes;
}