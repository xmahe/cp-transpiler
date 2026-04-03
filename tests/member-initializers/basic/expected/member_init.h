#ifndef CPLUS_MEMBER_INIT_H_H
#define CPLUS_MEMBER_INIT_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Board___Child {
    u32 value;
} Board___Child;

typedef struct Board___Parent {
    Board___Child primary;
    Board___Child secondary;
} Board___Parent;

void Board___Child___Destroy(Board___Child* self);
void Board___Child___Construct(Board___Child* self, u32 value);
void Board___Parent___Destroy(Board___Parent* self);
void Board___Parent___Construct(Board___Parent* self, u32 primary_value, u32 secondary_value);

#endif
