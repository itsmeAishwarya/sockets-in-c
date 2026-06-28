#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUFFER_SIZE 4096

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: ./rawhttp <hostname>\n");
        printf("Example: ./rawhttp example.com\n");
        return 1;
    }

    char *hostname = argv[1];
    char request[BUFFER_SIZE];
    char full_response[BUFFER_SIZE * 8];
    int sock_fd;

    // Step 1: Resolve hostname to IP
    struct hostent *server = gethostbyname(hostname);
    if (server == NULL) {
        printf("Error: could not resolve host '%s'\n", hostname);
        return 1;
    }
    printf("Resolved %s → %s\n", hostname,
           inet_ntoa(*(struct in_addr*)server->h_addr));

    // Step 2: Create TCP socket
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0) { perror("socket"); return 1; }

    // Step 3: Connect to port 80
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(80);
    memcpy(&server_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    if (connect(sock_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        return 1;
    }
    printf("Connected to %s:80\n", hostname);

    // Step 4: Build HTTP GET request by hand
    snprintf(request, sizeof(request),
        "GET / HTTP/1.1\r\n"
        "Host: %s\r\n"
        "Connection: close\r\n"
        "\r\n",
        hostname);

    printf("\n--- SENDING REQUEST ---\n%s", request);

    // Step 5: Send it
    send(sock_fd, request, strlen(request), 0);

    // Step 6: Read full response into buffer
    int total = 0, bytes;
    while ((bytes = recv(sock_fd, full_response + total,
                         sizeof(full_response) - total - 1, 0)) > 0) {
        total += bytes;
    }
    full_response[total] = '\0';

    // Step 7: Split headers and body at \r\n\r\n
    char *header_end = strstr(full_response, "\r\n\r\n");
    if (header_end == NULL) {
        printf("Malformed response\n");
        close(sock_fd);
        return 1;
    }

    *header_end = '\0';
    char *body = header_end + 4;

    // Step 8: Print headers line by line
    printf("\n=== STATUS ===\n");
    char *line = strtok(full_response, "\r\n");
    printf("%s\n", line);

    printf("\n=== HEADERS ===\n");
    while ((line = strtok(NULL, "\r\n")) != NULL) {
        printf("  %s\n", line);
    }

    // Step 9: Print body
    printf("\n=== BODY ===\n%s\n", body);
    printf("=== TOTAL BYTES RECEIVED: %d ===\n", total);

    close(sock_fd);
    return 0;
}