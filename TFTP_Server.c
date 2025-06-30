/* TFTP Server code */
#include <stdio.h>        //Standard I/O functions
#include <stdlib.h>       //General utilities (exit, malloc, free,etc)
#include <string.h>       //String operations (memset, strcpy, etc)
#include <unistd.h>       //Required for close()
#include <sys/socket.h>   //Core socket functions (socket, bind, recvfrom, etc)
#include <netinet/in.h>   //Structers for internet domain addresses (sockaddr_in, INADDR_ANY, etc)
#include <arpa/inet.h>    //Functions to convert addresses (inet_ntoa, inet_addr, etc)


#define PORT 69          //Defining the port your server will listen on
#define BUFFER_SIZE 516  //4 Bytes for TFTP + 512 bytes for Data
#define BLOCK_SIZE 512

//TFTP OPCODES
#define RRQ 1
#define DATA 3
#define ERROR 5

/* Send TFTP error Packet */
void send_error(int sockfd, struct sockaddr_in *client_addr, socklen_t len, const char* message){
	char buffer[BUFFER_SIZE];
	int message_len = strlen(message);

	buffer[0] = 0;
	buffer[1] = ERROR;
	buffer[2] = 0;
	buffer[3] = 1;  //File not found
	strcpy(buffer + 4, message);
	buffer[4 + message_len] = 0;

	    sendto(sockfd, buffer, 5 + message_len, 0, (struct sockaddr *)client_addr, len);
	}


int main(){
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;
	char buffer[BUFFER_SIZE];
	socklen_t len;

	/* create Socket */
	if((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	/* set server address structure */
	memset(&servaddr, 0, sizeof(servaddr));
	//memset(&cliaddr, 0, sizeof(cliaddr));

	servaddr.sin_family = AF_INET;		    //IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;	//Bind to all interfaces
	servaddr.sin_port = htons(PORT);	    //Port number

	/* Bind the socket with the server address */
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
			printf("RRQ received.\n");

			char *filename = buffer + 2;
			char *mode = filename + strlen(filename) + 1;
			
			printf("Client requests file: %s (mode: %s)\n", filename, mode);

		FILE *fp = fopen(filename, "rb");
		if(!fp){
			send_error(sockfd, &cliaddr, len, "File not found");
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
			
			sendto(sockfd, buffer, bytesRead + 4, 0, (struct sockaddr *)&cliaddr, len);
			printf("sent block %d (%zu bytes)\n", block, bytesRead);
			block++;

			//For now, no ACK wait implemented
			usleep(100000); //shortly delay to avoid flooding
		} while (bytesRead == BLOCK_SIZE);

		fclose(fp);
		      printf("File transfer completed...\n");
		}else {
			printf("Unsupported request or invalid opcode.\n");
		}
	}

	/* But good practice */
	close(sockfd);
	return 0;
}