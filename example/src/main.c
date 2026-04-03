#include "demo.h"

int main(void) {
    Board___Uart uart;
    u8 data[2] = {'o', 'k'};

    Board___Uart___Construct(&uart, 115200U);
    Board___Boot();
    return Board___Uart___Write(&uart, data, 2U);
}
