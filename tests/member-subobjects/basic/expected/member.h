#ifndef CPLUS_MEMBER_H_H
#define CPLUS_MEMBER_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Board___Inner {
    u32 value;
} Board___Inner;

typedef struct Board___Outer {
    Board___Inner first;
    Board___Inner second;
} Board___Outer;

void Board___Inner___Destroy(Board___Inner* self);
void Board___Inner___Construct(Board___Inner* self);
void Board___Outer___Destroy(Board___Outer* self);
void Board___Outer___Construct(Board___Outer* self);

#endif
