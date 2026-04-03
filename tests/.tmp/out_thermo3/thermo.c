#include "thermo.h"

void Board___Thermometer___Calibrate(Board___Thermometer* self, u32 offset) {
    reading += offset;
}

u32 Board___Thermometer___Read(Board___Thermometer* self) {
    return self->reading;
}

void Board___Thermometer___construct(Board___Thermometer* self, u32 initial_reading) {
    reading = initial_reading;
}

void Board___ResetThermometer(u32 value) {
    u32 adjusted;
    adjusted = value + 1;
}

