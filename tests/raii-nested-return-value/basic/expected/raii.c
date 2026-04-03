#include "raii.h"

void Board___Logger___Destruct(Board___Logger* self) {
    /* empty */
}

u32 Board___ReadValue() {
    u32 __cplus_ret;
    
        Board___Logger logger;
        {
            Board___Logger inner_logger;
            __cplus_ret = 7;
    goto __cleanup_0;
        }
    __cleanup_0:
    Board___Logger___Destruct(&inner_logger);
    Board___Logger___Destruct(&logger);
    return __cplus_ret;
}

