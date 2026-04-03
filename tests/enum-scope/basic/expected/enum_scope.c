#include "enum_scope.h"

i32 Main() {
    i32 i;
    for (i = 0; i < Demo___Greeting___kN; i += 1) {
        return i;
    }
    return 0;
}

const char* Demo___Greeting___ToString(Demo___Greeting value) {
    switch (value) {
    case Demo___Greeting___kIdle: return "kIdle";
    case Demo___Greeting___kBusy: return "kBusy";
    case Demo___Greeting___kN: return "kN";
    default: return "<invalid>";
    }
}
