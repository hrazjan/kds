#ifndef _packet_h
#define _packet_h
#include <stdint.h>

#define DATALEN 1015

typedef struct {
    char type;
    uint32_t  size;
    char name[1019];
} Start; 

typedef struct {
    char type;
    uint32_t dataid;
    unsigned char data[DATALEN];
	uint32_t crc;
} Data;

typedef struct {
    char type;
    uint32_t hash[8];
    char align[991];
} Stop; 

typedef struct {
	char type; //'S' = start, 'D'=data
	uint32_t dataid; // size for S, dataid for D
} Ack;

#endif