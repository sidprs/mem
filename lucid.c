#include <stdint.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    uint16_t u16;
    uint32_t u32;
    float    f1;
} data_pack;

#define STRUCT_BITS (sizeof(data_pack) * 8)

// Pack struct into a bit array (uint8_t buf, 1 bit per element)
void pack_bits(const data_pack *src, uint8_t *bitbuf) {
    const uint8_t *bytes = (const uint8_t *)src;
    for (int i = 0; i < sizeof(data_pack); i++) {
        for (int j = 0; j < 8; j++) {
            bitbuf[8*i + j] = (bytes[i] >> j) & 1;  // LSB first
        }
    }
}

// Unpack bit array back into struct
void unpack_bits(const uint8_t *bitbuf, data_pack *dst) {
    uint8_t *bytes = (uint8_t *)dst;
    memset(dst, 0, sizeof(data_pack));
    for (int i = 0; i < sizeof(data_pack); i++) {
        for (int j = 0; j < 8; j++) {
            bytes[i] |= (bitbuf[8*i + j] & 1) << j;  // reconstruct byte
        }
    }
}

int main() {
    data_pack original = {0xABCD, 0x12345678, 3.14f};
    uint8_t bitbuf[STRUCT_BITS];
    data_pack recovered;

    pack_bits(&original, bitbuf);
    unpack_bits(bitbuf, &recovered);

    // Verify
    printf("u16: 0x%04X\n", recovered.u16);
    printf("u32: 0x%08X\n", recovered.u32);
    printf("f1:  %f\n",     recovered.f1);
}