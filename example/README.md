# Example Project

This folder shows the intended build shape for a real `c+` project.

## Important Build Answer

You do **not** need a separate `cpmake`.

The clean build model is:

1. assume `cplus` is already installed as a normal host tool
2. let CMake find it on `PATH`
3. run it from CMake with a custom command
4. write generated `.h` and `.c` files into the build directory
5. compile the generated `.c` files with the normal C compiler

That means the `c+` step is part of the build graph, not a separate build system.

## Crossing From C Into `c+`

The clean boundary is an exported free function:

```c
namespace Board {

export_c fn Main() -> i32 {
    return 0;
}

}
```

`export_c` means:

- the function is callable directly from handwritten C
- the emitted C symbol keeps the plain function name
- the rest of the module can still stay inside normal namespace-mangled `c+`

So the handwritten C side can stay simple:

```c
#include "demo.h"

int main(void) {
    return Main();
}
```

## Why Not A `PRE_BUILD` Step?

`PRE_BUILD` is the wrong shape here:

- it is not the normal portable CMake pattern
- it is harder to make incremental
- it is less explicit about outputs

The correct CMake pattern is `add_custom_command(OUTPUT ...)`.

That tells CMake:

- exactly which generated files exist
- what command produces them
- what inputs they depend on

## Where Should Intermediate Artifacts Go?

Generated files should go in the build tree, not in the source tree.

In this example they go to:

```text
build/example/generated/
```

That is where the transpiler writes:

- `demo.h`
- `demo.c`

This is cleaner because:

- source control stays clean
- rebuilds are incremental
- generated files are per-build-directory
- debug and release builds do not fight each other

## Build The Example

This example assumes `cplus` is already installed and available on `PATH`.

From the repository root:

```sh
cmake -S . -B build -DCPLUS_BUILD_EXAMPLE=ON
cmake --build build --target cplus_example_app
```

That will:

1. locate the installed `cplus` executable
2. transpile `example/src/demo.hp` and `example/src/demo.cp`
3. compile the generated C together with `example/src/main.c`

If CMake does not find `cplus` automatically, pass it explicitly:

```sh
cmake -S . -B build -DCPLUS_BUILD_EXAMPLE=ON -DCPLUS_EXECUTABLE=$HOME/.local/bin/cplus
```

## Files

- `src/demo.hp` and `src/demo.cp`: the source `c+` module
- `src/cplus_types.h`: temporary fixed-width aliases used by the generated C
- `src/main.c`: a tiny C program that enters `c+` through exported `Main`
- `CMakeLists.txt`: the recommended build integration pattern

## Current Example

The current demo intentionally stays inside the subset the compiler already lowers well:

- `export_c fn Main() -> i32` is the C entry point
- a local `Thing thing;` shows local-object `Construct` / `Destruct`
- enum string conversion is shown through the generated helper

Right now, enum `ToString` is available as a generated C helper function, so the example uses:

```c+
printf("%s", Demo___DemoState___ToString(kDemoStateHello));
```

That is not the final language surface. It is just the currently working form.

## Why The Example Is Off By Default

At the repository root, `CPLUS_BUILD_EXAMPLE` defaults to `OFF`.

That keeps normal compiler builds and tests independent of whether `cplus` is installed globally. The example is meant to show consumer-project integration, not to be a hard dependency of the compiler repo itself.

## Current Limitation

The current transpiler still assumes aliases such as `u8`, `u32`, and `i32` exist in the C compile. That is why this example uses `-include example/include/cplus_types.h`.
The current transpiler still assumes aliases such as `u8`, `u32`, and `i32` exist in the C compile. That is why this example uses `-include example/src/cplus_types.h`.
Longer-term, v1.0 should make the generated C prelude story cleaner.
