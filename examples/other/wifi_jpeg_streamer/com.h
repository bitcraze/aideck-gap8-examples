#include <stdint.h>

#ifndef __COM_H__
#define __COM_H__

#define MTU (1022)

typedef struct
{
  uint16_t len; // Of data (max 1022)
  uint8_t data[MTU];
} __attribute__((packed)) packet_t;

/* Initialize the communication */
void com_init();

void com_read(packet_t * p);

void com_write(packet_t * p);

#endif