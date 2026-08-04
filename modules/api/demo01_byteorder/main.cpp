#include <iostream>

/**
 * 检查机器的字节序
 * */

void byteorder() {
    union {
        short value;
        char union_bytes [sizeof (short)];
    } test;

    test.value = 0x0102;
    if ((test.union_bytes[0] == 1) && (test.union_bytes[1] == 2)) {
        std::cout << "big endian" << std::endl;
    } else if ((test.union_bytes[0] == 2) && (test.union_bytes[1] == 1)) {
        std::cout << "little endian" << std::endl;
    } else {
        std::cout << "unknown..." << std::endl;
    }
}

int main() {
    byteorder();
    return 0;
}
