#ifndef CPLUS_MAYBE_GUARD_H_H
#define CPLUS_MAYBE_GUARD_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    bool has_value;
    u32 value;
} __cplus_maybe_u32;

u32 Board___UnwrapMaybe(__cplus_maybe_u32 value);

#endif
