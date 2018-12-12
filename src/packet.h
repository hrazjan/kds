#ifndef _packet_h
#define _packet_h
#include <stdint.h>

#define DATALEN 1015
#define PACKETLEN 1024

typedef struct {
    char type;
    char name[1019];
    uint32_t  size;
} Start; 

typedef struct {
    char type;
    unsigned char data[DATALEN];
    uint32_t dataid;
	uint32_t crc;
} Data;

typedef struct {
    char type;
    char align[991];
    uint32_t hash[8];
} Stop; 

typedef struct {
	char type; //'S' = start, 'D'=data
	uint32_t dataid; // size for S, dataid for D
} Ack;

#endif