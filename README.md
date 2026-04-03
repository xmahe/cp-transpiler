# c+

`c+` is a transpiler for a small embedded-focused language that sits between C and C++.

The language is designed to generate plain `.h` and `.c` output so it can build on top of existing C codebases such as vendor HALs and framework-generated MCU projects.

Key ideas:

- classes lower to C structs plus functions
- interfaces are compile-time contracts
- `inject` and `bind` provide compile-time dependency wiring
- quoted-path `import "path/to/module.hp";` is the c+-level dependency form between modules, and the compiler resolves the import closure during semantic analysis
- namespaces lower to `___`-separated C prefixes
- RAII is supported through transpiler-generated cleanup code
- output stays compatible with ordinary C build systems

## Build

Configure and build from the repository root:

```sh
cmake -S . -B build
cmake --build build
```

This builds:

- `cplus`, the transpiler executable
- `cplus_lib`, the shared compiler library used by tests

## Install

Install the transpiler into your user-local prefix:

```sh
cmake -S . -B build
cmake --build build
cmake --install build
```

By default, this installs to:

```text
$HOME/.local/bin/cplus
```

If you want another location, override the install prefix when configuring:

```sh
cmake -S . -B build -DCMAKE_INSTALL_PREFIX=/some/other/prefix
```

## Test

Run the full automated suite with CTest:

```sh
ctest --test-dir build --output-on-failure
```

Or in one flow:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The test setup currently includes:

- CTest fixture-based end-to-end tests that run the transpiler on input directories and compare generated output to checked-in expected files
- optional GoogleTest unit tests, enabled automatically when `CPLUS_ENABLE_GTEST` is on and `find_package(GTest)` succeeds

Legacy `tests/run_*.sh` scripts still exist as convenience helpers, but CTest is the canonical test path.

## Status

Current state:

- the compiler is a real working vertical slice across `lex`, `parse`, `sema`, `lower`, and `emit`
- the repo now builds through CMake and tests through CTest, with GoogleTest available for unit tests
- end-to-end fixture coverage is in place for namespaces, enums, interfaces, `maybe<T>`, compile-time DI, member initializers, out-of-class methods, import-closure resolution, qualified local object lowering, and several RAII cases
- quoted-path `import "path/to/module.hp";` is the intended way to link `c+` modules together; raw `#include` remains for vendor and libc interop

Still left before v1.0:

- fuller AST-driven body parsing and expression lowering
- stronger `maybe<T>` flow analysis
- deeper C interop modeling
- more complete RAII cleanup for general scope exit and body lowering
- a clearer “supported real-world file” boundary

## Layout

- [src](/Users/magnus/c+/src): compiler implementation
- [tests](/Users/magnus/c+/tests): fixture tests, CTest integration, and unit tests
- [example](/Users/magnus/c+/example): example generated-C project showing how to integrate transpilation in CMake
- [language-definition.md](/Users/magnus/c+/language-definition.md): language guide
- [language-syntax.md](/Users/magnus/c+/language-syntax.md): syntax guide
- [transpiler-architecture.md](/Users/magnus/c+/transpiler-architecture.md): compiler architecture guide

## Documents

- [language-definition.md](/Users/magnus/c+/language-definition.md): high-level language guide and design rules
- [language-syntax.md](/Users/magnus/c+/language-syntax.md): concrete syntax guide with examples
- [transpiler-architecture.md](/Users/magnus/c+/transpiler-architecture.md): compiler architecture and pass structure
- [transpiler-implementation-spec.md](/Users/magnus/c+/transpiler-implementation-spec.md): implementation-oriented compiler spec
- [transpiler-implementation-plan.md](/Users/magnus/c+/transpiler-implementation-plan.md): detailed implementation plan
- [next-step-plan.md](/Users/magnus/c+/next-step-plan.md): near-term compiler work plan
- [src/README.md](/Users/magnus/c+/src/README.md): beginner-friendly walkthrough of the compiler source tree
- [tests/README.md](/Users/magnus/c+/tests/README.md): how the test harness works
- [example/README.md](/Users/magnus/c+/example/README.md): how to wire a `c+` module into a real CMake build
- [nvim/README.md](/Users/magnus/c+/nvim/README.md): Neovim syntax highlighting for `.cp` and `.hp`
- [tests/examples/README.md](/Users/magnus/c+/tests/examples/README.md): overview of larger example inputs
- [tests/examples/board-serial/basic/README.md](/Users/magnus/c+/tests/examples/board-serial/basic/README.md): notes for the board-serial example

For a stage-by-stage explanation of the compiler pipeline, start with [src/README.md](/Users/magnus/c+/src/README.md). For build and test usage, start with [tests/README.md](/Users/magnus/c+/tests/README.md).
