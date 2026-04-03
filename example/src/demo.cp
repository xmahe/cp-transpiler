#include "stdio.h"

namespace Demo {

fn HelloWorlder::Construct() -> void {
    printf("Hello");
}

fn HelloWorlder::Destruct() -> void {
    printf("World\n");
}

export_c fn Main() -> i32 {
    {  // Show some RAII support
        HelloWorlder hello_worlder;
        printf(" ");
    }  // Should output "Hello World"

    // Print some other greetings
    i16 i;
    for (i = 0; i < Greeting::kN; i += 1) {
        printf("hej");
    }

    // printf("%s", Demo___DemoState___ToString(kDemoStateHello));

    return 0;
}

}
