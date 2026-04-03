#include "local_object.h"

void Board___Logger___Destruct(Board___Logger* self) {
    /* empty */
}

void Board___Logger___Ping(Board___Logger* self) {
    /* empty */
}

void Board___Logger___Construct(Board___Logger* self) {
    /* empty */
}

i32 Boot() {
    i32 __cplus_ret;
    
        Board___Logger logger;
    Board___Logger___Construct(&logger);
        Board___Logger___Ping(&logger);
        __cplus_ret = 7;
    goto __cleanup_0;
    __cleanup_0:
    Board___Logger___Destruct(&logger);
    return __cplus_ret;
}

