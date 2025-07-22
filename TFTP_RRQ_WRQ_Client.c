/* TFTP UDP Client RRQ + WRQ */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 69
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 516
#define BLOCK_SIZE 512
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5
#define MODE "octet"

void send_rrq_or_wrq(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, const char *filename, int opcode) {
    char buffer[BUFFER_SIZE];
    int len;

    buffer[0] = 0;
    buffer[1] = opcode;

    strcpy(&buffer[2], filename);
    len = 2 + strlen(filename) + 1;

    strcpy(&buffer[len], MODE);
    len += strlen(MODE) + 1;

    sendto(sockfd, buffer, len, 0, (struct sockaddr *)server_addr, addr_len);

    if (opcode == RRQ)
        printf("RRQ sent for file: %s\n", filename);
    else
        printf("WRQ sent for file: %s\n", filename);
}

void send_ack(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, int block_num) {
    char ack[4];
    ack[0] = 0;
    ack[1] = ACK;
    ack[2] = (block_num >> 8) & 0xFF;
    ack[3] = block_num & 0xFF;
    sendto(sockfd, ack, 4, 0, (struct sockaddr *)server_addr, addr_len);
}

void handle_rrq(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, const char *filename) {
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        perror("File open error");
        return;
    }

    send_rrq_or_wrq(sockfd, server_addr, addr_len, filename, RRQ);

    int expected_block = 1;
    char buffer[BUFFER_SIZE];

    while (1) {
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)server_addr, &addr_len);
        if (n < 0) {
            perror("Receive failed");
            break;
        }

        int opcode = buffer[1];

        if (opcode == ERROR) {
            int error_code = (buffer[2] << 8) | buffer[3];
            printf("Server error (code %d): %s\n", error_code, buffer + 4);
            break;
        }

        int block = (buffer[2] << 8) | buffer[3];

        if (opcode == DATA && block == expected_block) {
            fwrite(buffer + 4, 1, n - 4, fp);
            send_ack(sockfd, server_addr, addr_len, block);
            printf("Received block %d (%d bytes)\n", block, n - 4);

            if (n < BUFFER_SIZE) {
                printf("File download complete.\n");
                break;
            }
            expected_block++;
        } else {
            printf("Unexpected packet. (opcode: %d Block: %d)\n", opcode, block);
        }
    }

    fclose(fp);
}

void handle_wrq(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("File open error");
        return;
    }

    send_rrq_or_wrq(sockfd, server_addr, addr_len, filename, WRQ);

    char buffer[BUFFER_SIZE];
    int block = 0;

    // Wait for ACK 0
    int ack_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)server_addr, &addr_len);
    if (ack_len < 4 || buffer[1] != ACK || ((buffer[2] << 8) | buffer[3]) != 0) {
        printf("Did not receive expected ACK 0. Aborting upload.\n");
        fclose(fp);
        return;
    }

    printf("Received ACK 0. Starting file upload...\n");

    size_t bytes_read;
    do {
        bytes_read = fread(buffer + 4, 1, BLOCK_SIZE, fp);
        block++;
        buffer[0] = 0;
        buffer[1] = DATA;
        buffer[2] = (block >> 8) & 0xFF;
        buffer[3] = block & 0xFF;

        sendto(sockfd, buffer, bytes_read + 4, 0, (struct sockaddr *)server_addr, addr_len);
        printf("Sent block %d (%ld bytes)\n", block, bytes_read);

        // Wait for ACK
        ack_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)server_addr, &addr_len);
        if (ack_len >= 4 && buffer[1] == ACK) {
            int ack_block = (buffer[2] << 8) | buffer[3];
            if (ack_block != block) {
                printf("Unexpected ACK block: %d (expected %d)\n", ack_block, block);
                break;
            }
        } else {
            printf("ACK timeout or error.\n");
            break;
        }
    } while (bytes_read == BLOCK_SIZE);

    printf("File upload complete.\n");
    fclose(fp);
}

int main() {
    int sockfd;
    struct sockaddr_in servaddr;
    char filename[100];
    socklen_t addr_len = sizeof(servaddr);
    int choice;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    printf("1. Download (RRQ)\n2. Upload (WRQ)\nChoose option: ");
    scanf("%d", &choice);
    printf("Enter filename: ");
    scanf("%s", filename);

    if (choice == 1) {
        handle_rrq(sockfd, &servaddr, addr_len, filename);
    } else if (choice == 2) {
        handle_wrq(sockfd, &servaddr, addr_len, filename);
    } else {
        printf("Invalid choice.\n");
    }

    close(sockfd);
    return 0;
}