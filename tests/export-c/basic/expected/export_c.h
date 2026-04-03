#ifndef CPLUS_EXPORT_C_H_H
#define CPLUS_EXPORT_C_H_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    Board___Mode___kModeIdle,
    Board___Mode___kModeBusy,
    Board___Mode___kModeN,
} Board___Mode;

const char* Board___Mode___ToString(Board___Mode value);

typedef struct Board___Gpio {
    u32 value;
} Board___Gpio;

void Board___Gpio___Toggle(Board___Gpio* self);
void Board___Gpio___Construct(Board___Gpio* self);
i32 Main();
void Board___InternalBoot();

#endif
