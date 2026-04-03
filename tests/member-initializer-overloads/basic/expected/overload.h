#ifndef CPLUS_OVERLOAD_H_H
#define CPLUS_OVERLOAD_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Board___Child {
    u32 value;
} Board___Child;

typedef struct Board___Parent {
    Board___Child child;
} Board___Parent;

void Board___Child___Destruct(Board___Child* self);
void Board___Child___Construct(Board___Child* self, u32 value);
void Board___Child___Construct(Board___Child* self, f32 value);
void Board___Parent___Destruct(Board___Parent* self);
void Board___Parent___Construct(Board___Parent* self, u32 value);

#endif
