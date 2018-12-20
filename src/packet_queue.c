#include "packet_queue.h"

PacketQueue pq;

void create_queue(int size)
{
    pq.queue_head = pq.queue_tail = NULL;
    pq.max_length = size;
    pq.length = 0;
}

int insert_packet(Data data_packet)
{
    if (pq.length == pq.max_length) {return -1;}
    if (pq.length == 0)
    {
        pq.queue_head = pq.queue_tail = malloc(sizeof(Node));
        pq.queue_head->data = data_packet;
        pq.queue_head->next = NULL;
        pq.length++;
    }
    else
    {
        Node* current = pq.queue_head;
        Node* previous = NULL;
        while (data_packet.dataid > current->data.dataid)
        {   
            if (current->next)
            {
                previous = current;
                current = current->next;
            }
            else 
            {
                Node* new_node = malloc(sizeof(Node));
                new_node->data = data_packet;
                new_node->next = NULL;
                pq.queue_tail = new_node;
                current->next = new_node;
                pq.length++;
                return 1;
            }
        }
        Node* new_node = malloc(sizeof(Node));
        new_node->data = data_packet;
        new_node->next = current;
        if (previous)
        {
            previous->next = new_node;            
        }
        else
        {
            pq.queue_head = new_node;
        }
        pq.length++;
        return 1;
    }
}


int find_packet(uint32_t id, Data* data_packet)
{
    if (pq.length == 0) {return -1;}
    Node* current = pq.queue_head;
    Node* previous = NULL;
    while (current->data.dataid != id)
    {
        if (current->next)
        {
            previous = current;
            current = current->next;
        }
        else {return -1;}
    }
    *data_packet = current->data;
    if (pq.length == 1) 
    {
        pq.queue_head = pq.queue_tail = NULL;
    }    
    else
    {
        if (!previous)
        {
            pq.queue_head = current->next;
        }
        else if (!current->next)
        {
            previous->next = NULL;
            pq.queue_tail = previous;            
        }
        else 
        {
            previous->next = current->next;
        }
    }
    free(current);
    pq.length--;
    return 1;
}

int read_packet(uint32_t id, Data* data_packet)
{
    if (pq.length == 0) {return -1;}
    Node* current = pq.queue_head;
    Node* previous = NULL;
    while (current->data.dataid != id)
    {
        if (current->next)
        {
            previous = current;
            current = current->next;
        }
        else {return -1;}
    }
    *data_packet = current->data;
	//printf("read %i\n", data_packet->dataid);
    return 1;
}

void print_queue()
{
    printf("HEAD OF QUEUE:%i\n",pq.length);
    if (pq.length == 0) 
    {
        printf("Queue empty!\n");
        return;
    }
    Node* current = pq.queue_head;
    while (current->next)
    {
        printf("DATA ID %u\n", current->data.dataid);
        current = current->next;
    }
    printf("DATA ID %u\n", current->data.dataid);
    printf("TAIL OF QUEUE\n\n");
}

int get_queue_length()
{
    return pq.length;
}

