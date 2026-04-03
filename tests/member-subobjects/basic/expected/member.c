#include "member.h"

void Board___Inner___Destruct(Board___Inner* self) {
    self->value = 0;
}

void Board___Inner___Construct(Board___Inner* self) {
    self->value = 1;
}

void Board___Outer___Destruct(Board___Outer* self) {
    Board___Inner___Destruct(&self->second);
    Board___Inner___Destruct(&self->first);
}

void Board___Outer___Construct(Board___Outer* self) {
    Board___Inner___Construct(&self->first);
    Board___Inner___Construct(&self->second);
}

