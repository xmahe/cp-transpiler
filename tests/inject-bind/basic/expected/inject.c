#include "inject.h"

i32 Board___SpiA___Transfer(Board___SpiA* self, u8* tx, u8* rx, u32 len) {
    return 1;
}

void Board___SpiA___Construct(Board___SpiA* self) {
    /* empty */
}

i32 Board___SpiB___Transfer(Board___SpiB* self, u8* tx, u8* rx, u32 len) {
    return 2;
}

void Board___SpiB___Construct(Board___SpiB* self) {
    /* empty */
}

i32 Board___Device___Send(Board___Device* self, u8* tx, u8* rx, u32 len) {
    return Board___SpiA___Transfer(&self->control_spi, tx, rx, len) + Board___SpiB___Transfer(&self->data_spi, tx, rx, len);
}

void Board___Device___Construct(Board___Device* self) {
    Board___SpiA___Construct(&self->control_spi);
    Board___SpiB___Construct(&self->data_spi);
}

