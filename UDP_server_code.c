/* UDP Server code */
#include <stdio.h>        //Standard I/O functions
#include <stdlib.h>       //General utilities (exit, malloc, free,etc)
#include <string.h>       //String operations (memset, strcpy, etc)
#include <unistd.h>       //Required for close()
#include <sys/socket.h>   //Core socket functions (socket, bind, recvfrom, etc)
#include <netinet/in.h>   //Structers for internet domain addresses (sockaddr_in, INADDR_ANY, etc)
#include <arpa/inet.h>    //Functions to convert addresses (inet_ntoa, inet_addr, etc)


#define PORT 8080           //Defining the port your server will listen on
#define BUFFER_SIZE 1024    //Maximum buffer size for incoming/outgoing data

int main(){
	int sockfd;
	struct sockaddr_in servaddr, cliaddr;
	char buffer[BUFFER_SIZE];
	socklen_t len;

	/*create Socket*/
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd < 0) {
		perror("socket creation failed");
		exit(EXIT_FAILURE);
	}

	/* set server address structure*/
	memset(&servaddr, 0, sizeof(servaddr));
	memset(&cliaddr, 0, sizeof(cliaddr));

	servaddr.sin_family = AF_INET;		//IPv4
	servaddr.sin_addr.s_addr = INADDR_ANY;	//Bind to all interfaces
	servaddr.sin_port = htons(PORT);	//Port number

	/* Bind the socket  with the server address*/
	if(bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("Bind failed");
		close(sockfd);
		exit(EXIT_FAILURE);
			
	}

	printf("UDP Server is running on port %d...\n", PORT);

	/* Receive messagesin a loop */
	while (1) { 
		len = sizeof(cliaddr);		//Reset client length
	        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0,
				(struct sockaddr *)&cliaddr, &len);
		if(n < 0){
			perror("Recevie failed");
			continue;
		}

		buffer[n] = '\0';		//Null-terminate received data
		printf("Client (%s:%d) says: %s\n",
			inet_ntoa(cliaddr.sin_addr),
			ntohs(cliaddr.sin_port),
			buffer);
		
		/* Optional: Send response back*/
		sendto(sockfd, buffer, n, 0, (const struct sockaddr *)&cliaddr, len);
	}

	/* Never reached, but good practice*/
	close(sockfd);
	return 0;
}
