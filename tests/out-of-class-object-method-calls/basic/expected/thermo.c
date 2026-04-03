#include "thermo.h"

void Board___Logger___Flush(Board___Logger* self) {
}

void Board___Thermometer___Apply(Board___Thermometer* self) {
    Board___Logger___Flush(&self->logger);
}

void Board___Thermometer___UseLocalLogger(Board___Thermometer* self) {
    Board___Logger___Flush(&logger);
}

