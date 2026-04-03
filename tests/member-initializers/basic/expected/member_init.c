#include "member_init.h"

void Board___Child___Destroy(Board___Child* self) {
    self->value = 0;
}

void Board___Child___Construct(Board___Child* self, u32 value) {
    self->value = value;
}

void Board___Parent___Destroy(Board___Parent* self) {
    Board___Child___Destroy(&self->secondary);
    Board___Child___Destroy(&self->primary);
}

void Board___Parent___Construct(Board___Parent* self, u32 primary_value, u32 secondary_value) {
    Board___Child___Construct(&self->primary, primary_value);
    Board___Child___Construct(&self->secondary, secondary_value);
}

