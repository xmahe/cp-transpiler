#ifndef CPLUS_RAII_H_H
#define CPLUS_RAII_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Board___Logger {
} Board___Logger;

typedef struct Board___Sensor {
} Board___Sensor;

void Board___Logger___Destruct(Board___Logger* self);
void Board___Sensor___Destruct(Board___Sensor* self);
void Board___Boot();

#endif
