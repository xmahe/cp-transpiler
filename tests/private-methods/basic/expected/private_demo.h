#ifndef CPLUS_PRIVATE_DEMO_H_H
#define CPLUS_PRIVATE_DEMO_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Board___Gpio {
    u32 value;
} Board___Gpio;

void Board___Gpio___Toggle(Board___Gpio* self);
void Board___Gpio___Construct(Board___Gpio* self);

#endif
