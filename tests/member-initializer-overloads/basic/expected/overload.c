#include "overload.h"

void Board___Child___Destruct(Board___Child* self) {
    self->value = 0;
}

void Board___Child___Construct(Board___Child* self, u32 value) {
    self->value = value;
}

void Board___Child___Construct(Board___Child* self, f32 value) {
    self->value = 0;
}

void Board___Parent___Destruct(Board___Parent* self) {
    Board___Child___Destruct(&self->child);
}

void Board___Parent___Construct(Board___Parent* self, u32 value) {
    Board___Child___Construct(&self->child, value);
}

