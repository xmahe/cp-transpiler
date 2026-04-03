#ifndef CPLUS_THERMO_H_H
#define CPLUS_THERMO_H_H

#include <stdbool.h>
#include <stdint.h>

typedef struct Board___Thermometer {
    u32 reading;
} Board___Thermometer;

void Board___Thermometer___Calibrate(Board___Thermometer* self, u32 offset);
u32 Board___Thermometer___Read(Board___Thermometer* self);
void Board___Thermometer___construct(Board___Thermometer* self, u32 initial_reading);
void Board___ResetThermometer(u32 value);

#endif
