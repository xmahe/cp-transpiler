#include "demo.h"

i32 Board___Uart___Write(Board___Uart* self, u8* data, u32 length) {
    /* empty */
}

void Board___Uart___construct(Board___Uart* self, u32 baud_rate) {
    /* empty */
}

void Board___Boot() {
    /* empty */
}

const char* Board___UartState___ToString(Board___UartState value) {
    switch (value) {
    case kUartStateIdle: return "kUartStateIdle";
    case kUartStateBusy: return "kUartStateBusy";
    case kUartStateN: return "kUartStateN";
    default: return "<invalid>";
    }
}

