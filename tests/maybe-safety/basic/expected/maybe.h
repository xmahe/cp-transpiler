#ifndef CPLUS_MAYBE_H_H
#define CPLUS_MAYBE_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool has_value;
    u32 value;
} __cplus_maybe_u32;

typedef struct Board___Holder {
    __cplus_maybe_u32 maybe_value;
} Board___Holder;

void Board___Holder___Construct(Board___Holder* self);

#endif
