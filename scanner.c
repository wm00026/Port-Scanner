#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <arpa/inet.h>

#define MIN_PORT 1
#define MAX_PORT 65535


int parse_port(const char* str, int *out_port) {
    char* endptr;
    errno = 0;
    long port_val = strtol(str, &endptr, 10);

    if (endptr == str || *endptr != 0 || 
            errno == ERANGE || port_val < MIN_PORT || port_val > MAX_PORT) {
        return 0;
    }

    *out_port = (int)port_val;
    return 1;
}

int build_target(const char* ip, int port, struct sockaddr_in* target) {
    memset(target, 0, sizeof(*target));

    target->sin_family = AF_INET;
    target->sin_port = htons(port);
    
    return inet_pton(AF_INET, ip, &target->sin_addr) == 1;
}

void check_port(int sock, struct sockaddr_in* target, int port) {
    if (connect(sock, (struct sockaddr *)target, sizeof(*target)) == 0) {
        printf("Port %d is OPEN\n", port);
        return;
    }

    switch (errno) {
        case ECONNREFUSED: printf("Port CLOSED\n"); break;
        case ETIMEDOUT: printf("Filtered (timeout)\n"); break;
        case EHOSTUNREACH: printf("Host unreachable\n"); break;
        case ENETUNREACH: printf("Network unreachable\n"); break;
        default: perror("connect");
    }
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <ip> <port>\n", argv[0]);
        return 1;
    }

    const char* ip_address = argv[1];

    int target_port;
    if (!parse_port(argv[2], &target_port)) {
        fprintf(stderr, "Invalid port: %s\n", argv[2]);
        return 1;
    }

    printf("ip is %s, port is %d\n", ip_address, target_port);

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in target;
    if (!build_target(ip_address, target_port, &target)) {
        fprintf(stderr, "Invalid IPv4 address\n");
        close(sock);
        return 1;
    }

    check_port(sock, &target, target_port);

    close(sock);
    return 0;
}
