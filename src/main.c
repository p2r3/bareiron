#ifdef ESP_PLATFORM
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lwip/sockets.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#elif defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#define PORT 25565
#define BUFFER_SIZE 1024

static char recv_buffer[BUFFER_SIZE];
static int recv_count = 0;

static int server_fd = -1;
static int client_fd = -1;

static void handlePacket(const char *data, int len) {
    if (len < 1) return;

    unsigned char packetId = (unsigned char)data[0];
    printf("Received Packet ID: %u, Length: %d\n", packetId, len);

    switch (packetId) {
        case 0x00: // Handshake
            printf("Handshake packet received.\n");
            break;
        case 0x01: // Status request
            printf("Status request packet received.\n");
            {
                const char *response = "Server is running!";
                send(client_fd, response, (int)strlen(response), 0);
            }
            break;
        case 0x02: // Login start
            printf("Login start packet received.\n");
            break;
        case 0x03: // Keep Alive
            printf("Keep Alive packet received.\n");
            break;
        case 0x04: // Chat message
            printf("Chat message packet received.\n");
            break;
        case 0x05: // Player position
            printf("Player position packet received.\n");
            break;
        default:
            printf("Unknown Packet ID: %u\n", packetId);
            break;
    }
}

int main(void) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup failed.\n");
        return 1;
    }
#endif

    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind failed");
#ifdef _WIN32
        closesocket(server_fd);
        WSACleanup();
#else
        close(server_fd);
#endif
        return 1;
    }

    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
#ifdef _WIN32
        closesocket(server_fd);
        WSACleanup();
#else
        close(server_fd);
#endif
        return 1;
    }

    printf("Server listening on port %d...\n", PORT);

    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_addr_len);
    if (client_fd < 0) {
        perror("accept failed");
#ifdef _WIN32
        closesocket(server_fd);
        WSACleanup();
#else
        close(server_fd);
#endif
        return 1;
    }

    printf("Client connected.\n");

    while (true) {
        recv_count = recv(client_fd, recv_buffer, sizeof(recv_buffer), 0);
        if (recv_count <= 0) {
            printf("Client disconnected.\n");
            break;
        }
        handlePacket(recv_buffer, recv_count);
    }

#ifdef _WIN32
    closesocket(client_fd);
    closesocket(server_fd);
    WSACleanup();
#else
    close(client_fd);
    close(server_fd);
#endif

    return 0;
}
