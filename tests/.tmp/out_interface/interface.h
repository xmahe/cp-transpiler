#ifndef CPLUS_INTERFACE_H_H
#define CPLUS_INTERFACE_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Board___MockSpi {
} Board___MockSpi;

i32 Board___SpiBus___Transfer(Board___SpiBus* self, u8* tx, u8* rx, u32 len);
i32 Board___MockSpi___Transfer(Board___MockSpi* self, u8* tx, u8* rx, u32 len);

#endif
