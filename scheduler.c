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

/*
 * You receive a packed bitstream where every 5 bits represents
 * one sensor reading. Unpack them back into an array of uint8_t,
 * one reading per byte (zero-padded to 8 bits).
 *
 * Example:
 *   input (packed) : [ D E F G H I J K ] [ L M N O P 0 0 0 ]
 *   output         : [ 0 0 0 D E F G H ] [ 0 0 0 I J K L M ] [ 0 0 0 N O P 0 0 ]
 *                      ^-- each output byte holds one 5-bit reading
 *
 * @param input       packed bitstream
 * @param input_len   byte length of packed buffer
 * @param output      destination, one uint8_t per reading
 * @param num_values  how many 5-bit values to unpack
 */
void unpack_sensor_data(const uint8_t* input, size_t input_len,
                        uint8_t* output, size_t num_values);

void unpack_sensor_data(const uint8_t* input, size_t input_len, uint8_t* output, size_t num_values){
    // create a 32 int 
    //     
    uint8_t current_byte = 0;
    int8_t current_bit = 7;

    for(int i = 0; i < input_len ; i++){
        uint8_t value = 0;
        for(int j = 4; i >= 0; j--){
            uint8_t bit = (input[current_byte] >> current_bit) & 1;
            value |= bit << j;
            current_bit--;
            if (current_bit < 0) {
                current_bit = 7;
                current_byte++;
            }    
        }
        output[i] = value;
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

/*
 * A raw 8-byte CAN frame arrives as a uint8_t[8].
 * A signal is defined by:
 *   start_bit  : bit index from LSB of byte 0 (little-endian)
 *   length     : number of bits in the signal
 *
 * Extract the signal value as a uint32_t.
 *
 * Example:
 *   frame = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x12 }
 *      { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x12 }
 *      //  b0    b1    b2    b3    b4    b5    b6    b7

 *   start_bit = 48, length = 16
 *   result = 0x1234
 *
 * @param frame      pointer to 8-byte CAN frame
 * @param start_bit  starting bit position (0 = LSB of byte 0)
 * @param length     number of bits to extract (1-32)
 * @return           extracted signal value
 */
uint32_t can_unpack_signal(const uint8_t* frame, uint8_t start_bit, uint8_t length);



uint32_t can_unpack_signal(const uint8_t* frame, uint8_t start_bit, uint8_t length){
    // length in bits 
    // move it into a uint64_t then find it 
    uint64_t val = 0;
    for(int i = 0; i < 8; i++){
        val |= (uint64_t)frame[i] << (i * 8);
    }

    val = val >> start_bit;
    uint32_t result = val & ((1 << length) - 1);    
    return result;

}
/*
 * You are given a 32-bit memory-mapped status register.
 * Implement four operations without using any branching (no if/else/ternary).
 *
 * set_bit    : set bit at position pos to 1 
 * clear_bit  : clear bit at position pos to 0 
 * toggle_bit : toggle bit at position pos to other
 * check_bit  : return 1 if bit at pos is set, 0 otherwise
 *
 * @param reg  pointer to the 32-bit register
 * @param pos  bit position (0-31)
 */
void set_bit   (volatile uint32_t* reg, uint8_t pos);
void clear_bit (volatile uint32_t* reg, uint8_t pos);
void toggle_bit(volatile uint32_t* reg, uint8_t pos);
uint8_t check_bit(volatile uint32_t* reg, uint8_t pos);


void set_bit(volatile uint32_t* reg, uint8_t pos){
    if (reg == NULL || pos >= 32) return;
    *reg |= (1 << pos);
}

void clear_bit (volatile uint32_t* reg, uint8_t pos){
    *reg &= ~(1<< pos);
}

void toggle_bit(volatile uint32_t* reg, uint8_t pos){
    *reg ^= (1<< pos);
}

uint8_t check_bit(volatile uint32_t* reg, uint8_t pos){
    return (uint8_t)((*reg >> pos) & 1U);
}

int has_consecutive_ones(uint32_t n)
{
    return (n & (n << 1)) != 0;
}

int has_k_consecutive_ones(uint32_t n, int k)
{
    uint32_t mask = n;

    for (int i = 1; i < k; i++) {
        mask &= (n >> i);
    }

    return mask != 0;
}

/*
 *
 * A BMS status register is a uint32_t with the following layout:
 *
 *   bits [31:24] - reserved
 *   bits [23:16] - temperature (raw ADC, 8-bit unsigned)
 *   bits [15:8]  - cell voltage (raw ADC, 8-bit unsigned)
 *   bits [7:4]   - fault flags (4 bits)
 *   bits [3:0]   - state machine state (4 bits)
 *
 * (a) Write get_field() and set_field() for arbitrary bit ranges.
 * (b) Using only those two, extract temperature and write back
 *     a new fault flag value of 0b1010 without touching any
 *     other field.
 *
 * uint32_t get_field(uint32_t reg, uint8_t shift, uint8_t width);
 * void     set_field(uint32_t *reg, uint8_t shift, uint8_t width, uint32_t val);
 */


uint32_t get_field(uint32_t reg, uint8_t shift, uint8_t width);
void     set_field(uint32_t *reg, uint8_t shift, uint8_t width, uint32_t val);


uint32_t get_field(uint32_t reg, uint8_t shift, uint8_t width){
    /*
        reg — the full 32-bit status register value
        shift — which bit position the field starts at (LSB of the field)
        width — how many bits wide the field is
    */  
    uint32_t reg_c = reg;
    return (reg_c >> shift) & ((1U << width) - 1);  
    //   reg >> start & (1 << (width) - 1)
}


/*
 * You have a packed 3-byte (24-bit) big-endian buffer from an AFE
 * over SPI. It holds a 20-bit signed cell voltage reading in the
 * upper 20 bits (bits [23:4]), and a 4-bit status nibble in [3:0].
 *
 *   buf = { 0xA4, 0xB3, 0xC5 }
 *          MSB          LSB
 *
 * (a) Extract the 20-bit raw voltage as a uint32_t.
 * (b) Sign-extend it to int32_t (the raw value is two's complement).
 * (c) Extract the 4-bit status nibble.
 *
 * int32_t  get_voltage(const uint8_t buf[3]);
 * uint8_t  get_status(const uint8_t buf[3]);
 *
 * Do not use memcpy or any union tricks — only shifts and masks.
 */

int32_t  get_voltage(const uint8_t buf[3]){
    uint32_t raw = 0;
    for(int i =0; i < 3; i++){
        raw |= (uint32_t)buf[i] << ((2-i)*8);
    }
    uint32_t voltage_20 = raw >> 4;
    if (voltage_20 & (1U << 19)) {        // if bit 19 (MSB of 20-bit value) is set
        voltage_20 |= 0xFFF00000;         // fill bits [31:20] with 1s
    }

    return (int32_t)voltage_20;
}

uint8_t get_status(const uint8_t buf[3]) {
    return buf[2] & 0x0F;   // status nibble is bits [3:0] of last byte
}




/***
 *  Bitstream Packet Construction
 *
 *  You are given 3 fields:
 *  - 11-bit sensor ID
 *  - 5-bit command
 *  - 8-bit payload
 *
 *  You must pack them into a continuous bitstream and compute a checksum.
 *
 *  Bit Layout (MSB-first streaming order):
 *
 *    ID (11 bits) | CMD (5 bits) | PAYLOAD (8 bits) | CHECKSUM (8 bits)
 *
 *  Checksum rule:
 *      checksum = (id + command + payload) & 0xFF
 *
 *  Output buffer is 6 bytes (48 bits total).
 *  Unused bits must be zeroed.
 *
 *
 *  Example:
 *
 *  id      = 0b00000101101 (11 bits)
 *  cmd     = 0b10101       (5 bits)
 *  payload = 0xA7          (8 bits)
 *
 *  Stream:
 *
 *      ID (11 bits)    CMD (5 bits)    PAYLOAD (8 bits)    CHECKSUM (8 bits)
 *   [ A B C D E F G H I J K ] [ L M N O P ] [ Q R S T U V W X ] [ Y Z ... ]
 *
 *   Bits are packed continuously into bytes:
 *
 *       Byte0        Byte1        Byte2        Byte3        Byte4        Byte5
 *   [ ........ ] [ ........ ] [ ........ ] [ ........ ] [ ........ ] [ ........ ]
 *
 *
 *  Requirements:
 *  - MSB-first bit packing
 *  - No structs or bitfields
 *  - No undefined behavior in shifts
 *  - Must not write beyond output[6]
 *  - All unused bits must be zero
 *
 *
 *  Function to implement:
 *
 *  void build_packet(uint16_t id,
 *                    uint8_t command,
 *                    uint8_t payload,
 *                    uint8_t *out);
 *
 */