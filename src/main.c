#include <stdio.h> // printf
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <stdint.h>


#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

// clang -g src/main.c -o main.exe

typedef struct addrinfo addrinfo;

// void* bump_alloc(void* buffer, )


char* socktype_str[] = {
    [SOCK_STREAM]    = "SOCK_STREAM",
    [SOCK_DGRAM]     = "SOCK_DGRAM",
    [SOCK_RAW]       = "SOCK_RAW",
    [SOCK_RDM]       = "SOCK_RDM",
    [SOCK_SEQPACKET] = "SOCK_SEQPACKET",
};

char* family(int f) {
    switch (f) {
        case AF_UNSPEC: return "AF_UNSPEC";
        case AF_INET:   return "AF_INET";
        case AF_INET6:  return "AF_INET6";
    }
    return 0;
}

void print_addresses(struct addrinfo* addr) {
    for (; addr; addr = addr->ai_next) {
        char node_name[128];
        char serv_name[64];
        int flags = NI_NUMERICHOST | NI_NUMERICSERV;

        int ret = getnameinfo(addr->ai_addr, addr->ai_addrlen, node_name, sizeof(node_name), serv_name, sizeof(serv_name), flags);

        char* ip_version = 0;
        switch (addr->ai_family) {
            case AF_INET:  ip_version = "IPv4"; break;
            case AF_INET6: ip_version = "IPv6"; break;
        }

        printf("\t%s............%s\n", ip_version, node_name);
    }
}

void print_addr(struct addrinfo* addr) {

    // printf("ai_flags     = %d\n", addr->ai_flags);
    printf("ai_family    = %s\n", family(addr->ai_family));
    printf("ai_socktype  = %s\n", socktype_str[addr->ai_socktype]);
    printf("ai_protocol  = %d\n", addr->ai_protocol);
    // printf("ai_addrlen   = %zu\n", addr->ai_addrlen);
    // printf("ai_canonname = %s\n", addr->ai_canonname);

    char node_name[128];
    char serv_name[64];
    int flags = NI_NUMERICHOST | NI_NUMERICSERV;

    int ret = getnameinfo(addr->ai_addr, addr->ai_addrlen, node_name, sizeof(node_name), serv_name, sizeof(serv_name), flags);

    char* ip_version = 0;
    switch (addr->ai_family) {
        case AF_INET:  ip_version = "IPv4"; break;
        case AF_INET6: ip_version = "IPv6"; break;
    }

    printf("\t%s............%s\n", ip_version, node_name);

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
    int size = vsnprintf(NULL, 0, format, arg_list);
    va_end(arg_list);

    string str = {0};
    if (size < 0) return str;
    str.chars = calloc(1, size + 1);

    va_start(arg_list, format);
    str.length = vsnprintf(str.chars, size + 1, format, arg_list);
    va_end(arg_list);
    return str;
}


typedef enum Addr_Family {
    Addr_Family_Unspecified = AF_UNSPEC,
    Addr_Family_Inet        = AF_INET,
    Addr_Family_Inet6       = AF_INET6,
} Addr_Family;

typedef enum Sok_Type {
    Sok_Type_Stream    = SOCK_STREAM,    // stream socket
    Sok_Type_Dgram     = SOCK_DGRAM,     // datagram socket
    Sok_Type_Raw       = SOCK_RAW,       // raw-protocol interface
    Sok_Type_Rdm       = SOCK_RDM,       // reliably-delivered message
    Sok_Type_Seqpacket = SOCK_SEQPACKET, // sequenced packet stream
} Sok_Type;

addrinfo* resolve_host(char* url_or_ip, char* port_or_service) {
    addrinfo hints = {
        .ai_flags    = AI_CANONNAME,
        .ai_family   = AF_UNSPEC,   // IPv4 or IPv6
        .ai_socktype = SOCK_STREAM, // TCP
        .ai_protocol = IPPROTO_TCP,
    };

    addrinfo* addr;
    if (getaddrinfo(url_or_ip, port_or_service, &hints, &addr)) {
        printf("ERROR: getaddrinfo\n");
        WSACleanup();
        return 0;
    }
    return addr;
}

#pragma pack(push, 1)
typedef struct Icmp_Header {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
} Icmp_Header;

typedef struct Ip_Header {
    uint8_t  ver_ihl;   // version (4 bits) + IHL (4 bits)
    uint8_t  tos;
    uint16_t total_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint32_t src_addr;
    uint32_t dst_addr;
} Ip_Header;
#pragma pack(pop)

unsigned short checksum_icmp(const void* data, size_t length) {
    const unsigned char* bytes = (const unsigned char*)data;
    unsigned long sum = 0;

    for (size_t i = 0; i + 1 < length; i += 2) {
        sum += ((unsigned short)bytes[i] << 8) | bytes[i + 1];
    }

    if (length & 1) {
        sum += (unsigned short)bytes[length - 1] << 8;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (unsigned short)(~sum & 0xFFFF);
}

int traceroute(const char* hostname, int max_hops, int timeout_ms) {

    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_RAW;
    hints.ai_protocol = IPPROTO_ICMP;

    struct addrinfo* result = NULL;
    int status = getaddrinfo(hostname, NULL, &hints, &result);
    if (status != 0) {
        printf("ERROR: getaddrinfo() failed for %s: %d\n", hostname, status);
        return 1;
    }

    struct sockaddr_in target = {0};
    memcpy(&target, result->ai_addr, sizeof(target));
    freeaddrinfo(result);

    SOCKET sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock == INVALID_SOCKET) {
        int error = WSAGetLastError();
        printf("ERROR: socket() failed for ICMP traceroute: %d\n", error);
        return 1;
    }

    int rcv_timeout = timeout_ms;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&rcv_timeout, sizeof(rcv_timeout)) == SOCKET_ERROR) {
        printf("ERROR: setsockopt(SO_RCVTIMEO) failed: %d\n", WSAGetLastError());
        closesocket(sock);
        return 1;
    }

    unsigned short process_id = (unsigned short)(rand() & 0xFFFF);

    printf("Tracing route to %s over a maximum of %d hops\n\n", hostname, max_hops);

    for (int ttl = 1; ttl <= max_hops; ++ttl) {
        int hop_ttl = ttl;
        if (setsockopt(sock, IPPROTO_IP, IP_TTL, (const char*)&hop_ttl, sizeof(hop_ttl)) == SOCKET_ERROR) {
            printf("ERROR: setsockopt(IP_TTL) failed: %d\n", WSAGetLastError());
            break;
        }


        ICMP_HEADER request = {0};
        request.type = 8; // ICMP Echo Request
        request.code = 0;
        request.id = (unsigned short)process_id;
        request.sequence = (unsigned short)ttl;
        request.checksum = checksum_icmp(&request, sizeof(request));

        clock_t start = clock();
        int sent = sendto(sock, (const char*)&request, sizeof(request), 0, (const struct sockaddr*)&target, sizeof(target));
        if (sent == SOCKET_ERROR) {
            printf("%d  sendto() failed: %d\n", ttl, WSAGetLastError());
            break;
        }

        char buffer[512] = {0};
        struct sockaddr_in from = {0};
        int from_len = sizeof(from);

        int bytes = recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr*)&from, &from_len);
        long elapsed = (clock() - start) * 1000 / CLOCKS_PER_SEC;

        if (bytes == SOCKET_ERROR) {
            printf("%d  *\t\tRequest timed out\n", ttl);
            printf("ERROR: %s\n", gai_strerror(WSAGetLastError()));
            continue;
        }

        if (bytes < 28) {
            printf("%d  *\t\tMalformed ICMP reply\n", ttl);
            continue;
        }

        unsigned int ip_header_length = (unsigned int)(buffer[0] & 0x0F) * 4;
        if (ip_header_length < 20 || bytes < (int)(ip_header_length + sizeof(ICMP_HEADER))) {
            printf("%d  *\t\tIncomplete ICMP reply\n", ttl);
            continue;
        }

        ICMP_HEADER* reply = (ICMP_HEADER*)(buffer + ip_header_length);
        char from_ip[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &from.sin_addr, from_ip, sizeof(from_ip));

        if (reply->type == 11) {
            printf("%d  %s\t%ld ms\t(TTL expired)\n", ttl, from_ip, elapsed);
            continue;
        }

        if (reply->type == 0 && reply->id == request.id && reply->sequence == request.sequence) {
            printf("%d  %s\t%ld ms\t(destination reached)\n", ttl, from_ip, elapsed);
            closesocket(sock);
            return 0;
        }

        printf("%d  %s\t%ld ms\t(ICMP type %u)\n", ttl, from_ip, elapsed, reply->type);
    }

    closesocket(sock);
    printf("Trace complete.\n");
    return 1;
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
        printf("       %s traceroute <hostname> [max_hops] [timeout_ms]\n", argv[0]);
        return 1;
    }

    char* host = argv[1];
    char* port = argc > 2 ? argv[2] : "80"; // plaintext HTTP normally uses 80; 443 is HTTPS and requires TLS/SSL.
    printf("host: %s\n", host);
    printf("port: %s\n", port);

    WSADATA wsa_data = {0};
    if (WSAStartup(MAKEWORD(2,2), &wsa_data)) {
        printf("ERROR: WSAStartup\n");
        return 1;
    }


    {
        int max_hops = 30;
        int timeout_ms = 3000;
        int traceroute_result = traceroute(host, max_hops, timeout_ms);
        return traceroute_result;
    }

    struct addrinfo hints = {
        .ai_flags    = AI_CANONNAME,
        .ai_family   = AF_UNSPEC,   // IPv4 or IPv6
        .ai_socktype = SOCK_STREAM, // TCP
        .ai_protocol = IPPROTO_TCP,
    };

    struct addrinfo* addr;
    if (getaddrinfo(host, port, &hints, &addr)) {
        printf("ERROR: getaddrinfo\n");
        WSACleanup();
        return 2;
    }

    print_addresses(addr);
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
            host);

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