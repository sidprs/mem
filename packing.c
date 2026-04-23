#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


typedef struct{
    uint32_t* buffer;
    size_t buffer_size;
} can_bus_buffer_t;

can_bus_buffer_t packet_info[5];

void pack_can_bus_lsb(const uint8_t* buffer, size_t buff_len, uint8_t* outbuff, size_t out_len)
{
    uint32_t current_bit = 7;
    uint32_t current_byte = 0;
    // pack a 5 bit value from the buffer into 
    for(int i = 0; i < buff_len; i++){
        uint32_t mask = buffer[i] & 0x1F;
        for(int j = 4; j >= 0; j--){
            uint32_t bit_to_out = (mask >> j) & 1;
            outbuff[current_byte] |= bit_to_out << current_bit;
            if(current_bit < 0){
                current_byte++;
                current_bit = 7;
            }
        }
    }
}

/*
 * Reverse the bits of a single uint8_t
 * Input:  0b11010000  (0xD0)
 * Output: 0b00001011  (0x0B)
 */
uint8_t reverse_bits(uint8_t x);

uint8_t reverse_bits(uint8_t x){
    uint8_t out = 0;
    for(int i = 0; i < 8; i++){
        out <<= 1;
        uint8_t n = (x >> 1) & 1;
        out |= n << (7 - i);
    }
    
}


/*
 * Count the number of 1-bits in a uint32_t
 * Input:  0xDEADBEEF
 * Output: 24
 * 
 * Follow-up: can you do it without a loop?
 */
uint8_t popcount(uint32_t x){
    uint32_t count = 0;
    for(int i = 0; i < 32; i++){
        uint8_t curr = (x >> i) & 0x1;
        if(curr == 1){
            count++;
        }
    }
    return count;

}
/*
 * A 32-bit status register is laid out as:
 *   [31:16] reserved
 *   [15:12] device_id   (4 bits)
 *   [11:8]  error_code  (4 bits)
 *   [7:4]   mode        (4 bits)
 *   [3:0]   flags       (4 bits)
 *
 * Extract each field from the raw register value.
 */
typedef struct {
    uint8_t device_id;
    uint8_t error_code;
    uint8_t mode;
    uint8_t flags;
} RegFields;

RegFields parse_register(uint32_t reg){
    RegFields* out;
    out->flags |= (reg>>0) & ((1 << 3)-1);
    out->mode |= (reg>>4) & ((1 << 3)-1);
    out->error_code |= (reg>>8) & ((1 << 3)-1);
    out->device_id |= (reg>>12) & ((1 << 3)-1);




}

/*
 * A CAN frame's 4-byte data field holds two signals:
 *
 *   Byte layout (big-endian / Motorola):
 *   Byte 0: [ S S S S S S S S ]  <- RPM bits [15:8]
 *   Byte 1: [ R R R R R R R R ]  <- RPM bits [7:0]
 *   Byte 2: [ T T T T T T T T ]  <- Temp bits [9:2]
 *   Byte 3: [ T T x x x x x x ]  <- Temp bits [1:0] in [7:6], rest unused
 *
 *   RPM  : uint16_t, range 0–65535
 *   Temp : uint16_t, 10-bit raw, scale = raw * 0.25 - 40.0  (degrees C)
 *
 * Extract both signals from data[4].
 */
void parse_can_frame(const uint8_t data[4],
                     uint16_t* rpm_out,
                     float*    temp_out);


#include <stdint.h>

void parse_can_frame(const uint8_t data[4],
                     uint16_t* rpm_out,
                     float* temp_out)
{
    // ---- RPM (big-endian) ----
    *rpm_out = ((uint16_t)data[0] << 8) | data[1];

    // ---- Temp (10-bit raw) ----
    uint16_t temp_raw = 0;

    temp_raw |= ((uint16_t)data[2]) << 2;      // bits 9..2
    temp_raw |= (data[3] >> 6) & 0x03;         // bits 1..0

    // ---- scaling ----
    *temp_out = (temp_raw * 0.25f) - 40.0f;
}


/*
 * A BMS cell measurement must be sent over SPI as 3 bytes.
 * Pack the following fields tightly, MSB first:
 *
 *   [23:12]  voltage_raw  12 bits  (0–4095)
 *   [11:4]   temp_raw      8 bits  (0–255)
 *   [3:1]    status        3 bits  (0–7)
 *   [0]      valid         1 bit   (0 or 1)
 *
 * Example: voltage=0xABC, temp=0x5E, status=0b101, valid=1
 * Output bytes: [0xAB][0xC5][0xEB]
 */
#include <stdint.h>

void pack_bms_cell(uint16_t voltage_raw, uint8_t temp_raw,
                   uint8_t status,       uint8_t valid,
                   uint8_t out[3]){

    uint32_t extract_values = 0;

    // place each field in correct position
    extract_values |= ((uint32_t)(voltage_raw & 0x0FFF)) << 12;
    extract_values |= ((uint32_t)(temp_raw    & 0xFF))   << 4;
    extract_values |= ((uint32_t)(status      & 0x07))   << 1;
    extract_values |= ((uint32_t)(valid       & 0x01))   << 0;

    // split into bytes (MSB first)
    out[0] = (extract_values >> 16) & 0xFF;
    out[1] = (extract_values >> 8)  & 0xFF;
    out[2] = (extract_values >> 0)  & 0xFF;
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
 *             ID (11 bits)    CMD (5 bits)    PAYLOAD (8 bits)    CHECKSUM (8 bits)
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




// - - - - - - - - - - - | 
void build_packet(uint16_t id, uint8_t command, uint8_t payload, uint8_t* out){
    uint32_t total_bit = 0;
    uint32_t checksum = (id + command + payload);

    total_bit |= (uint32_t)(id & 0x7FF) << 21;
    total_bit |= (uint32_t)(command & 0x1F) << 16;
    total_bit |= (uint32_t)(payload & 0xFF) << 8;
    total_bit |= (uint32_t)(checksum & 0xFF) << 8;
 
    // we created a 32 int total bit containing everything now we need to pack into out;
    size_t input_leng = 32;
    for (int i = 0; i < input_leng; i++) {
        uint8_t bit = (total_bit >> (31 - i)) & 1;

        int byte_idx = i / 8;
        int bit_idx = 7 - (i % 8);

        out[byte_idx] |= bit << bit_idx;
    }


}

void pack_msb(const uint8_t* buff, size_t len, uint8_t* out){

    size_t bit_pos = 0;
    for(int i = 0; i < len; i++){
        uint8_t mask = buff[i]; /* & 0x1F for 5 bits*/
        for(int j = 7; j >= 0 ; j--){
            uint8_t pos_to_out = (mask >> j) & 1;
            
            size_t bite_idx = 7 - (bit_pos % 8);
            size_t byte_pos = bit_pos / 8;

            out[byte_pos] |= pos_to_out << bite_idx;
            bit_pos++;
        }

    }


}


void pack_lsb(const uint8_t* buff, size_t len, uint8_t* out){

    size_t bit_pos = 0;
    for(int i = 0; i < len; i++){
        uint8_t mask = buff[i]; /* & 0x1F for 5 bits*/
        for(int j = 0; j < 8 ; j){
            uint8_t pos_to_out = (mask >> j) & 1;
            
            size_t bite_idx =  (bit_pos % 8);
            size_t byte_pos = bit_pos / 8;

            out[byte_pos] |= pos_to_out << bite_idx;
            bit_pos++;
        }

    }

}
int main(){
    printf("%d", popcount(1));

    return 0;
}