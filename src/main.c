#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <sys/ioctl.h>

#include "packet.h"
#include "crc.h"
#include "packet_queue.h"

#define DATAPORT 8080
#define ACKPORT 8081 

int init_socket(int port)
{
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
    
    // Forcefully attaching socket to the port 8080 
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
                                                &opt, sizeof(opt))) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 
    address.sin_family = AF_INET; 
    address.sin_addr.s_addr = INADDR_ANY; 
    address.sin_port = htons( port ); 
    
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
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,  
                    (socklen_t*)&addrlen))<0) 
    { 
        perror("accept"); 
        exit(EXIT_FAILURE); 
    } 

	return new_socket;
}

int init_send_socket(char const *addr, int port)
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
	serv_addr.sin_port = htons(port); 
	
	// Convert IPv4 and IPv6 addresses from text to binary form 
	if(inet_pton(AF_INET, addr, &serv_addr.sin_addr)<=0) 
	{ 
		printf("\nInvalid address/ Address not supported \n"); 
		return -1; 
	} 

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) 
	{ 
		printf("\nConnection Failed \n"); 
		return -1; 
	}
	return sock;
}

void send_ack(int socket, char type, int dataid)
{
    Ack ack_packet;
    char ack_buffer[sizeof(Ack)] = {0};
    ack_packet.type = type;
    ack_packet.dataid = dataid;
    printf("sending ack id %i\n",ack_packet.dataid);
    memcpy(ack_buffer,&ack_packet, sizeof(Ack));
    send(socket, ack_buffer, sizeof(Ack), 0);
}

void receive_file(int data_socket, int ack_socket)
{
    uint32_t new_crc;
	int datalen, bytes_sent, data_ready, current_id = 0;
    int bytes_to_read = DATALEN;
	char data_buffer[PACKETLEN] = {0};
	Start start_packet;
    Data data_packet;
	FILE *fp;
    char file_name[200] = "received/";

    create_queue(10);

	if (read(data_socket, data_buffer, PACKETLEN) == PACKETLEN) 
	{
		memcpy(&start_packet, data_buffer, datalen);
        if (start_packet.type == 'S')
        {
            send_ack(ack_socket,'S',start_packet.size);
	    	fp = fopen(file_name, "wb+");
		    printf("START:\nsize = %i\n",start_packet.size);
        } 

        while (bytes_sent < start_packet.size)
        {
            if (start_packet.size - bytes_sent < DATALEN)
            {
                bytes_to_read = start_packet.size - bytes_sent;
            }
            ioctl(data_socket, FIONREAD, &data_ready);
            if (data_ready >= PACKETLEN) 
            {
                datalen = read( data_socket , data_buffer, PACKETLEN);
                memcpy(&data_packet, data_buffer, datalen);
                if (data_packet.type != 'D') {continue;} // drop non-data packets
                printf("DATA\nid = %u: received %u bytes.\n",data_packet.dataid,bytes_to_read);
                new_crc = crc32(data_packet.data,DATALEN);
                if (new_crc == data_packet.crc)
                {
                    printf("current = %i\nreceived = %i\n",current_id, data_packet.dataid);
                    if (current_id == data_packet.dataid)
                    {
                        fwrite(&data_packet.data,sizeof(unsigned char),bytes_to_read,fp);
                        send_ack(ack_socket,'D',current_id++);
                        bytes_sent += DATALEN;
                    }
                    else 
                    {
                        insert_packet(data_packet);
                    }
                    while(read_packet(current_id,&data_packet) == 1)
                    {
                        fwrite(&data_packet.data,sizeof(unsigned char),bytes_to_read,fp);
                        send_ack(ack_socket,'D',current_id++);
                        bytes_sent += DATALEN;
                    }
                }
                else
                {
                    printf("CRC ERROR\n");
                    printf("Received CRC: %u\t\tCounted CRC: %u\n",data_packet.crc, new_crc);
                    exit(1);
                    send_ack(ack_socket,'N',data_packet.dataid);
                }
            }
            else 
            {
                while(find_packet(current_id,&data_packet) == 1)
                {
                    send_ack(ack_socket,'D',current_id++);
                    fwrite(&data_packet.data,sizeof(unsigned char),bytes_to_read,fp);
                    bytes_sent += DATALEN;
                }
            }
            fflush(stdout);
        }
        fclose(fp);
	}
}


int main( int argc, const char* argv[] )
{
	int data_socket = init_socket(DATAPORT);
    int ack_socket = init_send_socket("127.0.0.1",ACKPORT); 
	receive_file(data_socket,ack_socket);
	return 0;
}
