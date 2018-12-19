#include <sys/socket.h>

//initialize socket to receive data
int init_socket(int port, struct sockaddr_in * servaddr);

//initialize and bind socket to send data
int init_send_socket(char const *addr, int port, struct sockaddr_in * servaddr);