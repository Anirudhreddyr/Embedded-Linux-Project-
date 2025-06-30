/* TFTP UDP Clinet code */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 69 		         //Default TFTP Port
#define SERVER_IP "127.0.0.1"   //Option change if server is remote
#define BUFFER_SIZE 516         //512bytes data + 4 bytes header
#define RRQ 1
#define DATA 3
#define ACK 4
#define MODE "octet"	        //Binary mode


void send_rrq(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, const char *filename){
	char buffer[BUFFER_SIZE];
	int len;

	buffer[0] = 0;
	buffer[1] = RRQ;

	strcpy(&buffer[2], filename);
	len = 2 + strlen(filename) + 1;

	strcpy(&buffer[len], MODE);
	len += strlen(MODE) + 1;

	sendto(sockfd, buffer, len, 0, (struct sockaddr *)server_addr, addr_len);
	printf("RRQ sent file: %s\n", filename);

	}

void send_ack(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, int block_num){
	char ack[4];
	ack[0] = 0;
	ack[1] = ACK;
	ack[2] = (block_num >> 8) & 0XFF;
	ack[3] = block_num & 0XFF;
	sendto(sockfd, ack, 4, 0, (struct sockaddr *)server_addr, addr_len);
	}


int main(){
	int sockfd;
	struct sockaddr_in servaddr;
	char buffer[BUFFER_SIZE];
	char filename[100];
	FILE *fp;
	socklen_t addr_len = sizeof(servaddr);


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

	/* Get filename from user */
	printf(" Enter filename to download: ");
	scanf("%s", filename);
	
	fp = fopen(filename, "wb");
	if(!fp) {
	   perror("File open Error");
	   close(sockfd);
	   return -1;
	}

	/* send RRQ */
	send_rrq(sockfd, &servaddr, addr_len, filename);
	

	/* Receive data loop */
	int expected_block = 1;
	while(1) {
		int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&servaddr, &addr_len);
		if (n < 0) {
		    perror("Receive failed");
		    break;
		 }

	int opcode = buffer[1];
	int block = (buffer[2] << 8) | buffer[3];

	if (opcode = DATA && block == expected_block) {
		fwrite(buffer + 4, 1, n - 4, fp);
		send_ack(sockfd, &servaddr, addr_len, block);
		printf(" Received block %d (%d bytes)\n", block, n - 4);

		if(n < BUFFER_SIZE) {
			printf("File transfer complete.\n");
			break;
		}
		expected_block++;
	} else {
	     printf("Unexpected packet. opcode: %d Block: %d\n", opcode, block);
	 }
    }

	fclose(fp);
	close(sockfd);
	return 0;
}