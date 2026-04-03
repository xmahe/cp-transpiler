#ifndef CPLUS_THERMO_H_H
#define CPLUS_THERMO_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Board___Logger {
} Board___Logger;

typedef struct Board___Thermometer {
    Board___Logger logger;
} Board___Thermometer;

void Board___Logger___Flush(Board___Logger* self);
void Board___Thermometer___Apply(Board___Thermometer* self);
void Board___Thermometer___UseLocalLogger(Board___Thermometer* self);

#endif
