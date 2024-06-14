/* UDP client Code */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1"
#define BUFFER_SIZE 1024

int main () {
    int sockfd;
    struct sockaddr_in servaddr;
    char buffer[BUFFER_SIZE];
    char *message = "Hello from client";

    /* create socket file descriptor */
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    /* Filling server information */
    servaddr.sin_family = AF_INET; 
    servaddr.sin_port = htons(PORT);
    servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    /* Send message to server */
    sendto(sockfd, (const char *)message, strlen(message), 0, (const struct sockaddr *) &servaddr, sizeof(servaddr));
    printf(" Message sent. \n");

    /* Receive message from server */
    int n = recvfrom(sockfd, (char *)buffer, BUFFER_SIZE, 0, NULL, NULL);
    if (n < 0) {
        perror(" recvfrom failed ");
        exit(EXIT_FAILURE);
    }
    buffer[n] = '\0'; // Null terminate string
    printf(" Server: %s\n", buffer);

    /* close the socket */
    close(sockfd);
    return 0;
}
