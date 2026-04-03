#ifndef CPLUS_DEMO_H_H
#define CPLUS_DEMO_H_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    Board___UartState___kUartStateIdle,
    Board___UartState___kUartStateBusy,
    Board___UartState___kUartStateN,
} Board___UartState;

const char* Board___UartState___ToString(Board___UartState value);

typedef struct Board___Uart {
    u32 baud_rate;
} Board___Uart;

i32 Board___Uart___Write(Board___Uart* self, u8* data, u32 length);
void Board___Uart___Construct(Board___Uart* self, u32 baud_rate);
void Board___Boot();

#endif
