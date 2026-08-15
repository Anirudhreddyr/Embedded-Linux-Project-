/* TFTP RRQ + WRQ Server code */
#include <stdio.h>        //Standard I/O functions
#include <stdlib.h>       //General utilities (exit, malloc, free,etc)
#include <string.h>       //String operations (memset, strcpy, etc)
#include <unistd.h>       //Required for close()
#include <sys/socket.h>   //Core socket functions (socket, bind, recvfrom, etc)
#include <netinet/in.h>   //Structers for internet domain addresses (sockaddr_in, INADDR_ANY, etc)
#include <arpa/inet.h>    //Functions to convert addresses (inet_ntoa, inet_addr, etc)
#include <errno.h>        

#define PORT 69          //Defining the port your server will listen on
#define BUFFER_SIZE 516  //4 Bytes for TFTP + 512 bytes for Data
#define BLOCK_SIZE 512

//TFTP OPCODES
#define RRQ   1
#define WRQ   2
#define DATA  3
#define ACK   4
#define ERROR 5


/* send TFTP ack */
void send_ack(int sockfd, struct sockaddr_in *cli_addr, socklen_t addr_len, int block_num) {
	char ack[4];
	ack[0] = 0 ;
	ack[1] = ACK;
	ack[2] = (block_num >> 8) & 0XFF;
	ack[3] = block_num & 0XFF;
	sendto(sockfd, ack, 4, 0, (const struct sockaddr_in *)cli_addr, addr_len);
 }


/*Send TFTP error Packet*/
void send_error(int sockfd, struct sockaddr_in *client_addr, socklen_t len, int error_code,  const char* message){
	char buffer[BUFFER_SIZE];
	int message_len = strlen(message);

	buffer[0] = 0;
	buffer[1] = ERROR;
	buffer[2] = 0;
	buffer[3] = error_code;
	strcpy(buffer + 4, message);
	buffer[4 + message_len] = 0;
	sendto(sockfd, buffer, 5 + message_len, 0, (struct sockaddr *)client_addr, len);
 }


void handle_rrq(int sockfd, struct sockaddr_in *client_addr, socklen_t addr_len, char *filename) {
	FILE *fp = fopen(filename, "rb");
	if (!fp) {
		send_error(sockfd, client_addr, addr_len, 1, "File not found");
		return;
	}

	char buffer[BUFFER_SIZE];
	int block = 1;
	size_t bytesRead;

	do {
		bytesRead = fread(buffer + 4, 1, BLOCK_SIZE, fp);
		buffer[0] = 0;
		buffer[1] = DATA;
		buffer[2] = (block >> 8) & 0xFF;
		buffer[3] = block & 0xFF;

		int retries = 0;
		const int MAX_RETRIES = 5;

	resend: 
		sendto(sockfd, buffer, bytesRead + 4, 0, (struct sockaddr *)client_addr, addr_len);

	char ack_buf[BUFFER_SIZE]; 
	int ack_len = recvfrom(sockfd, ack_buf, BUFFER_SIZE, 0, (struct sockaddr *)client_addr, &addr_len);
	if (ack_len >= 4 && ack_buf[1] == ACK) {
		int ack_block =  (ack_buf[2] << 8) | ack_buf[3];
		if(ack_block == block) { 
		       printf("RRQ: Sent block %d (%ld bytes)\n", block, bytesRead);
		       block++;	
		       continue;
		}
	}

	if (ack_len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
		if (++retries <= MAX_RETRIES) {
			printf("RRQ: Timeout, retrying block %d...\n", block);
			goto resend;
		} else {
			printf("RRQ: Max retries reached. Aborting transfer.\n");
			break;

		}
	}
	} while (bytesRead == BLOCK_SIZE);

	fclose(fp);

}

void handle_wrq(int sockfd, struct sockaddr_in *client_addr, socklen_t addr_len, char *filename){
	FILE *fp = fopen(filename, "wb");
	if (!fp){
		send_error(sockfd, client_addr, addr_len, 2, "Cannot create file");
		return;
	}

	send_ack(sockfd, client_addr, addr_len, 0); //ACK block 0

	int expected_block = 1;
	char buffer[BUFFER_SIZE];
	
	while(1){
		int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) client_addr, &addr_len);
		if(n < 0) {
			perror("WRQ: recvfrom failed");
			break;
		}

		int opcode = (buffer[0] << 8) | buffer[1];
		int block = (buffer[2] << 8) | buffer[3];

		if(opcode == DATA && block == expected_block) {
			fwrite(buffer + 4, 1, n - 4, fp);
			fflush(fp);
			send_ack(sockfd, client_addr, addr_len, block);
			printf("WRQ: Received block %d (%d bytes)\n", block, n - 4);

			if (n < BUFFER_SIZE) {
				printf("WRQ: Upload complete.\n");
				break;
			}
			expected_block++;
		} else {
			printf("WRQ: Unexpected packet. (opcode=%d block=%d)\n", opcode, block);
		}
	}

	fclose(fp);
}

			

int main(){
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;
	char buffer[BUFFER_SIZE];
	socklen_t addr_len = sizeof(cliaddr);

	/*create Socket*/
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}
	
	/* set timeout for recviveing ACKs */
	struct timeval timeout;
	timeout.tv_sec = 2;  	 //2secs timeout
	timeout.tv_usec = 0;
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));


	/* Bind sever to port 69 */ 
	memset(&servaddr, 0, sizeof(servaddr));
	//memset(&cliaddr, 0, sizeof(cliaddr));

	servaddr.sin_family = AF_INET;		//IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;	//Bind to all interfaces
	servaddr.sin_port = htons(PORT);	//Port number

	/* Bind the socket with the server address*/
	if(bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("Bind failed");
		close(sockfd);
		exit(EXIT_FAILURE);
			
	}

	printf("TFTP Server is (RRQ + WRQ) running on port %d...\n", PORT);

	/* Receive messagesin a loop */
	while (1) { 
		printf("\nWaiting for RRQ/WRQ...\n");
	
		int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
				(struct sockaddr *)&cliaddr, &addr_len);
		if(n < 0){
			perror("recvform failed");
			continue;
		}

		/* Check Opcode */
		if(buffer[0] == 0) {
			char *filename = buffer + 2;
			char *mode = filename + strlen(filename) + 1;

			switch(buffer[1]) {
				case RRQ:
					printf("RRQ Received for file: %s\n", filename);
				       	handle_rrq(sockfd, &cliaddr, addr_len, filename);
					break;
				case WRQ:
					printf("WRQ Received for file: %s\n", filename);
					handle_wrq(sockfd, &cliaddr, addr_len, filename);
					break;
				default:
					send_error(sockfd, &cliaddr, addr_len, 4,"Illegal TFTP Operation");
					printf("Unknown request received. Ignored.\n");
					break;
			}
		}
	}

	close(sockfd);
	return 0;
}