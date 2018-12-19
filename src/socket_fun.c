
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <arpa/inet.h> 
#include <sys/ioctl.h>
#include <string.h>

#include "socket_fun.h"

int init_socket(int port, struct sockaddr_in * servaddr)
{
	struct timeval t;
		t.tv_sec = 0;
		t.tv_usec = 1000000;
		
	//memset(servaddr, 0, sizeof(servaddr)); 
	
	int sockfd; 
      
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    }       
      
	setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&t, sizeof(t));
	
	memset(servaddr, 0, sizeof(*servaddr));
	
    // Filling server information 
    servaddr->sin_family    = AF_INET; // IPv4 
    servaddr->sin_addr.s_addr = INADDR_ANY; 
    servaddr->sin_port = htons(port); 
      
    // Bind the socket with the server address 
    if ( bind(sockfd, (const struct sockaddr *)servaddr, sizeof(*servaddr)) < 0 ) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
	return sockfd;
	
}


int init_send_socket(char const *addr, int port, struct sockaddr_in * servaddr)
{
	int sockfd;
  
	//memset(&servaddr, 0, sizeof(servaddr)); 
	
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    memset(servaddr, 0, sizeof(*servaddr)); 
	
	servaddr->sin_family = AF_INET; 
	servaddr->sin_port = htons(port);
    // Filling server information 
	
	servaddr->sin_addr.s_addr = inet_addr(addr);
	
    return sockfd;

}
