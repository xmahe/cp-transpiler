#include "export_c.h"

void Board___Gpio___Toggle(Board___Gpio* self) {
    /* empty */
}

static void Board___Gpio___Helper(Board___Gpio* self) {
    /* empty */
}

void Board___Gpio___Construct(Board___Gpio* self) {
    /* empty */
}

i32 Main() {
    return 0;
}

void Board___InternalBoot() {
    /* empty */
}

const char* Board___Mode___ToString(Board___Mode value) {
    switch (value) {
    case kModeIdle: return "kModeIdle";
    case kModeBusy: return "kModeBusy";
    case kModeN: return "kModeN";
    default: return "<invalid>";
    }
}

