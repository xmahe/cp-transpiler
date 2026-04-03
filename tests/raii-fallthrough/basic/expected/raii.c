#include "raii.h"

void Board___Logger___Destroy(Board___Logger* self) {
    /* empty */
}

void Board___Sensor___Destroy(Board___Sensor* self) {
    /* empty */
}

void Board___Boot() {
        Board___Logger logger;
        Board___Sensor sensor;
    Board___Sensor___Destroy(&sensor);
    Board___Logger___Destroy(&logger);
}

