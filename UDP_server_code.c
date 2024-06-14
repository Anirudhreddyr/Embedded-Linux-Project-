/* UDP Server Code */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define PORT 8080
#define BUFFER_SIZE 1024

int main(){
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buffer [BUFFER_SIZE];
    socklen_t len;
     
    /* create socket file descriptor */ 
    if (( sockfd = socket (AF_INET, SOCK_DGRAM, 0 )) < 0 )
    {
        perror(" socket creation failed ");
        exit ( EXIT_FAILURE );
    }
    
    memset (&servaddr, 0, sizeof(servaddr));
    memset (&cliaddr, 0, sizeof(cliaddr));
    
    /* Filling server information */
    servaddr.sin_family = AF_INET; // IPV4
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);
    
    /* Bind the socket with the server address */
    if (bind(sockfd, (const struct sockaddr *) &servaddr, sizeof(servaddr)) < 0 ){
        perror(" bind failed ");
        exit( EXIT_FAILURE );
    }
    
    while (1) {
        printf (" Waiting for message......\n ");
        len = sizeof(cliaddr);
        
        /* Receive message from client */
        int n = recvfrom( sockfd, (char *)buffer, BUFFER_SIZE, 0, (struct sockaddr *) &cliaddr, &len );
        if (n < 0){
            perror(" recvfrom failed ");
            continue;
        }
        buffer [n] = '\0'; //Null terminate string
        
        printf (" Client: %s\n", buffer );
        
        /* send acknowledgement to client */
        const char *ack = " Message received Successfully ";
        sendto (sockfd, ack, strlen(ack), 0, (const struct sockaddr *) &cliaddr, len);
    }    
    /* close the socket */
    close(sockfd);
    return 0;
}
