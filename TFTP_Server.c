/* TFTP Server code */
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
#define RRQ 1
#define DATA 3
#define ACK 4
#define ERROR 5

/*Send TFTP error Packet*/
void send_error(int sockfd, struct sockaddr_in *client_addr, socklen_t len, int error_code,  const char* message){
	char buffer[BUFFER_SIZE];
	int message_len = strlen(message);

	buffer[0] = 0;
	buffer[1] = ERROR;
	buffer[2] = (error_code >> 8) & 0xFF;
	buffer[3] = error_code & 0xFF;  //File not found
	strcpy(buffer + 4, message);
	buffer[4 + message_len] = 0;

	    sendto(sockfd, buffer, 5 + message_len, 0, (struct sockaddr *)client_addr, len);
	}


int main(){
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;
	char buffer[BUFFER_SIZE];
	socklen_t len;

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
	if(bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("Bind failed");
		close(sockfd);
		exit(EXIT_FAILURE);
			
	}

	printf("TFTP Server is running on port %d...\n", PORT);

	/* Receive messagesin a loop */
	while (1) { 
		printf("\nWaiting for RRQ...\n");
		len = sizeof(cliaddr);		//Reset client length
	        
		int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
				(struct sockaddr *)&cliaddr, &len);
		if(n < 0){
			perror("recvform failed");
			continue;
		}

		/* Check Opcode */
		if(buffer[0] == 0 && buffer[1] == RRQ){
			
			/* Extract filename and mode*/
			char *filename = buffer + 2;
			char *mode = filename + strlen(filename) + 1;
			

			char client_ip[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(cliaddr.sin_addr), client_ip, INET_ADDRSTRLEN);
			printf("RRQ from %s:%d\n", client_ip, ntohs(cliaddr.sin_port));
			printf("Client requests file: %s (mode: %s)\n", filename, mode);

		//only support octet mode
		if(strcasecmp(mode, "octet") != 0){
			send_error(sockd, &cliaddr, len, 4, "only octet mode supported");
			continue;
		}

		FILE *fp = fopen(filename, "rb");
		if(!fp){
			send_error(sockfd, &cliaddr, len, 1, "File not found");
			continue;
		}

		int block = 1;
		size_t bytesRead;
		
		do {
			bytesRead = fread(buffer + 4, 1, BLOCK_SIZE, fp);
			buffer[0] = 0;
			buffer[1] = DATA;
			buffer[2] = (block >> 8) & 0XFF;
			buffer[3] = block & 0XFF;
			

			int retries = 0;
			const int MAX_RETRIES = 5;
			
		resend:
			sendto(sockfd, buffer, bytesRead + 4, 0, (struct sockaddr *)&cliaddr, len);
			
			/* Wait for ACK */ 
			char ack_buf[BUFFER_SIZE];
			int ack_len = recvfrom(sockfd, ack_buf, BUFFER_SIZE, 0, (struct sockaddr *)&cliaddr, &len);

			if (ack_len >= 4 && ack_buf[1] == ACK) {
				int ack_block = (ack_buf[2] << 8) | ack_buf[3];
				if (ack_block == block) {
					printf(" Received ACK for block %d", block);
					block++;
					retries = 0;
					continue;
				}

			 }

			if (ack_len < 0) {
				if(errno == EAGAIN || errno == EWOULDBLOCK) {
					if(++retries <= MAX_RETRIES) {
						printf("Timeout waiting for ACK. Retrying block %d..\n", block);
						goto resend;
					} else {
						printf("Max retries reached. Aborting transfer... \n");
						break;
					}
				} else {
					perror("recvfrom ACK failed");
					break;
				}
			}

		} while (bytesRead == BLOCK_SIZE);  //last packet may be < BLOCK_SIZE

			fclose(fp);
			printf("File Transfer completed.\n");
		} else {
			printf("Not valid RRQ packet.\n");
		 }	
	}

	close(sockfd);
	return 0;
 }