#include "maybe_guard.h"

u32 Board___UnwrapMaybe(__cplus_maybe_u32 value) {
    if (value.has_value()) {
        return value.value();
    }
    return 0;
}

