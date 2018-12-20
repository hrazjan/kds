// Client side C/C++ program to demonstrate Socket programming 
#include <unistd.h>
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <arpa/inet.h>
#include <netinet/in.h> 
#include <string.h> 
#include <sys/stat.h>
#include <time.h>
#include <sys/ioctl.h>

//#include "packet.h"
#include "crc.h"
#include "packet_queue.h"
#include "socket_fun.h"
#include "sha256.h"

#define DATAPORT 8082
#define ACKPORT 8083

#ifndef min
    #define min(a,b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
    #define max(a,b) ((a) > (b) ? (a) : (b))
#endif


int receive_ack(int socket, double timeout, Ack * ack_packet, struct sockaddr * src_addr, socklen_t *len)
{
	int bytes_in_buffer = 0;
	char ack_buffer[sizeof(Ack)]={0};
	clock_t t0 = clock();
	uint32_t crc;
	
	ack_packet->type = 0;
	ack_packet->dataid = 0;
	
	int n;
	//socklen_t len = sizeof(*src_addr);
	n = recvfrom(socket, ack_buffer, sizeof(Ack), MSG_WAITALL, src_addr, len);
	if(n!=sizeof(Ack))
	{
		//printf("error length of received ack %i\n", n);
		return -1;
	}
	memcpy(ack_packet, ack_buffer, sizeof(Ack));
	crc = crc32(ack_buffer, sizeof(Ack)-4);
	if(crc == ack_packet->crc)
	{
		printf("received packet type %c, id %u\n", ack_packet->type, ack_packet->dataid);
		return 1;
	}
	else
	{
		printf("CRC ERROR\n");
		ack_packet->dataid = 0;
		ack_packet->type = 0;
		return -2;
	}
}

int send_file(char* filename, int sockfd, int rec_sock, struct sockaddr * dest_addr, struct sockaddr *src_addr)
{
	FILE *fd;
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
	SHA256_CTX sha256_ctx;
	sha256_init(&sha256_ctx);

	Ack ack_packet;
	
	int window_size = 10;
	create_queue(window_size);

	size = st.st_size;
	fd = fopen(filename, "r");
	if (fd==NULL){
		printf("ERROR: File does not exist!\n");	
		exit(1);
	}
	
	// initialize start packet
	ack_packet.dataid = 0;
	ack_packet.type = '0';
	Start start_packet;
	start_packet.type = 'S';
	start_packet.size = size;
	printf("size %i\n", size);
	memset(start_packet.name, '\0', sizeof(start_packet.name));
	memcpy(start_packet.name, filename, strlen(filename));
	char buffer[PACKETLEN]={0};
	memcpy(buffer, &start_packet, PACKETLEN);
	start_packet.crc = crc32(buffer, PACKETLEN-4);
	memcpy(buffer, &start_packet, PACKETLEN);
	socklen_t addrlen = sizeof(*dest_addr);
	// send start packet and wait for ack
	//s = sendto(sockfd, buffer, PACKETLEN, MSG_CONFIRM, (const struct sockaddr *) dest_addr, sizeof(*dest_addr));	
	while(ack_packet.type!='S')
	{
		printf("about to send\n");
		s = 0;
		s = sendto(sockfd, buffer, PACKETLEN, MSG_CONFIRM, dest_addr, sizeof(*dest_addr));
		printf("sent %i bytes \n", s);
		s = receive_ack(rec_sock, timeout, &ack_packet, src_addr, &addrlen);
		//s = recvfrom(rec_sock, buffer, sizeof(Ack), 0, dest_addr, &addrlen);
		printf("receive ack %i\n", s);
		printf("ack_packet.type %c\n", ack_packet.type);
	}
	
	printf("ack_packet.dataid: %i\n sending file starts\n", ack_packet.dataid);
	ack_packet.dataid = 0;
	
	//parse data
	packet_num = size/DATALEN+1;
	printf("packetnum = %u",packet_num);
	
	//send data packets
	Data data_packet;
	unsigned char  data_buffer[PACKETLEN];
	
	uint32_t bytes_sent = 0;
	uint32_t tail_id = 0;
	uint32_t head_id = 0;
	uint32_t id = 0;
	int lim;
	for (uint32_t i=0; i<min(window_size,packet_num);i++) //fill queue
	{
		data_packet.dataid = i;
		data_packet.type = 'D';
		succ = fread(data_packet.data, sizeof(unsigned char), DATALEN, fd);
		sha256_update(&sha256_ctx,data_packet.data,DATALEN);
		memcpy(data_buffer, &data_packet, PACKETLEN);
		data_packet.crc = crc32(data_buffer, PACKETLEN-4);
		printf("ID %u, CRC %u\n", data_packet.dataid, data_packet.crc);
		printf("packet inserted %i\n", insert_packet(data_packet));
		tail_id = i;
	}
	while(id<packet_num)
	{
		printf("current id: %i\n", id);
		printf("tail_id: %i\n", tail_id);
		printf("head_id: %i\n", head_id);
		printf("packet_num %i\n", packet_num);
		for(int i=head_id; i<tail_id+1; i++)
		{
			s = read_packet(i,&data_packet); 
			//printf("packet %i read %i\n", i, s);
			memcpy(data_buffer, &data_packet, PACKETLEN);
			sendto(sockfd, data_buffer, PACKETLEN, MSG_CONFIRM, dest_addr, sizeof(*dest_addr));
			printf("sending id : %i\n", data_packet.dataid);
			/*
			if(receive_ack(rec_sock, timeout, &ack_packet, src_addr, &addrlen)<0){continue;}
			if (ack_packet.type=='D')
			{
				printf("ack_packet.dataid: %i\n", ack_packet.dataid);
				id = ack_packet.dataid+1;
				for(uint32_t j=head_id; j<id;j++)
				{
					//find_packet(j, &data_packet);
					s = find_packet(j, &data_packet);
					head_id = j+1;
					printf("packet id %i found: %i\n", j, s);
					printf("queue length %i\n", get_queue_length());
				}
				break;
			}
			else if(ack_packet.type=='N')
			{
				read_packet(ack_packet.dataid, &data_packet);
				memcpy(data_buffer, &data_packet, PACKETLEN);
				sendto(sockfd, data_buffer, PACKETLEN, MSG_CONFIRM, dest_addr, sizeof(*dest_addr));
			}*/
			if(receive_ack(rec_sock, timeout, &ack_packet, src_addr, &addrlen)==1){break;}
		}

		//if(receive_ack(rec_sock, timeout, &ack_packet, src_addr, &addrlen)<0){continue;}
		if (ack_packet.type=='D')
		{
			printf("ack_packet.dataid: %i\n", ack_packet.dataid);
			id = ack_packet.dataid+1;
			for(uint32_t j=head_id; j<id;j++)
			{
				//find_packet(j, &data_packet);
				s = find_packet(j, &data_packet);
				head_id = j+1;
				printf("packet id %i found: %i\n", j, s);
				printf("queue length %i\n", get_queue_length());
			}
		}
		else if(ack_packet.type=='N')
		{
			read_packet(ack_packet.dataid, &data_packet);
			memcpy(data_buffer, &data_packet, PACKETLEN);
			sendto(sockfd, data_buffer, PACKETLEN, MSG_CONFIRM, dest_addr, sizeof(*dest_addr));
		}
		
		for(uint32_t i=tail_id+1; i<min(id+window_size,packet_num); i++)
		{
			data_packet.type = 'D';
	 		data_packet.dataid = i;
			succ = fread(data_packet.data, sizeof(unsigned char), DATALEN, fd);
			sha256_update(&sha256_ctx,data_packet.data,DATALEN);
			memcpy(data_buffer, &data_packet, PACKETLEN);
			data_packet.crc = crc32(data_buffer, PACKETLEN-4);
			//insert_packet(data_packet);
			printf("packet pushed : %i\n", insert_packet(data_packet));
			printf("queue_length: %i\n", get_queue_length());			
			tail_id = i;
		}
	}

	Stop stop_packet;
	BYTE hash[8*4];
	sha256_final(&sha256_ctx,hash);
	stop_packet.type = 'E';
	memcpy(stop_packet.hash, hash, 8*4);
	memcpy(data_buffer, &stop_packet, PACKETLEN);
	stop_packet.crc = crc32(data_buffer, PACKETLEN-4);
	memcpy(buffer, &stop_packet, PACKETLEN);
	
	while(ack_packet.type!='E')
	{	
		s = sendto(sockfd, buffer, PACKETLEN, MSG_CONFIRM, dest_addr, sizeof(*dest_addr));
		printf("sent %i bytes \n", s);
		s = receive_ack(rec_sock, timeout, &ack_packet, src_addr, &addrlen);
	}
	//s = recvfrom(rec_sock, buffer, sizeof(Ack), 0, dest_addr, &addrlen);
	printf("receive ack %i\n", s);
	printf("ack_packet.type %c\n", ack_packet.type);


	fclose(fd);
	return 0;
}

int main(int argc, char const *argv[]) 
{ 
	int winsize = 1;
	if (argc > 1) {
		winsize = atoi(argv[1]);
	}
	char* dataaddr = "127.0.0.1";
	if (argc > 2) {
		dataaddr = argv[2];
	}
	char* filename = "sklenicky.png";
	if (argc > 3) {
		filename = argv[3];
	}
	/*
	struct sockaddr_in my_addr, dest_addr;
	
	//memset(&my_addr, 0, sizeof(my_addr));
	//memset(&dest_addr, 0, sizeof(dest_addr));
	
	//int  sock = init_send_socket("127.0.0.1", DATAPORT, &dest_addr);
	//int  sock = init_send_socket("127.0.0.1");
	printf("send_socket created\n");
	//int rec_socket = init_socket(ACKPORT, &dest_addr);
	printf("rec_socket created\n");
	
	int sockfd;
  
	//memset(&servaddr, 0, sizeof(servaddr)); 
	
    // Creating socket file descriptor 
    if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) { 
        perror("socket creation failed"); 
        exit(EXIT_FAILURE); 
    } 
    
    memset(&dest_addr, 0, sizeof(dest_addr)); 
	
	dest_addr.sin_family = AF_INET; 
	dest_addr.sin_port = htons(DATAPORT);
    // Filling server information 
	
	dest_addr.sin_addr.s_addr = INADDR_ANY;
	int sock = sockfd;	
	if (sock==-1)
	{
		return -1;
	}
	*/
	
	struct timeval t;
    t.tv_sec = 0;
    t.tv_usec = 10000;
    //PACKET queue[MAX_PACKETS];
	int sockfd, sockfdrec;
	char buf[PACKETLEN];
	char buf2[10];
	clock_t start;
	FILE* fp;
	struct sockaddr_in bindaddr, recaddr;
	socklen_t addrlen = sizeof(bindaddr);

    printf("init\n");

    if ((sockfdrec = socket(AF_INET, SOCK_DGRAM, 0)) == -1) { //zalozeni socketu
		perror("socket creation failed");
		exit(1);
	}
    int option = 1;
	//setsockopt(sockfdrec, SOL_SOCKET, SO_REUSEADDR, (const char*)&option, sizeof(option));
	setsockopt(sockfdrec, SOL_SOCKET, SO_RCVTIMEO, (const char*)&t, sizeof(t));
	//memset(&recaddr, 0, sizeof(recaddr));
	recaddr.sin_family = AF_INET;
	recaddr.sin_port = htons(ACKPORT);
	recaddr.sin_addr.s_addr = INADDR_ANY;
	memset(recaddr.sin_zero, 0x00, sizeof(recaddr.sin_zero));

	/*
	if (bind(sockfdrec, (struct sockaddr *)&recaddr, sizeof(recaddr)) == -1) {
	    perror("bind failed");
		exit(1);
	}*/
	
	if (bind(sockfdrec, (struct sockaddr *)&recaddr, sizeof(recaddr)) == -1) {
		perror("bind failed\n");
		exit(1);
	} else {
		printf("bind on RECEIVE socked succenfull\n");
	}
	
	printf("bind\n");	


	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) { //zalozeni socketu
		perror("socket creation failed");
		exit(1);
	}
	
	memset(&bindaddr, 0, sizeof(bindaddr));
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_addr.s_addr = inet_addr("147.32.217.244");//INADDR_ANY; //inet_addr("192.168.43.100") ; //147.32.219.190
	bindaddr.sin_port = htons(DATAPORT);
	
	send_file(filename, sockfd, sockfdrec, (struct sockaddr *) &bindaddr, (struct sockaddr *)&recaddr);
	//send_file("sklenicky.png", sockfd, sockfd, (struct sockaddr *) &bindaddr, (struct sockaddr *)&recaddr);
	
	printf("Finished sending file\n"); 
	
	close(sockfdrec);
	
	return 0; 
} 
