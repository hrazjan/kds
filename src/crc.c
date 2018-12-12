#include "crc.h"

uint32_t crc32(char *data, int len) {
   int i, j;
   uint32_t mask, crc = 0xFFFFFFFF;
   for (i = 0; i < len; i++) {
      crc = crc ^ ((unsigned char) data[i]);
      for (j = 7; j >= 0; j--) {
         mask = -(crc & 1);
         crc = (crc >> 1) ^ (0xEDB88320 & mask);
      }
   }
   return ~crc;
}