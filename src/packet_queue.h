#ifndef _packet_queue_h
#define _packet_queue_h

#include <stdlib.h>
#include <stdio.h>

#include "packet.h"

typedef struct {
    void* next;
    Data data;    
} Node;

typedef struct {
    Node* queue_head;
    Node* queue_tail;
    int length;
    int max_length;
} PacketQueue;

// create data packet queue
// size = maximum number of packets queue is able to store. 
void create_queue(int size);
// insert data_packet to queue accordint to it's id
// head is packet with lowest id
// if succeeded return 1 otherwise -1 
int insert_packet(Data data_packet);
// Try to find packet with this id. If found return 1
// and save packet to data_packet a free memory.
// If not found return 0;
int find_packet(uint32_t id, Data* data_packet);
// print queue
void print_queue();
// returns current length
int get_queue_length();

int read_packet(uint32_t id, Data* data_packet);

#endif