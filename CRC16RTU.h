#pragma once
typedef unsigned int uint;
typedef unsigned char uchar;
typedef unsigned long ulong;
void crc16RTU(uchar* buf, int len, uchar* retBuf)
{
    uint crc = 0xFFFF;
    for (int i = 0; i != len; i++) {
        crc ^= (uint)buf[i];    // XOR byte into least sig. byte of crc
        for (int i = 8; i != 0; i--) {    // Loop over each bit
            if ((crc & 0x0001) != 0) {      // If the LSB is set
                crc >>= 1;                    // Shift right and XOR 0xA001
                crc ^= 0xA001;
            }
            else                            // Else LSB is not set
                crc >>= 1;                    // Just shift right
        }
    }
    *(short*)retBuf = crc;
}