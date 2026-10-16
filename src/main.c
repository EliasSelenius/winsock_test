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



typedef struct addrinfo addrinfo;
typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr_in6 sockaddr_in6;
typedef struct sockaddr_storage sockaddr_storage;

// typedef struct sockaddr_storage {
//     uint16_t ss_family;   // address family
//     char __ss_pad1[128 - 2];
// } sockaddr_storage;

void* addrptr(sockaddr* addr) {
    switch (addr->sa_family) {
        case AF_INET:  return (void*)&((sockaddr_in*)addr)->sin_addr;
        case AF_INET6: return (void*)&((sockaddr_in6*)addr)->sin6_addr;
    }

    return 0;
}

char* get_address_string(sockaddr* addr, char* buffer, size_t buffer_size) {
    switch (addr->sa_family) {
        case AF_INET:  break;
        case AF_INET6: break;
        default: return 0;
    }

    char* ret = (char*)inet_ntop(addr->sa_family, addrptr(addr), buffer, buffer_size);
    if (ret == 0) {
        printf("ERROR: %s\n", gai_strerror(WSAGetLastError()));
    }

    return ret;
}


// // IPv6 Internet address (RFC 2553)
// // This is an 'on-wire' format structure.
// typedef struct in6_addr {
//     union {
//         uint8_t  Byte[16];
//         uint16_t Word[8];
//     } u;
// } in6_addr;

// typedef struct sockaddr_in6 {
//     uint16_t sin6_family;    // AF_INET6.
//     uint16_t sin6_port;      // Transport level port number.
//     uint32_t sin6_flowinfo;  // IPv6 flow information.
//     in6_addr sin6_addr;      // IPv6 address.
//     union {
//         uint32_t sin6_scope_id;     // Set of interfaces for a scope.
//         // SCOPE_ID sin6_scope_struct;
//     };
// } sockaddr_in6;

// // IPv4 Internet address
// // This is an 'on-wire' format structure.
// typedef struct in_addr {
//     union {
//         uint8_t s_b1, s_b2, s_b3, s_b4;
//         uint16_t s_w1, s_w2;
//         uint32_t S_addr;
//     } S_un;
// } in_addr;

// // IPv4 Socket address, Internet style
// typedef struct sockaddr_in {
//     uint16_t sin_family;
//     uint16_t sin_port;
//     in_addr  sin_addr;
//     char     sin_zero[8];
// } sockaddr_in;

// // Structure used to store most addresses.
// typedef struct sockaddr {
//     uint16_t sa_family;  // Address family.
//     char sa_data[14];    // Up to 14 bytes of direct address.
// } sockaddr;

// // Structure used in getaddrinfo() call
// typedef struct addrinfo {
//     int                 ai_flags;       // AI_PASSIVE, AI_CANONNAME, AI_NUMERICHOST
//     int                 ai_family;      // PF_xxx
//     int                 ai_socktype;    // SOCK_xxx
//     int                 ai_protocol;    // 0 or IPPROTO_xxx for IPv4 and IPv6
//     size_t              ai_addrlen;     // Length of ai_addr
//     char*               ai_canonname;   // Canonical name for nodename
//     struct sockaddr*    ai_addr;        // Binary address
//     struct addrinfo*    ai_next;        // Next structure in linked list
// } addrinfo;


// void* bump_alloc(void* buffer, )


char* socktype_str(int ty) {
    switch (ty) {
        case SOCK_STREAM:    return "SOCK_STREAM";
        case SOCK_DGRAM:     return "SOCK_DGRAM";
        case SOCK_RAW:       return "SOCK_RAW";
        case SOCK_RDM:       return "SOCK_RDM";
        case SOCK_SEQPACKET: return "SOCK_SEQPACKET";
    }

    return 0;
};

char* family(int f) {
    switch (f) {
        case AF_UNSPEC: return "AF_UNSPEC";
        case AF_INET:   return "AF_INET";
        case AF_INET6:  return "AF_INET6";
    }
    return 0;
}

void print_addresses(addrinfo* addr) {
    printf("\t\tfamily:\tsocktype:\tprotocol:\n");

    int count = 0;
    for (; addr; addr = addr->ai_next, count+=1) {
        char node_name[128];
        char serv_name[64];
        int flags = NI_NUMERICHOST | NI_NUMERICSERV;

        int ret = getnameinfo(addr->ai_addr, addr->ai_addrlen, node_name, sizeof(node_name), serv_name, sizeof(serv_name), flags);
        if (ret != 0) {
            int error = WSAGetLastError();
            char* info = gai_strerror(error);
            printf("ERROR: getnameinfo() returned %d '%s'.\n", error, info);
            continue;
        }

        char* ip_version = 0;
        switch (addr->ai_family) {
            case AF_INET:  ip_version = "IPv4"; break;
            case AF_INET6: ip_version = "IPv6"; break;
        }

        printf("\t%d.\t%d:%s\t%d:%s\t%d:%s\t(%zubytes) %s\t%s\n",
            count,
            addr->ai_family, family(addr->ai_family),
            addr->ai_socktype, socktype_str(addr->ai_socktype),
            addr->ai_protocol, "",
            addr->ai_addrlen,
            node_name,
            serv_name
        );
    }

    printf("\tPrinted %d addresses.\n", count);
}

addrinfo* resolve_host(char* host_or_ip, char* service_or_port) {
    addrinfo hints = {
        .ai_flags    = 0, // AI_CANONNAME,
        .ai_family   = AF_INET, // AF_UNSPEC,   // IPv4 or IPv6
        // .ai_socktype = SOCK_STREAM, // TCP
        // .ai_protocol = IPPROTO_TCP,
        // .ai_protocol = IPPROTO_ICMP,
    };

    addrinfo* addr = 0;
    if (getaddrinfo(host_or_ip, service_or_port, &hints, &addr)) {
        int error = WSAGetLastError();
        char* info = gai_strerror(error);
        printf("ERROR: getaddrinfo() returned %d '%s'.\n", error, info);
        return 0;
    }

    return addr;
}

SOCKET socket_connect(addrinfo* addr) {
    for (;addr; addr = addr->ai_next) {

        SOCKET sok = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sok == INVALID_SOCKET) {
            int error = WSAGetLastError();
            char* info = gai_strerror(error);
            printf("ERROR: socket(): %d '%s'.\n", error, info);
            continue;
        }

        int status = connect(sok, addr->ai_addr, (int)addr->ai_addrlen);
        if (status != SOCKET_ERROR) {
            return sok;
        }

        int error = WSAGetLastError();
        char* info = gai_strerror(error);
        printf("ERROR: connect() returned SOCKET_ERROR. %d - %s\n", error, info);
        closesocket(sok);
    }

    return INVALID_SOCKET;
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

uint16_t checksum_icmp(void* data, size_t length) {
    uint8_t* bytes = (uint8_t*)data;
    uint32_t sum = 0;

    for (size_t i = 0; i + 1 < length; i += 2) {
        sum += ((uint16_t)bytes[i] << 8) | bytes[i + 1];
    }

    if (length & 1) {
        sum += (uint16_t)bytes[length - 1] << 8;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (uint16_t)(~sum & 0xFFFF);
}


int traceroute(char* hostname, int max_hops, int timeout_ms) {

    // addrinfo hints = {0};
    // hints.ai_family = AF_INET;
    // hints.ai_socktype = SOCK_RAW;
    // hints.ai_protocol = IPPROTO_ICMP;

    // addrinfo* result = NULL;
    // int status = getaddrinfo(hostname, NULL, &hints, &result);
    // if (status != 0) {
    //     printf("ERROR: getaddrinfo() failed for %s: %d\n", hostname, status);
    //     return 1;
    // }

    addrinfo* result = resolve_host(hostname, 0);
    print_addresses(result);

    sockaddr_storage target = {0};
    memcpy(&target, result->ai_addr, result->ai_addrlen);
    freeaddrinfo(result);

    SOCKET sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock == INVALID_SOCKET) {
        int error = WSAGetLastError();
        char* info = gai_strerror(error);
        printf("ERROR: socket(): %d '%s'.\n", error, info);
        return 1;
    }

    int rcv_timeout = timeout_ms;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&rcv_timeout, sizeof(rcv_timeout)) == SOCKET_ERROR) {
        printf("ERROR: setsockopt(SO_RCVTIMEO) failed: %d\n", WSAGetLastError());
        closesocket(sock);
        return 1;
    }

    uint16_t process_id = (uint16_t)(rand() & 0xFFFF);

    printf("Tracing route to %s over a maximum of %d hops\n\n", hostname, max_hops);

    for (int ttl = 1; ttl <= max_hops; ++ttl) {
        int hop_ttl = ttl;
        if (setsockopt(sock, IPPROTO_IP, IP_TTL, (const char*)&hop_ttl, sizeof(hop_ttl)) == SOCKET_ERROR) {
            printf("ERROR: setsockopt(IP_TTL) failed: %d\n", WSAGetLastError());
            break;
        }


        Icmp_Header request = {0};
        request.type = 8; // ICMP Echo Request
        request.code = 0;
        request.id = (uint16_t)process_id;
        request.sequence = (uint16_t)ttl;
        request.checksum = checksum_icmp(&request, sizeof(request));

        clock_t start = clock();
        int sent = sendto(sock, &request, sizeof(request), 0, &target, sizeof(target));
        if (sent == SOCKET_ERROR) {
            int error = WSAGetLastError();
            char* info = gai_strerror(error);
            printf("ERROR: sendto(): %d '%s'.\n", error, info);
            break;
        }

        char buffer[512] = {0};
        sockaddr_storage from = {0};
        int from_len = sizeof(from);

        int bytes = recvfrom(sock, buffer, sizeof(buffer), 0, &from, &from_len);
        long elapsed = (clock() - start) * 1000 / CLOCKS_PER_SEC;

        if (bytes == SOCKET_ERROR) {
            printf("%d  *\t\tRequest timed out\n", ttl);

            int error = WSAGetLastError();
            char* info = gai_strerror(error);
            printf("ERROR: recvfrom(): %d '%s'.\n", error, info);
            continue;
        }

        char from_ip[INET6_ADDRSTRLEN] = {0};
        get_address_string(&from, from_ip, sizeof(from_ip));
        printf("%s\n", from_ip);

        if (bytes < 28) {
            printf("%d  *\t\tMalformed ICMP reply\n", ttl);
            continue;
        }

        uint32_t ip_header_length = (uint32_t)(buffer[0] & 0x0F) * 4;
        if (ip_header_length < 20 || bytes < (int)(ip_header_length + sizeof(Icmp_Header))) {
            printf("%d  *\t\tIncomplete ICMP reply\n", ttl);
            continue;
        }

        Icmp_Header* reply = (Icmp_Header*)(buffer + ip_header_length);

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

int main(int argc, char** argv) {
    printf("Hello Winsock\n");

    if (argc < 2) {
        printf("Usage: %s <hostname> [port]\n", argv[0]);
        return 1;
    }

    char* host = argv[1];
    char* port = argc > 2 ? argv[2] : 0;
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
        traceroute(host, max_hops, timeout_ms);
        return 0;
    }


    addrinfo* addr = resolve_host(host, port);
    if (!addr) {
        return 0;
    }

    print_addresses(addr);
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
            char* info = gai_strerror(error);
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
            char* info = gai_strerror(error);
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