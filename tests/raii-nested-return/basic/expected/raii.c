#include "raii.h"

void Board___Logger___Destruct(Board___Logger* self) {
    /* empty */
}

void Board___Sensor___Destruct(Board___Sensor* self) {
    /* empty */
}

void Board___Boot() {
        Board___Logger logger;
        {
            Board___Sensor sensor;
            goto __cleanup_0;
        }
    __cleanup_0:
    Board___Sensor___Destruct(&sensor);
    Board___Logger___Destruct(&logger);
    return;
}

