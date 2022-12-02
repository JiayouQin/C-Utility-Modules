#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

class CommClass {

private:
    WSADATA wsaData;
    struct addrinfo* result = NULL, * ptr = NULL, hints;
    const char* defaultMessage = "this is a test";
    int bufLength = 512;
    const char* ip;
    const char* port;
    CustomLogger& logger = CustomLogger::get();
    bool connected = false;
    bool* runProcess = 0;
public:
    char recvbuf[256] = "";
    void(__stdcall* errCallback)() = 0;
    void(__stdcall* connectCallback)() = 0;

    CommClass(bool* runProcess = 0, const char* ip="127.0.0.1", const char* port = "8081") {
        this->ip = ip;
        this->port = port;
        this->runProcess = runProcess;
    }
    ~CommClass() {
        this->close();
        freeaddrinfo(result);
    }

    SOCKET sock = INVALID_SOCKET;

    int connectSocket() {
        int ret;
         ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (ret != 0) {
            logger.warn("WSA启动失败:{}", ret);
            return ret;
        }

        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        // Resolve the server address and port
        ret = getaddrinfo(ip, port, &hints, &result);
        logger.info("尝试连接至主程序");
        if (ret != 0) {
            logger.warn("无法获取地址信息: {}",ret);
            this->close();
            return 1;
        }

        for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
            this->sock = socket(ptr->ai_family, ptr->ai_socktype,
                ptr->ai_protocol);
            if (this->sock == INVALID_SOCKET) {
                logger.warn("套字节出错:{}", WSAGetLastError());
                this->close();
                return 1;
            }

            ret = connect(this->sock, ptr->ai_addr, (int)ptr->ai_addrlen);
            if (ret == SOCKET_ERROR) {
                closesocket(this->sock);
                logger.warn("套字节错误");
                this->sock = INVALID_SOCKET;
                continue;
            }
            if (this->sock == INVALID_SOCKET) {
                logger.warn("无效套字节");
                WSACleanup();
                return 1;
            }
            this->connected = true;
            connectCallback();
            return 0;
        }
        //freeaddrinfo(result);
        return 1;
    }

    int sendMessage(const char* message="") {
        if (message == "") { message = defaultMessage; };
        int ret;
        // Send an initial buffer
        ret = send(this->sock, (char*)message, (int)strlen(message), 0);
        if (ret == SOCKET_ERROR) {
            logger.warn("发送信息失败: {}", WSAGetLastError());
            closesocket(this->sock);
            WSACleanup();
            return 1;
        }
        return 0;
    }

    int waitMessage() {
        memset(recvbuf, 0, sizeof recvbuf);
        int ret = recv(this->sock,recvbuf ,256,0);
        if (ret == 0) {
            logger.warn("主机连接已断开");
            connected = false;
        }

        while (!connected) {
            Sleep(500);
            if (runProcess) {
                if (!(*runProcess)) { break; }
            }
            errCallback();
            logger.warn("重新连接套字节中……");
            this->connectSocket();
        }
        //initialize callback
        return ret;
    }

    int close() {
        closesocket(this->sock);
        int ret = shutdown(this->sock, SD_SEND);
        if (ret == SOCKET_ERROR) {
            logger.warn("无法关闭套字节: {}", WSAGetLastError());
            closesocket(this->sock);
            WSACleanup();
            return 1;
        }
        WSACleanup();
        return 0;
    };

};