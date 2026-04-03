#include "enum_scope.h"

i32 Main() {
    i32 i;
    for (i = 0; i < kN; i += 1) {
        return i;
    }
    return 0;
}

const char* Demo___Greeting___ToString(Demo___Greeting value) {
    switch (value) {
    case kIdle: return "kIdle";
    case kBusy: return "kBusy";
    case kN: return "kN";
    default: return "<invalid>";
    }
}

