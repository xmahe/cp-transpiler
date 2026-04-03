#ifndef CPLUS_THERMO_H_H
#define CPLUS_THERMO_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Board___Thermometer {
    u32 reading;
} Board___Thermometer;

void Board___Thermometer___Reset(Board___Thermometer* self);
void Board___Thermometer___Calibrate(Board___Thermometer* self, u32 offset);
void Board___Thermometer___Apply(Board___Thermometer* self, u32 offset);
void Board___Thermometer___Construct(Board___Thermometer* self, u32 initial_reading);

#endif
