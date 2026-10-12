#include "stdio.h" // printf

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

// clang -g src/main.c -o main.exe

void print_addr(struct addrinfo* addr) {

    printf("ai_flags     = %d\n", addr->ai_flags);
    printf("ai_family    = %d\n", addr->ai_family);
    printf("ai_socktype  = %d\n", addr->ai_socktype);
    printf("ai_protocol  = %d\n", addr->ai_protocol);
    printf("ai_addrlen   = %zu\n", addr->ai_addrlen);
    printf("ai_canonname = %s\n", addr->ai_canonname);

    if (addr->ai_next) {
        printf("\naddr->ai_next:\n");
        print_addr(addr->ai_next);
    }

    // int                 ai_flags;       // AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST
    // int                 ai_family;      // PF_xxx
    // int                 ai_socktype;    // SOCK_xxx
    // int                 ai_protocol;    // 0 or IPPROTO_xxx for IPv4 and IPv6
    // size_t              ai_addrlen;     // Length of ai_addr
    // char *              ai_canonname;   // Canonical name for nodename
    // _Field_size_bytes_(ai_addrlen) struct sockaddr *   ai_addr;        // Binary address
    // struct addrinfo *   ai_next;        // Next structure in linked list
}

SOCKET socket_connect(struct addrinfo* addr) {
    for (;addr; addr = addr->ai_next) {

        SOCKET sok = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sok == INVALID_SOCKET) {
            printf("ERROR: socket(): %d\n", WSAGetLastError());
            continue;
        }

        int status = connect(sok, addr->ai_addr, (int)addr->ai_addrlen);
        if (status != SOCKET_ERROR) {
            return sok;
        }

        int error = WSAGetLastError();
        char* info = "noinfo";
        switch (error) {
            case WSAENETUNREACH: info = "unreachable"; break;
            case WSAETIMEDOUT:   info = "timed out"; break;
        }

        printf("ERROR: connect() returned SOCKET_ERROR. %d - %s\n", error, info);
        closesocket(sok);
    }

    return INVALID_SOCKET;
}
/*
ai_family:
    #define AF_UNSPEC       0               // unspecified
    #define AF_INET         2               // internetwork: UDP, TCP, etc.
    #define AF_INET6        23              // Internetwork Version 6

*/
int main(int argc, char** argv) {
    printf("Hello Winsock\n");

    char* url = argv[1];
    char* port = "80"; // "27015";
    printf("url: %s\n", url);

    WSADATA wsa_data = {0};
    if (WSAStartup(MAKEWORD(2,2), &wsa_data)) {
        printf("ERROR: WSAStartup\n");
        return 1;
    }

    struct addrinfo hints = {
        .ai_flags    = AI_CANONNAME,
        .ai_family   = AF_UNSPEC,   // IPv4 or IPv6
        .ai_socktype = SOCK_STREAM, // TCP
        .ai_protocol = IPPROTO_TCP,
    };

    struct addrinfo* addr;
    if (getaddrinfo(url, port, &hints, &addr)) {
        printf("ERROR: getaddrinfo\n");
        WSACleanup();
        return 2;
    }

    print_addr(addr);

    SOCKET sok = socket_connect(addr);
    freeaddrinfo(addr);

    if (sok == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    printf("Successfully connected to socket\n");

    char* sendbuf = "Hello?";
    int bytes = send(sok, sendbuf, (int)strlen(sendbuf), 0);
    if (bytes == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(sok);
        WSACleanup();
        return 1;
    }

    printf("Bytes Sent: %d\n", bytes);

    // shutdown the connection since no more data will be sent
    if (shutdown(sok, SD_SEND) == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(sok);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection
    char buffer[512];
    int bytes_received = 0;
    while (1) {
        int bytes_received = recv(sok, buffer, sizeof(buffer), 0);
        if (bytes_received < 0) {
            printf("recv failed with error: %d\n", WSAGetLastError());
            break;
        }

        if (bytes_received == 0) {
            printf("Connection closed\n");
            break;
        }

        printf("Bytes received: %d\n", bytes_received);
        printf("%s\n", buffer);
    }

    closesocket(sok);
    WSACleanup();
}