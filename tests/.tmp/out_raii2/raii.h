#ifndef CPLUS_RAII_H_H
#define CPLUS_RAII_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Board___Logger {
} Board___Logger;

typedef struct Board___Sensor {
} Board___Sensor;

void Board___Logger___Destroy(Board___Logger* self);
void Board___Sensor___Destroy(Board___Sensor* self);
void Board___Boot();

#endif
