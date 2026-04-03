#ifndef CPLUS_STATIC_VARS_H_H
#define CPLUS_STATIC_VARS_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Board___Timer {
    u32 tick_count;
} Board___Timer;

void Board___Timer___Construct(Board___Timer* self);
void Board___Boot();

#endif
