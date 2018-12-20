#include <unistd.h> 
#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <arpa/inet.h>
#include <netinet/in.h> 
#include <string.h> 
#include <sys/ioctl.h>
#include <time.h>

#include "packet.h"
#include "crc.h"
#include "packet_queue.h"
#include "socket_fun.h"
#include "sha256.h"

#define DATAPORT 8080
#define ACKPORT 8081

void send_ack(int socket, char type, int dataid, struct sockaddr* src_addr, socklen_t len)
{
    Ack ack_packet;
    char ack_buffer[sizeof(Ack)] = {0};
    ack_packet.type = type;
    ack_packet.dataid = dataid;
    printf("sending %c ack id %i\n",ack_packet.type,ack_packet.dataid);
    memcpy(ack_buffer,&ack_packet, sizeof(Ack));
	ack_packet.crc = crc32(ack_buffer, sizeof(Ack)-4);
	memcpy(ack_buffer,&ack_packet, sizeof(Ack));
	sendto(socket, ack_buffer, sizeof(ack_packet), MSG_CONFIRM, src_addr, len);
}

void receive_file(int data_socket, int ack_socket, struct sockaddr * src_addr, struct sockaddr * dest_addr)
{
    uint32_t new_crc;
	uint32_t datalen, bytes_sent, data_ready, current_id = 0;
    int bytes_to_read = DATALEN;
	char data_buffer[PACKETLEN] = {0};
	Start start_packet;
    Data data_packet;
	Stop stop_packet;
	FILE *fp;
	SHA256_CTX ctx;
	sha256_init(&ctx);
    char file_name[200] = "received/";
	
	socklen_t addrlen = sizeof(* src_addr);

    create_queue(10);
	int r = recvfrom(data_socket, data_buffer, PACKETLEN, MSG_WAITALL, dest_addr, &addrlen);
	
	if(r==PACKETLEN)
	{
		memcpy(&data_packet, data_buffer, PACKETLEN);
		printf("start packet type %c\n", start_packet.type);
		
		while(data_packet.type!='D')
		{
			if(crc32(data_buffer, PACKETLEN-4)!=data_packet.crc){continue;}
			if (data_packet.type == 'S')
			{
				memcpy(&start_packet, &data_packet, PACKETLEN);
				send_ack(ack_socket,'S',start_packet.size,src_addr, addrlen);
				printf("size %i\n", start_packet.size);
			}
			r = recvfrom(data_socket, data_buffer, PACKETLEN, MSG_WAITALL, dest_addr, &addrlen);
			memcpy(&data_packet, data_buffer, PACKETLEN);
			printf("data packet type %c\n", data_packet.type);
		}
		printf(strcat(file_name,start_packet.name));
		printf("\n");
		fp = fopen(file_name, "wb+");
		
		while (bytes_sent < start_packet.size)
        {			        
            if (start_packet.size - bytes_sent < DATALEN)
            {
                bytes_to_read = start_packet.size - bytes_sent;
            }
<<<<<<< HEAD
            //ioctl(data_socket, FIONREAD, &data_ready);
            if (r==PACKETLEN) 
            {
                //datalen = read( data_socket , data_buffer, PACKETLEN);
                if (data_packet.type != 'D') {continue;} // drop non-data packets
                printf("DATA\nid = %u: received %u bytes.\n",data_packet.dataid,bytes_to_read);
                new_crc = crc32(data_buffer, PACKETLEN-4);
                if (new_crc == data_packet.crc)
                {
                    printf("current = %i\nreceived = %i\n",current_id, data_packet.dataid);
                    if (current_id == data_packet.dataid)
                    {
                        fwrite(&data_packet.data,sizeof(unsigned char),bytes_to_read,fp);
                        send_ack(ack_socket,'D',current_id++,src_addr,addrlen);
                        bytes_sent += DATALEN;
                    }
                    else if (current_id>data_packet.dataid)
					{
						send_ack(ack_socket, 'D', current_id-1, src_addr, addrlen);
					}
                    else 
                    {
                        insert_packet(data_packet);
                    }
                    while(read_packet(current_id,&data_packet) == 1)
                    {
                        fwrite(&data_packet.data,sizeof(unsigned char),bytes_to_read,fp);
                        send_ack(ack_socket,'D',current_id++,src_addr,addrlen);
                        bytes_sent += DATALEN;
                    }
                }
                else
                {
                    printf("CRC ERROR\n");
                    printf("Received CRC: %u\t\tCounted CRC: %u\n",data_packet.crc, new_crc);
                    send_ack(ack_socket,'N',data_packet.dataid,src_addr,addrlen);
                }
            }
            else 
            {
                while(find_packet(current_id,&data_packet) == 1)
                {
                    send_ack(ack_socket,'D',current_id++,src_addr,addrlen);
                    fwrite(&data_packet.data,sizeof(unsigned char),bytes_to_read,fp);
                    bytes_sent += DATALEN;
=======
			if (data_packet.type != 'D') {continue;} // drop non-data packets
			printf("DATA\nid = %u: received %u bytes.\n",data_packet.dataid,bytes_to_read);
			new_crc = crc32(data_buffer, PACKETLEN-4);
			if (new_crc == data_packet.crc)
			{
				printf("current = %i\nreceived = %i\n",current_id, data_packet.dataid);
				if (current_id == data_packet.dataid)
				{
					fwrite(&data_packet.data,sizeof(unsigned char),bytes_to_read,fp);
					sha256_update(&ctx,data_packet.data,DATALEN);
					send_ack(ack_socket,'D',current_id++,src_addr,addrlen);
					bytes_sent += DATALEN;
>>>>>>> 01635f89cd3c24e5840f06fa3ef0ed52a9d0ef71
				}
				else if (current_id>data_packet.dataid)
				{
					send_ack(ack_socket, 'D', current_id-1, src_addr, addrlen);
				}
				else 
				{
					insert_packet(data_packet);
				}
				while(read_packet(current_id,&data_packet) == 1)
				{
					fwrite(&data_packet.data,sizeof(unsigned char),bytes_to_read,fp);
					sha256_update(&ctx,data_packet.data,DATALEN);
					send_ack(ack_socket,'D',current_id++,src_addr,addrlen);
					bytes_sent += DATALEN;
				}
			}
			else
			{
				printf("CRC ERROR\n");
				printf("Received CRC: %u\t\tCounted CRC: %u\n",data_packet.crc, new_crc);
				send_ack(ack_socket,'N',data_packet.dataid,src_addr,addrlen);
			}
            fflush(stdout);
			r = recvfrom(data_socket, data_buffer, PACKETLEN, MSG_WAITALL, dest_addr, &addrlen);
			memcpy(&data_packet, data_buffer, PACKETLEN);
        }

		if (crc32(data_buffer, PACKETLEN-4)!=data_packet.crc || data_packet.type != 'E')
		{
			while(crc32(data_buffer, PACKETLEN-4)!=data_packet.crc || data_packet.type != 'E')
			{
				printf("ERROR: Stop packet crc doesn't match!\n");
				r = recvfrom(data_socket, data_buffer, PACKETLEN, MSG_WAITALL, dest_addr, &addrlen);
				memcpy(&data_packet, data_buffer, PACKETLEN);
				printf("Packet type: %c\n", data_packet.type);
			}
		}
		memcpy(&stop_packet, &data_packet, PACKETLEN);
		BYTE hash[8*4];
		sha256_final(&ctx, hash);
		if (memcmp( hash, stop_packet.hash, 8*4) == 0)
		{
			send_ack(ack_socket,'E',0,src_addr,addrlen);
		}
		else
		{
			printf("ERROR: Hash doesn't match!\n");
		}
        fclose(fp);
	}
}


int main( int argc, const char* argv[] )
{

	int sockfd_in, sockfd_out;
	//zalozeni socketu send
	struct sockaddr_in socket_out;
	socklen_t addrlen = sizeof(socket_out);
	if ((sockfd_out = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket creation failed\n");
		exit(1);
	} else {
		printf("socked SEND created\n");
	}
	//memset(&socket_out, 0, sizeof(socket_out));
	socket_out.sin_family = AF_INET;
	socket_out.sin_addr.s_addr = inet_addr("127.0.0.1");//INADDR_ANY; //inet_addr("147.32.216.145");
	socket_out.sin_port = htons(ACKPORT);
	memset(socket_out.sin_zero, 0x00, sizeof(socket_out.sin_zero));

	//zalozeni socketu RECIEVE
	struct sockaddr_in socket_in;
	if ((sockfd_in = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket creation failed\n");
		exit(1);
	} else {
		printf("socked RECIEVE created\n");
	}
	int t = 1;
	setsockopt(sockfd_in, SOL_SOCKET, SO_REUSEADDR, (const char*)&t, sizeof(t));

	memset(&socket_in, 0, sizeof(socket_in));
	socket_in.sin_family = AF_INET;
	socket_in.sin_port = htons(DATAPORT);
	socket_in.sin_addr.s_addr = inet_addr("127.0.0.1");//INADDR_ANY;
	//strcpy(buf_out, "ready");

	
	if (bind(sockfd_in, (struct sockaddr *)&socket_in, sizeof(socket_in)) == -1) {
		perror("bind failed\n");
		exit(1);
	} else {
		printf("bind on SEND socked succenfull\n");
	}
	
	receive_file(sockfd_in,sockfd_out, (struct sockaddr *) &socket_out, (struct sockaddr *) &socket_in);
	
	//close(data_socket);
	return 0;
}
