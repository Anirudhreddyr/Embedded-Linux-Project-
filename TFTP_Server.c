/* TFTP RRQ + WRQ Server code */
#include <stdio.h>        //Standard I/O functions
#include <stdlib.h>       //General utilities (exit, malloc, free,etc)
#include <string.h>       //String operations (memset, strcpy, etc)
#include <unistd.h>       //Required for close()
#include <sys/socket.h>   //Core socket functions (socket, bind, recvfrom, etc)
#include <netinet/in.h>   //Structers for internet domain addresses (sockaddr_in, INADDR_ANY, etc)
#include <arpa/inet.h>    //Functions to convert addresses (inet_ntoa, inet_addr, etc)
#include <errno.h>        
#include <pthread.h>
#include <time.h>

#define PORT 69          //Defining the port your server will listen on
#define BUFFER_SIZE 516  //4 Bytes for TFTP + 512 bytes for Data
#define BLOCK_SIZE 512   // Size of each data block transferred in TFTP

/* TFTP OPCODES */
#define RRQ   1          // Read Request 
#define WRQ   2		 // Write Request
#define DATA  3		 // Data Packet
#define ACK   4		 // Acknowledgment Packet
#define ERROR 5		 // Error Packet

/* Structure to hold arguments for each client request */
typedef struct {
	int sockfd;			// Socket descriptor used for communication with the client
	struct sockaddr_in client_addr; // Stores the address of the client IP and port
	socklen_t addr_len;			
	char filename[256];		// Name of the file requested by the client
	int opcode;			// Type of Operation code: RRQ (1) or WRQ (2), 
} client_args;


/*
 * Brief Sends an ACK (Acknowledgment) packet to the client.
 *
 * This function constructs a TFTP ACK packet acknowledging receipt of a specific
 * data block number and sends it back to the client over UDP.
 *
 * @param sockfd       The socket file descriptor used to send the ACK.
 * @param cli_addr     Pointer to the client address structure (destination address).
 * @param addr_len     Length of the client address structure.
 * @param block_num    Block number being acknowledged (typically starts from 0 for WRQ).
 *
 * TFTP ACK Packet Format (4 bytes):
 * ------------------------------------------------
 * | Byte 0 | Byte 1 | Byte 2        | Byte 3      |
 * |   0    |   4    | Block number (high byte)    |
 * |                   Block number (low byte)     |
 * ------------------------------------------------
 */
void send_ack(int sockfd, struct sockaddr_in *cli_addr, socklen_t addr_len, int block_num) {
	char ack[4];
	ack[0] = 0 ;
	ack[1] = ACK;
	ack[2] = (block_num >> 8) & 0XFF;
	ack[3] = block_num & 0XFF;
	sendto(sockfd, ack, 4, 0, (const struct sockaddr *)cli_addr, addr_len);
 }

/* Send TFTP error Packet */
void send_error(int sockfd, struct sockaddr_in *client_addr, socklen_t len, int error_code,  const char* message) {
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

/* Handles the RRQ Request */
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
	} else if (ack_len < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
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

/* Handles the WRQ Request */
void handle_wrq(int sockfd, struct sockaddr_in *client_addr, socklen_t addr_len, char *filename) {
	FILE *fp = fopen(filename, "wb");
	if (!fp){
		send_error(sockfd, client_addr, addr_len, 2, "Cannot create file");
		return;
	}

	send_ack(sockfd, client_addr, addr_len, 0); //ACK block 0

	int expected_block = 1;
	char buffer[BUFFER_SIZE];
	int retries = 0;
	const int MAX_RETRIES = 5;

	while(1) {	
	struct sockaddr_in data_addr;
	socklen_t data_addr_len = sizeof(data_addr);
	int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&data_addr, &data_addr_len);
		if(n < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				if(++retries <= MAX_RETRIES) {
					printf("WRQ: Timeout waiting for block %d, retrying...\n", expected_block);
					continue;
				} else {
					printf("WRQ: Max retries reached. Aborting upload. \n");
				       break;
				}
			} else {		
				perror("WRQ: recvfrom failed");
				break;
		   	}
		}

		retries = 0 ; //reset retries on success

		int opcode = (buffer[0] << 8) | buffer[1];
		int block = (buffer[2] << 8) | buffer[3];

		if(opcode == DATA && block == expected_block) {
			size_t written = fwrite(buffer + 4, 1, n - 4, fp);
			fflush(fp);
			printf("WRQ: Received block %d (%ld bytes)\n", block, written);
		
			send_ack(sockfd, &data_addr, data_addr_len, block);
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

			
void*  client_handler(void *arg) {
	client_args *args = (client_args *)arg;

	if(args->opcode == RRQ) {
		handle_rrq(args->sockfd, &args->client_addr, args->addr_len, args->filename);
	} else if (args->opcode == WRQ) {
		handle_wrq(args->sockfd, &args->client_addr, args->addr_len, args->filename);
	}
	free(arg);
	pthread_exit(NULL);
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

	printf("Multi-threaded TFTP Server (RRQ + WRQ) running on port %d...\n", PORT);

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
		if(buffer[0] == 0 && (buffer[1] == RRQ || buffer[1] == WRQ)) {
			char *filename = buffer + 2;
			client_args *args = malloc(sizeof(client_args));
			args-> sockfd = sockfd;
			args-> client_addr = cliaddr;
		        args-> addr_len = addr_len;
			args-> opcode = buffer[1];
			strncpy(args-> filename, filename, sizeof(args->filename) -1);
			args-> filename[sizeof(args->filename) - 1] = '\0';

			pthread_t tid;
			if (pthread_create(&tid, NULL, client_handler, args) != 0){
				perror("Thread creation failed");
				free(args);
			} else {
				pthread_detach(tid);
			}
		}
	}	
				/* char *mode = filename + strlen(filename) + 1;
	
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
						break; */
	close(sockfd);
	return 0;
}
