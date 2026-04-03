#include "demo.h"

i32 Board___Uart___Write(Board___Uart* self, u8* data, u32 length) {
    /* empty */
}

void Board___Uart___Construct(Board___Uart* self, u32 baud_rate) {
    /* empty */
}

void Board___Boot() {
    /* empty */
}

const char* Board___UartState___ToString(Board___UartState value) {
    switch (value) {
    case Board___UartState___kUartStateIdle: return "kUartStateIdle";
    case Board___UartState___kUartStateBusy: return "kUartStateBusy";
    case Board___UartState___kUartStateN: return "kUartStateN";
    default: return "<invalid>";
    }
}
