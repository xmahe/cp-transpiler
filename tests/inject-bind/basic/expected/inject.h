#ifndef CPLUS_INJECT_H_H
#define CPLUS_INJECT_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Board___SpiA {
} Board___SpiA;

typedef struct Board___SpiB {
} Board___SpiB;

typedef struct Board___Device {
    Board___SpiA control_spi;
    Board___SpiB data_spi;
} Board___Device;

i32 Board___SpiA___Transfer(Board___SpiA* self, u8* tx, u8* rx, u32 len);
void Board___SpiA___Construct(Board___SpiA* self);
i32 Board___SpiB___Transfer(Board___SpiB* self, u8* tx, u8* rx, u32 len);
void Board___SpiB___Construct(Board___SpiB* self);
i32 Board___Device___Send(Board___Device* self, u8* tx, u8* rx, u32 len);
void Board___Device___Construct(Board___Device* self);

#endif
