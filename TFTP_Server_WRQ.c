/* TFTP WRQ Server */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define PORT 69
#define BUFFER_SIZE 516
#define BLOCK_SIZE 512

//TFTP Opcodes
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5

void send_ack(int sockfd, struct sockaddr_in *client_addr, socklen_t addr_len, int block_num) {
	char ack[4];
	ack[0] = 0;
	ack[1] = ACK;
	ack[2] = (block_num >> 8) & 0XFF;
	ack[3] = block_num & 0XFF;
	sendto(sockfd, ack, 4, 0, (struct sockaddr *)client_addr, addr_len);
}

void send_error(int sockfd, struct sockaddr_in *client_addr, socklen_t len, const char* message){
	char buffer[BUFFER_SIZE];
	int message_len = strlen(message);
	buffer[0] = 0;
	buffer[1] = ERROR;
	buffer[2] = 0;
	buffer[3] = 1;
	strcpy(buffer + 4, message);
	buffer[4 + message_len] = 0;
	sendto(sockfd, buffer, 5 + message_len, 0, (struct sockaddr *)client_addr, len);
}

int main() {
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t addr_len = sizeof(cliaddr);
	char buffer[BUFFER_SIZE];

	//Create socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Socket creation failed");
		exit(EXIT_FAILURE);
	}

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = INADDR_ANY;
	servaddr.sin_port = htons(PORT);

	if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("bind failed");
		close(sockfd);
		exit(EXIT_FAILURE);
	}

	printf("TFTP Server WRQ mode is running on port %d....\n", PORT);

	while(1) {
		printf("\n Waiting for WRQ...\n");
		errno = 0;
		int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &addr_len);
		if(n < 0) {
			perror("recvfrom failed");
			continue;
		}

		// WRQ Check
		if(buffer[0] == 0 && buffer[1] == WRQ) {
			char *filename = buffer + 2;
			char *mode = filename + strlen(filename) + 1;

			printf("Client wants to upload file: %s (mode: %s)\n", filename, mode);

			FILE *fp = fopen(filename, "wb");
			if (!fp) {
				send_error(sockfd, &cliaddr, addr_len, "Cannot create file");
				continue;
			}

			//Send ACK for Block 0
			send_ack(sockfd, &cliaddr, addr_len, 0);

			int expected_block = 1;
			while(1) {
			      int  n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,  (struct sockaddr *)&cliaddr, &addr_len);
			      if(n < 0) {
			      		perror("recvfrom data failed");
					break;
			      }

			      int opcode = (buffer[0] << 8 )| buffer[1];
			      int block = (buffer[2]  << 8) | buffer[3];

			      if(opcode == DATA && block == expected_block) {
				      fwrite(buffer + 4, 1, n - 4, fp);
				      fflush(fp);                       // to fulsh each block to disk
				      send_ack(sockfd, &cliaddr, addr_len, block);
				      printf("Received block %d (%d bytes)\n", block, n - 4);

				      if (n < BUFFER_SIZE) {
					      printf("File upload complete.\n");
					      break;
				      }
				      expected_block++;
				} else {
				   printf("Unexpected packet received. (opcode: %d block: %d)\n", opcode, block);
				}
			}
		         fclose(fp);
		} else {
			printf("NOt a WRQ packet.\n");
		}
	}

	close(sockfd);
	return 0;
}