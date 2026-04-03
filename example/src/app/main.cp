import "drivers/hello-sayer/hello-sayer.hp";

namespace App {

export_c fn Main() -> i32 {
    Drivers::HelloSayer hs;
    hs.SayIt();
    return 0;
}

}
