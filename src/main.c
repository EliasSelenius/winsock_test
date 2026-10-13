#include "stdio.h" // printf
#include <stdlib.h>
#include <string.h>

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

char* get_error_info(int error) {
    switch (error) {
        case WSAENETUNREACH: return "unreachable";
        case WSAETIMEDOUT:   return "timed out";
        case WSAECONNRESET:  return "An existing connection was forcibly closed by the remote host.";
    }
    return "";
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
        char* info = get_error_info(error);
        printf("ERROR: connect() returned SOCKET_ERROR. %d - %s\n", error, info);
        closesocket(sok);
    }

    return INVALID_SOCKET;
}


typedef struct string {
    char* chars;
    size_t length;
} string;

string string_format(char* format, ...) {
    va_list arg_list;
    va_start(arg_list, format);

    int size = vsnprintf(0, 0, format, arg_list);

    string str = {0};
    str.chars = calloc(1, size);
    str.length = vsnprintf(str.chars, size+1, format, arg_list);

    va_end(arg_list);
    return str;
}

/*
ai_family:
    #define AF_UNSPEC       0               // unspecified
    #define AF_INET         2               // internetwork: UDP, TCP, etc.
    #define AF_INET6        23              // Internetwork Version 6

*/
int main(int argc, char** argv) {
    printf("Hello Winsock\n");

    if (argc < 2) {
        printf("Usage: %s <hostname> [port]\n", argv[0]);
        return 1;
    }

    char* url = argv[1];
    char* port = argc > 2 ? argv[2] : "80"; // plaintext HTTP normally uses 80; 443 is HTTPS and requires TLS/SSL.
    printf("url: %s\n", url);
    printf("port: %s\n", port);

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

    // print_addr(addr);

    SOCKET sok = socket_connect(addr);
    freeaddrinfo(addr);

    if (sok == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }

    printf("Successfully connected to socket\n");

    {
        string req = string_format(
            "GET / HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Accept: */*\r\n"
            "\r\n",
            url);

        printf("request-length: %zu\n", req.length);
        printf("request:\n%s\n", req.chars);

        int bytes = send(sok, req.chars, req.length, 0);
        free(req.chars);

        if (bytes == SOCKET_ERROR) {
            int error = WSAGetLastError();
            char* info = get_error_info(error);
            printf("send failed with error: %d - %s\n", error, info);
            closesocket(sok);
            WSACleanup();
            return 1;
        }

        printf("Bytes Sent: %d\n", bytes);
    }


    // shutdown the connection since no more data will be sent
    if (shutdown(sok, SD_SEND) == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(sok);
        WSACleanup();
        return 1;
    }

    // Receive until the peer closes the connection

    int buffer_size = 512;
    char* buffer = calloc(1, buffer_size);
    int total_bytes = 0;

    while (1) {
        int bytes = recv(sok, buffer + total_bytes, buffer_size - total_bytes, 0);
        if (bytes < 0) {
            int error = WSAGetLastError();
            char* info = get_error_info(error);
            printf("recv failed with error: %d - %s\n", error, info);
            break;
        }

        if (bytes == 0) {
            printf("Connection closed\n");
            break;
        }

        total_bytes += bytes;
        printf("Bytes received: %d\n", total_bytes);

        buffer = realloc(buffer, buffer_size*=2);
    }

    printf("Total bytes received: %d\n", total_bytes);
    printf("%s\n", buffer);

    closesocket(sok);
    WSACleanup();
}