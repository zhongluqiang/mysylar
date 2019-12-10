#include "sylar/endian.h"
#include <iostream>

int main() {
    if (SYLAR_BYTE_ORDER == SYLAR_LITTLE_ENDIAN) {
        std::cout << "little endian" << std::endl;
    } else {
        std::cout << "big endian" << std::endl;
    }
    printf("%x\n", 0x12345678);
    printf("%x\n", sylar::byteswapOnLittleEndian<uint32_t>(0x12345678));
    printf("%x\n", sylar::byteswapOnBigEndian<uint32_t>(0x12345678));
    return 0;
}