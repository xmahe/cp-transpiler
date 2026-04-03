#ifndef CPLUS_ENUM_SCOPE_H_H
#define CPLUS_ENUM_SCOPE_H_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    kIdle,
    kBusy,
    kN,
} Demo___Greeting;

const char* Demo___Greeting___ToString(Demo___Greeting value);

i32 Main();

#endif
