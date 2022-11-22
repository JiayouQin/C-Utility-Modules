#pragma once
#define INIT_POS 0x010600257530  //????
#define NOMINMAX //be wary of evil windows.h
#include <CRC16RTU.h>
#include <windows.h>
#include "ParamsLoader.h"

class Serial {

HANDLE hComm;
std::string port_name = Params::motorSerial;  //change port name
char wtBuf[8] = "";
char retBuf[8];
DWORD dNoOfBytesWritten = 0;     // No of bytes written to the port
DWORD bytes_read = 0;
int status = -1;
public:

    Serial() {
        hComm = CreateFileA(port_name.c_str(),                //port name
            GENERIC_READ | GENERIC_WRITE, //Read/Write
            0,                            // No Sharing
            NULL,                         // No Security
            OPEN_EXISTING,// Open existing port only
            0,            // Non Overlapped I/O
            NULL);        // Null for Comm Devices

        if (hComm == INVALID_HANDLE_VALUE)
        {
            std::cerr << "Error in opening serial port¡±" << std::endl;
            status = -1;
            return;

        }
        std::cerr << "opening serial port successful" << std::endl;
        status = 0;

        DCB dcbSerialParams = { 0 }; // Initializing DCB structure
        dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
        GetCommState(hComm, &dcbSerialParams);

        dcbSerialParams.BaudRate = CBR_115200;
        dcbSerialParams.ByteSize = 8;         // Setting ByteSize = 8
        dcbSerialParams.StopBits = ONESTOPBIT;// Setting StopBits = 1
        dcbSerialParams.Parity = NOPARITY;  // Setting Parity = None
        SetCommState(hComm, &dcbSerialParams);

        COMMTIMEOUTS timeouts = { 0 };
        timeouts.ReadIntervalTimeout = 20; // in milliseconds
        timeouts.ReadTotalTimeoutConstant = 100; // in milliseconds
        timeouts.ReadTotalTimeoutMultiplier = 10; // in milliseconds
        timeouts.WriteTotalTimeoutConstant = 100; // in milliseconds
        timeouts.WriteTotalTimeoutMultiplier = 10; // in milliseconds 

    }
    
    ~Serial() {
        CloseHandle(hComm);//Closing the Serial Port
    }

    void printHex(char* bts) {
        for (int i = 0; i < 8; i++) {
            std::cout << std::hex << ((short)bts[i] & 0xff) << " ";
        }
        std::cout << std::endl;
    }

    void sendmsg(long long bts) {
        uchar msgBuf[6], crcRetBuf[2];
        memcpy(msgBuf, &bts, 6);
        //*((long long*)&msgBuf) = bts; //corrupt
        std::reverse((char*)&msgBuf, (char*)&msgBuf + sizeof(msgBuf));
        crc16RTU(msgBuf, sizeof(msgBuf), crcRetBuf);

        *((long long*)&wtBuf) = bts << 16;
        wtBuf[0] = crcRetBuf[1];
        wtBuf[1] = crcRetBuf[0];
        std::reverse((char*)&wtBuf, (char*)&wtBuf + sizeof(wtBuf));
        std::cout << "sending message: " << std::endl;
        printHex(wtBuf);

        WriteFile(hComm, &wtBuf, 8, &dNoOfBytesWritten, //Bytes written
            NULL);
        Sleep(50);
        do {
            ReadFile(hComm, retBuf, 100, &bytes_read, NULL);

        } while (bytes_read <= 0);
        std::cout << "Read Data : " << std::endl;
        printHex(retBuf);
    }



};