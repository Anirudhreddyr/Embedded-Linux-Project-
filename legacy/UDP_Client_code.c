/* UDP Clinet code */
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

int main(){
	int sockfd;
	struct sockaddr_in servaddr;
	char buffer[BUFFER_SIZE];

	/* Create UDP socket */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0 ){
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	/* configure server address */
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);

	/* Get input from user*/
	printf(" Enter message to send to server: ");
	fgets(buffer, BUFFER_SIZE, stdin);

	/* Send message to server */
	sendto(sockfd, buffer, strlen(buffer), 0,
		(const struct sockaddr *)&servaddr, sizeof(servaddr));

	printf("Message sent to server.\n");

	int len = sizeof(servaddr);
	int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
			(struct sockaddr *)&servaddr, &len);

	buffer[n] = '\0';
	printf("Server response: %s\n", buffer);

	/* close socket */
	close(sockfd);
	return 0;
}
