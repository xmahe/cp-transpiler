#include "qualified_local_object.h"

void Drivers___HelloSayer___Destruct(Drivers___HelloSayer* self) {
    /* empty */
}

void Drivers___HelloSayer___SayIt(Drivers___HelloSayer* self) {
    /* empty */
}

void Drivers___HelloSayer___Construct(Drivers___HelloSayer* self) {
    /* empty */
}

void Main() {
        Drivers___HelloSayer hs;
    Drivers___HelloSayer___Construct(&hs);
        Drivers___HelloSayer___SayIt(&hs);
    Drivers___HelloSayer___Destruct(&hs);
}

