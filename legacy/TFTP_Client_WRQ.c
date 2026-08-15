/* TFTP WRQ client */ 
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

#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5
#define MODE "octet"

void send_wrq(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, const char *filename) {
	char buffer[BUFFER_SIZE];
	int len;

	buffer[0] = 0;
	buffer[1] = WRQ;

	strcpy(&buffer[2], filename);
        len = 2 + strlen(filename) + 1;
	
	strcpy(&buffer[len], MODE);
	len += strlen(MODE) + 1;
	
	sendto(sockfd, buffer, len, 0, (struct sockaddr *)server_addr, addr_len);
	printf("WRQ Sent to server for file: %s\n", filename);
}

void send_data(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, 
	       int block,  char *data, int size) {
	char packet[BUFFER_SIZE];
   	packet[0] = 0;
	packet[1] = DATA;
	packet[2] = (block >> 8) & 0xff;
	packet[3] = block & 0xff;
	memcpy(packet + 4, data, size);

	sendto(sockfd, packet, size + 4, 0, (struct sockaddr *)server_addr, addr_len);
}

int main() {
	int sockfd;
	struct sockaddr_in servaddr;
	socklen_t addr_len = sizeof(servaddr);
	char buffer[BUFFER_SIZE], filename[1000];
	FILE *fp;

	//Create socket
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	//set server address
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	servaddr.sin_addr.s_addr = inet_addr(SERVER_IP);

	//Get filename
	printf("Enter the filename to upload: ");
	scanf("%s", filename);

	fp = fopen(filename, "rb");
	if(!fp){
		perror("File open error");
		close(sockfd);
		return -1;
	}

	//Send wrq
	send_wrq(sockfd, &servaddr, addr_len, filename);

	//wait for ACK(0)
	int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&servaddr, &addr_len);
	if(n < 4 || buffer[1] != ACK || buffer[3] != 0) {
		printf("Server did not ACK WRQ. Exiting....\n");
		fclose(fp);
		close(sockfd);
		return -1;
	}

	//Begin sending data
	int block = 1;
	size_t bytesRead;
	while((bytesRead = fread(buffer, 1, BLOCK_SIZE, fp)) > 0 ) {
		send_data(sockfd, &servaddr, addr_len, block, buffer, bytesRead);

		n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &servaddr, &addr_len);
		if (n < 4 || buffer[1] != ACK || ((buffer[2] << 8) | buffer[3]) != block) {
			printf("Failed to receive ACK for block %d\n", block);
			break;
		}

		printf("Block %d uploaded (%ld bytes)\n", block, bytesRead);

		if(bytesRead < BLOCK_SIZE) break; //last block
		block++;

	} 

	fclose(fp);
	close(sockfd);
	printf("Upload completed.\n");
	return 0;
}