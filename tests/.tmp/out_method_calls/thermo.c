#include "thermo.h"

void Board___Thermometer___Reset(Board___Thermometer* self) {
    self->reading = 0;
}

void Board___Thermometer___Calibrate(Board___Thermometer* self, u32 offset) {
    self->reading += offset;
}

void Board___Thermometer___Apply(Board___Thermometer* self, u32 offset) {
    Board___Thermometer___Reset(self();
    Board___Thermometer___Calibrate(self, (offset);
}

void Board___Thermometer___construct(Board___Thermometer* self, u32 initial_reading) {
    self->reading = initial_reading;
}

