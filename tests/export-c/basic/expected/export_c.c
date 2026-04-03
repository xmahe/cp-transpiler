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
    case Board___Mode___kModeIdle: return "kModeIdle";
    case Board___Mode___kModeBusy: return "kModeBusy";
    case Board___Mode___kModeN: return "kModeN";
    default: return "<invalid>";
    }
}
