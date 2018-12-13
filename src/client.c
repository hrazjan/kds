// Client side C/C++ program to demonstrate Socket programming 
#include <unistd.h>
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <sys/stat.h>
#include <time.h>
#include <sys/ioctl.h>

//#include "packet.h"
#include "crc.h"
#include "packet_queue.h"

#define PORT 8082 
#define PORT_R 8083

#ifndef min
    #define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
    #define max(a,b) ((a) > (b) ? (a) : (b))
#endif

int init_socket()
{
	printf("enter init socket function\n");
	int server_fd, new_socket; 
    struct sockaddr_in address; 
    int opt = 1; 
    int addrlen = sizeof(address); 
    
    // Creating socket file descriptor 
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) 
    { 
        perror("socket failed"); 
        exit(EXIT_FAILURE); 
    } 
    printf("server_fd %i\n", server_fd);
    // Forcefully attaching socket to the port 8080 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                &opt, sizeof(opt))) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( PORT_R ); 
    
    // Forcefully attaching socket to the port 8080 
    if (bind(server_fd, (struct sockaddr *)&address,  
                                sizeof(address))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    if (listen(server_fd, 3) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
    /*
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,  
                    (socklen_t*)&addrlen))<0) 
    { 
        perror("accept"); 
        exit(EXIT_FAILURE); 
    }*/ 
	//printf("new_socket %i\n", new_socket);
	return new_socket;
}

int receive_ack(int socket, double timeout, Ack * ack_packet)
{
	int bytes_in_buffer = 0;
	char ack_buffer[sizeof(Ack)]={0};
	clock_t t0 = clock();
	uint32_t crc;
	
	ack_packet->type = 0;
	ack_packet->dataid = 0;
	
	ioctl(socket, FIONREAD, &bytes_in_buffer);
	while (((double)(clock()-t0)/1000000.)<timeout) //CLOCKS_PER_SEC=1000000
	{
		ioctl(socket, FIONREAD, &bytes_in_buffer);
		if (bytes_in_buffer>=sizeof(Ack))
		{
			read(socket, ack_buffer, sizeof(Ack));
			memcpy(ack_packet, ack_buffer, sizeof(Ack));
			crc = crc32(ack_buffer, PACKETLEN-4);
			if(crc == ack_packet->crc)
			{
				return 1;
			}
			else
			{
				return -1;
			}
		}
	}
	return 0;
}

int send_file(char* filename, int sockfd, int rec_sock)
{
	FILE *fd;
	FILE *fw;
	clock_t t0;
	double timeout = 0.0001;
	
	struct stat st;
	stat(filename, &st);
	int succ;
	int s;
	uint32_t packet_num;
	uint32_t last_packet_len;
	
	uint32_t size;
	
	uint32_t crc;
	
	Ack ack_packet;
	
	int window_size = 10;
	create_queue(window_size);

	size = st.st_size;
	
	fd = fopen(filename, "r");
	
	// initialize start packet
	ack_packet.dataid = 0;
	ack_packet.type = '0';
	Start start_packet;
	start_packet.type = 'S';
	start_packet.size = size;
	printf("size %i\n", size);
	memcpy(start_packet.name, filename, strlen(filename));
	char buffer[PACKETLEN]={0};
	memcpy(buffer, &start_packet, PACKETLEN);
	// send start packet and wait for ack
	while(ack_packet.type!='S')
	{
		printf("about to send\n");
		s = send(sockfd, buffer, PACKETLEN, 0);
		printf("sent %i bytes \n", s);
		receive_ack(rec_sock, timeout, &ack_packet);
		printf("ack_packet.dataid %i\n", ack_packet.dataid);
	}
	
	printf("ack_packet.dataid: %i\n sending file starts\n", ack_packet.dataid);
	ack_packet.dataid = 0;
	
	fw = fopen("file_to_send.txt", "wb+");
	
	//parse data
	packet_num = size/DATALEN+1;
	
	//send data packets
	Data data_packet;
	unsigned char  data_buffer[PACKETLEN];
	
	uint32_t bytes_sent = 0;
	uint32_t tail_id = -1;
	uint32_t id = 0;
	int lim;
	for (int i=0; i<min(window_size,packet_num);i++) //fill queue
	{
		data_packet.dataid = i;
		data_packet.type = 'D';
		succ = fread(data_packet.data, sizeof(unsigned char), DATALEN, fd);
		memcpy(data_buffer, &data_packet, sizeof(Data));
		data_packet.crc = crc32(data_buffer, PACKETLEN-4);
		insert_packet(data_packet);
		tail_id = i;
	}
	while(id<packet_num)
	{
		printf("current id: %i\n", id);
		printf("tail_id: %i\n", tail_id);
		for(int i=id; i<min(window_size+id, packet_num); i++)
		{
			read_packet(i,&data_packet); 
			memcpy(data_buffer, &data_packet, PACKETLEN);
			send(sockfd, data_buffer, PACKETLEN, 0);
			//printf("sending id : %i\n", data_packet.dataid);
			receive_ack(rec_sock, timeout, &ack_packet);
			if (ack_packet.type=='D')
			{
				printf("ack_packet.dataid: %i\n", ack_packet.dataid);
				id = ack_packet.dataid+1;
				lim = id-window_size;
				for(int j=max(0,lim); j<id;j++)
				{
					find_packet(j, &data_packet);
					//printf("packet id %i found: %i\n", i, find_packet(i, &data_packet));
					//printf("id %i, %i\n", i, fwrite(&data_packet.data, sizeof(char), DATALEN, fw));
				}
				break;
			}
			else if(ack_packet.type=='N')
			{
				read_packet(ack_packet.dataid, &data_packet);
				memcpy(data_buffer, &data_packet, sizeof(Data));
				send(sockfd, data_buffer, sizeof(Data),0);
			}
		}

		for(int i=tail_id+1; i<min(id+window_size,packet_num); i++)
		{
			data_packet.type = 'D';
			data_packet.dataid = i;
			succ = fread(data_packet.data, sizeof(unsigned char), DATALEN, fd);
			memcpy(data_buffer, &data_packet, PACKETLEN);
			data_packet.crc = crc32(data_buffer, PACKETLEN-4);
			insert_packet(data_packet);
			//printf("packet pushed : %i\n", insert_packet(data_packet));
			//printf("queue_length: %i\n", get_queue_length());			
			tail_id = i;
		}
	}	
	fclose(fd);
	fclose(fw);
	return 0;
}

int init_send_socket(char const *addr)
{
	struct sockaddr_in address; 
	int sock = 0, valread; 
	struct sockaddr_in serv_addr; 

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
	{ 
		printf("\n Socket creation error \n"); 
		return -1; 
	} 

	memset(&serv_addr, '0', sizeof(serv_addr)); 

	serv_addr.sin_family = AF_INET; 
	serv_addr.sin_port = htons(PORT); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, addr, &serv_addr.sin_addr)<=0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		return -1; 
	} 
	/*
	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed \n"); 
		return -1; 
	}*/
	printf("send socket %i\n", sock);
	return sock;
}


int main(int argc, char const *argv[]) 
{ 
	printf("Hi! client initialized\n");
	int  sock = init_send_socket("147.32.83.155");
	printf("send_socket created\n");
	int rec_socket = init_socket();
	printf("rec_socket created\n");
	
	if (sock==-1)
	{
		return -1;
	}
	
	send_file("sklenicky.png", sock, rec_socket);
	
	printf("Finished sending file\n"); 
	
	return 0; 
} 
