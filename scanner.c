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

void scan_target(const char* ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return;
    }

    struct sockaddr_in target;
    if (!build_target(ip, port, &target)) {
        fprintf(stderr, "Invalid IPv4 address.\n");
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr* )&target, sizeof(target)) == 0) {
        printf("Port %d is OPEN\n", port);
    }
    else {
        switch (errno) {
            case ECONNREFUSED: printf("Port %d is CLOSED\n", port); break;
            case ETIMEDOUT: printf("Port %d filtered (timeout)\n", port); break;
            case EHOSTUNREACH: printf("Port %d: host unreachable\n", port); break;
            case ENETUNREACH: printf("Port %d: network unreachable\n", port); break;
        }
    }

    close(sock);
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <ip> <start_port> <end_port> \n", argv[0]);
        return 1;
    }

    const char* ip_address = argv[1];

    int start_port, end_port;
    if (!parse_port(argv[2], &start_port) || !parse_port(argv[3], &end_port)) {
        fprintf(stderr, "Invalid port range: %s-%s\n", argv[2], argv[3]);
        return 1;
    }

    if (start_port > end_port) {
        fprintf(stderr, "Start port must be <= end port\n");
        return 1;
    }

    printf("Scanning %s, port %d-%d\n", ip_address, start_port, end_port);

    for (int port = start_port; port < end_port; port++) {
        scan_target(ip_address, port);
    }

    return 0;
}
