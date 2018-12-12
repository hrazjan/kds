#ifndef _packet_h
#define _packet_h
#include <stdint.h>

#define DATALEN 1016

typedef struct {
   uint32_t  size;
   char  name[100];
} Start; 

typedef struct {
    uint32_t dataid;
    unsigned char data[DATALEN];
	uint32_t crc;
} Data;

typedef struct {
    uint32_t hash[8];
} Stop; 

typedef struct {
	char type; //'S' = start, 'D'=data
	uint32_t dataid; // size for S, dataid for D
} Ack;

#endif