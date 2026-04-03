# c+ Language Definition

## Purpose

`c+` is a small systems language intended for embedded and MCU development. It sits between C and C++:

- lower-level and predictable like C
- more structured and ergonomic than C
- intentionally much smaller and stricter than C++
- designed to transpile into ordinary C that can build on top of existing C ecosystems such as STM32Cube-generated code

The toolchain model is:

1. `.cp` source files are interpreted/transpiled into `.c`
2. `.hp` header files are interpreted/transpiled into `.h`
3. the generated C code is compiled by `gcc`, `clang`, or a compatible C compiler

The design goal is to add useful structure for embedded software without introducing runtime cost, hidden allocation, RTTI, exceptions, or template-heavy complexity.

## Design Principles

- Zero-overhead abstractions where possible
- Output must remain readable C
- Easy interoperability with existing C headers, HALs, BSPs, RTOSes, and vendor code
- Deterministic behavior suitable for constrained systems
- Minimal feature set
- No implicit heap use
- No exceptions
- No inheritance-based object model
- No `goto`

## File Model

### Source Files

- `.cp` contains implementation code
- `.hp` contains declarations

Each `.cp` transpiles to a `.c`, and each `.hp` transpiles to a `.h`.

The transpiler performs a language-aware source-to-source conversion rather than compiling directly to machine code.

### Translation Unit Pairing

`c+` assumes that matching basenames belong to the same logical module.

Examples:

- `uart.hp` and `uart.cp` form one module
- `spi_bus.hp` and `spi_bus.cp` form one module

Rules:

- a `.hp` file declares the public surface of its matching `.cp` implementation file
- a `.cp` file may exist without a matching `.hp` for internal-only modules
- a `.hp` file should not have more than one matching `.cp`
- a `.cp` file should not have more than one matching `.hp`
- the transpiler treats a matching basename pair as the natural ownership boundary for declarations and definitions

This keeps the model simple and maps well onto ordinary C header/source organization.

### Inclusion Model

`.hp` files are included from generated `.c` or other `.h` files just like ordinary C headers.

The transpiler should preserve compatibility with existing C include flows:

- `#include "foo.h"` remains valid
- existing vendor headers can be included directly
- raw C declarations may be allowed inside c+ files for interop

## Basic Philosophy of Object Modeling

`c+` supports `class`, but a class is not a C++-style inheritance object. It is a structured way to declare:

- a data type
- methods operating on that data type
- visibility rules for API shaping
- deterministic lifetime hooks

A `class` transpiles to a C `struct` plus a set of namespaced functions.

Example conceptually:

```c+
namespace drivers {

class Uart {
private:
    int fd;

public:
    fn init(int port, int baud) -> bool;
    fn write(byte* data, usize len) -> int;
    fn destroy();
}

}
```

Conceptual C output:

```c
typedef struct drivers___Uart {
    int fd;
} drivers___Uart;

bool drivers___Uart___init(drivers___Uart* self, int port, int baud);
int drivers___Uart___write(drivers___Uart* self, byte* data, usize len);
static void drivers___Uart___destroy(drivers___Uart* self);
```

Note: exact output form is still open in a few areas described later.

## Namespaces

`c+` supports namespaces.

Syntax:

```c+
namespace drivers {
    // declarations
}

namespace board::sensors {
    // declarations
}
```

Semantics:

- namespaces are compile-time only
- they do not exist at runtime
- they transpile into symbol prefixes
- separator is `___` (three underscores) to reduce collision risk with user and vendor C symbols
- namespace scope begins at `{` and ends at the matching `}`

Examples:

- `drivers::Uart` becomes `drivers___Uart`
- `board::sensors::Imu` becomes `board___sensors___Imu`

Methods on classes use the fully qualified class symbol as prefix.

Example:

- `board::sensors::Imu.read()` becomes `board___sensors___Imu___read(...)`

### Naming Conventions

`c+` enforces naming conventions at the language level.

Rules:

- namespaces must use `CamelCase`
- classes must use `CamelCase`
- interfaces must use `CamelCase`
- enums must use `CamelCase`
- methods must use `CamelCase`
- free functions must use `CamelCase`
- variables must use `snake_case`
- conceptually private fields must use `_snake_case`

This is intended to enforce good style immediately rather than treating it as a project convention.

## Classes

### Class Syntax

Basic form:

```c+
class Name {
private:
    // fields and methods

public:
    // fields and methods
}
```

Initial intent:

- class fields become struct members
- class methods become C functions taking `self`
- the order of declarations in source should not matter within the class as long as names are unique

### Representation

A class transpiles to:

- one `typedef struct`
- declarations for exported methods in the generated header
- method definitions in the generated source

Methods are not stored in the object as function pointers by default. Dispatch is static.

### Visibility

`public` and `private` are compile-time visibility controls.

#### `public`

Public members are part of the generated external API.

- public methods are emitted into the generated `.h`
- all fields are emitted into the generated struct definition

#### `private`

Private members are internal to the translation unit or class implementation.

Current intended rule:

- private methods transpile to `static` functions in the generated `.c`
- fields are not private; only methods can be private

This keeps the generated C simple and fully visible while still allowing internal helper methods.

There is no opaque-object mode in v0. Struct layout is always visible in generated headers. Private intent may be documented by generated comments only where useful.

Generated header style for conceptually internal fields should use trailing comments.

Example:

```c
typedef struct drivers___Uart {
    int _fd; // private
    int port;
} drivers___Uart;
```

This is documentation only. The generated C layout remains fully visible and accessible.

### Methods

Methods are functions bound to a class by naming and implicit `self`.

Possible syntax:

```c+
fn init(int port, int baud) -> bool;
fn write(byte* data, usize len) -> int;
fn destroy();
```

Transpiled shape:

```c
bool ns___Class___init(ns___Class* self, int port, int baud);
int ns___Class___write(ns___Class* self, byte* data, usize len);
void ns___Class___destroy(ns___Class* self);
```

Rules:

- non-static methods receive implicit `self`
- `self` maps to `Class* self` in C
- method overloading is not supported, except for `construct`
- default parameters are not supported
- virtual dispatch is not supported
- class-level static methods are supported

Static methods transpile to ordinary C functions without a `self` parameter, still using the class-qualified symbol prefix.

Example:

```c+
class Clock {
public:
    static fn delay_ms(uint32 ms) -> void;
}
```

Transpiles conceptually to:

```c
void ns___Clock___delay_ms(uint32 ms);
```

### Static Variables

Static variables may be declared inside a class definition.

Semantics:

- they do not live inside each object instance
- they transpile to `static` file-scope variables in the generated `.c`
- they are not exposed through the generated `.h`

This makes them class-scoped storage from the c+ author's point of view, while remaining simple internal C state in code generation.

### Fields

Fields are ordinary data members stored inline in the struct.

Intended embedded-friendly behavior:

- no hidden allocations
- no hidden base subobjects
- no padding rules beyond the underlying C compiler's ABI

All non-static fields are public in v0.

### Field Naming

Leading-underscore field naming is reserved for private intent.

Examples:

```c+
int _count;
int port;
```

Rules:

- fields whose names begin with `_` are considered conceptually private at the c+ source level
- the transpiler emits them into the generated struct unchanged
- the transpiler adds trailing `// private` comments for those fields in generated code
- fields without a leading underscore are ordinary public fields
- all non-private variable and field names must use `snake_case`

This provides lightweight source-level signaling without hiding struct layout from C.

## Interfaces

### Purpose

`interface` provides compile-time capability checking for interchangeable MCU drivers and similar modules.

This is explicitly intended for cases such as:

- multiple SPI driver backends
- board-specific GPIO implementations
- vendor HAL vs custom bare-metal driver choices
- mock drivers for tests

An interface is not a runtime vtable by default. It is a compile-time contract.

### Syntax

```c+
interface SpiBus {
    fn transfer(byte* tx, byte* rx, usize len) -> int;
    fn lock() -> void;
    fn unlock() -> void;
}
```

Rules:

- interfaces may only contain method signatures
- no fields
- no implementation bodies
- no constructors or destructors beyond ordinary method signatures

### Fulfillment

A class fulfills an interface if it defines all required method signatures with matching names and compatible types.

Example:

```c+
class Stm32Spi implements SpiBus {
public:
    implementation fn transfer(byte* tx, byte* rx, usize len) -> int;
    implementation fn lock() -> void;
    implementation fn unlock() -> void;
}
```

Rules:

- a class may implement several interfaces
- interface fulfillment is checked at transpile time
- missing methods are a transpile error
- signature mismatches are a transpile error
- `implements` is mandatory
- every method meant to satisfy an interface must be marked `implementation`
- an `implementation` method that matches no declared interface requirement is a transpile error
- `implementation` methods must use the exact same name as the interface method they fulfill

This is intentionally similar to `override` in C++, but stricter: interface conformance must be explicit both at the class level and method level.

### Runtime Use of Interfaces

Interfaces are compile-time only.

- no runtime vtables
- no generated interface wrapper objects
- no dynamic dispatch support in v0

The selected implementation is fixed by source usage and build configuration.

## Enums

`c+` supports enums with enforced naming and iteration rules.

Example:

```c+
enum UartState {
    kUartStateIdle,
    kUartStateBusy,
    kUartStateError,
    kUartStateN,
}
```

Rules:

- enum type names must use `CamelCase`
- enum members must use `kConstantEnumValue` style
- every enum must end with a trailing `...N` member
- the trailing `...N` member is intended for iteration and bounds use

### Enum `ToString`

Every enum implicitly gets a generated `ToString` helper.

Conceptual C output:

```c
const char* UartState___ToString(UartState value);
```

The generated string should be static and constant.

## RAII

### Goal

`c+` supports deterministic resource management similar in spirit to RAII, but mapped onto C-friendly code generation.

This is especially useful for:

- peripheral locks
- temporary interrupt masking
- DMA handle setup/teardown
- buffer ownership wrappers
- chip-select assertions

### Constraints

- no exceptions
- no `goto`
- deterministic scope-based cleanup

### Proposed Semantics

Objects with destructors are cleaned up automatically at scope exit.

Example concept:

```c+
class LockGuard {
public:
    fn construct(Mutex* m);
    fn destroy();
}

fn do_work(Mutex* m) {
    LockGuard g(m);
    // work
}
```

Conceptually generated C:

```c
void do_work(Mutex* m) {
    LockGuard g;
    LockGuard___construct(&g, m);

    // work

    LockGuard___destroy(&g);
}
```

### Scope Exit Rules

Current intended behavior:

- local objects with destructors are destroyed in reverse declaration order at end of scope
- destruction occurs on `return`
- ordinary fallthrough to the end of scope destroys locals in reverse declaration order
- `break` and `continue` do not trigger RAII cleanup beyond what naturally occurs when the surrounding scope ends

Early `return` is expected to be rewritten by the transpiler into structured cleanup code.

Although user-authored `goto` is forbidden, generated C may use compiler-generated cleanup labels and `goto` for RAII lowering.

Rules:

- user-authored `goto` is forbidden in `c+`
- transpiler-generated `goto` is allowed in emitted `.c`
- generated `goto` may only target compiler-generated cleanup labels
- cleanup labels destroy exactly the objects known to be live at that point
- cleanup sections are emitted at the end of the generated function body

This allows early returns to lower cleanly without duplicating destroy sequences across many branches.

### Constructors and Destructors

Proposed convention:

- `construct(...)` is the initializer hook
- `destroy()` is the destructor hook

This avoids C++ parsing ambiguity and maps cleanly to C codegen.

These names are fixed in v0.

### Construction Syntax

User code may use constructor declaration sugar:

```c+
Foo x(1, 2);
Bar y;
```

Semantics:

- `Foo x(1, 2);` declares `x` and invokes the matching `construct(...)`
- `Bar y;` declares `y` and invokes a zero-argument `construct()` if one exists
- if no matching constructor exists, transpilation fails

Conceptual C output:

```c
Foo x;
Foo___construct(&x, 1, 2);
```

This keeps object creation concise while still lowering to explicit C initialization calls.

### Nested Object Construction

If a class contains fields whose types are themselves class types with `construct` / `destroy`, the transpiler must insert member lifetime handling automatically.

Rules:

- member subobjects are constructed in declaration order
- member subobjects are destroyed in reverse declaration order
- this applies both to normal destruction and constructor-failure cleanup

This gives deterministic object composition without hidden runtime systems.

### Infallible Construction

`construct` may not fail in v0.

Rules:

- `construct` does not return a failure status
- construction is expected to succeed for valid firmware configuration
- impossible or invalid conditions are handled by assertion or equivalent fail-fast behavior
- user-configurable build flags may later control how assertion/failure behavior is emitted

This matches the intended MCU usage model:

- objects are typically created during boot or static system bring-up
- object graphs are fixed by firmware design rather than dynamic runtime composition
- RAII applies only to fully constructed objects

There is therefore no partial-construction recovery path in the language core for v0.

### Generated Cleanup Labels

When a function contains multiple object declarations, the transpiler may need several cleanup labels depending on which objects have been constructed so far.

Example concept:

```c+
fn work() -> int {
    A a;
    if (cond1) {
        return -1;
    }

    B b;
    if (cond2) {
        return -2;
    }

    return 0;
}
```

Conceptual generated C:

```c
int work(void) {
    int __ret;
    A a;
    A___construct(&a);

    if (cond1) {
        __ret = -1;
        goto __cleanup_a;
    }

    B b;
    B___construct(&b);

    if (cond2) {
        __ret = -2;
        goto __cleanup_b_a;
    }

    __ret = 0;
    goto __cleanup_b_a;

__cleanup_b_a:
    B___destroy(&b);
__cleanup_a:
    A___destroy(&a);
    return __ret;
}
```

This means:

- yes, several cleanup targets may be needed
- yes, they can still all live at the end of the generated function
- labels should be chained so each one performs the cleanup appropriate for the set of live objects

### Multiple Constructors

Classes may define multiple `construct(...)` methods with different argument signatures.

Example:

```c+
class Uart {
public:
    fn construct();
    fn construct(int port);
    fn construct(int port, int baud);
    fn destroy();
}
```

Rules:

- overload resolution is supported only for `construct`
- signatures must be distinguishable by parameter types and/or arity
- if a constructor call is ambiguous, transpilation fails
- ordinary methods remain non-overloadable in v0

This allows ergonomic setup without expanding overloading into the rest of the language.

### No Hidden Heap

RAII in `c+` does not imply dynamic allocation.

- local class instances are stack allocated unless explicitly placed elsewhere
- ownership of external resources is user-defined
- heap allocation, if supported at all, should be explicit through library APIs

## Control Flow

`c+` aims to stay close to C for control flow, with fewer footguns.

Allowed:

- `if`
- `else`
- `switch`
- `while`
- `for`
- `break`
- `return`

Forbidden:

- `goto`

Rationale:

- easier RAII cleanup generation
- clearer embedded control flow
- fewer irreducible jumps in transpiled output

### Readability and Simplicity Restrictions

To keep parsing, lifetime analysis, and reviews simple, v0 adds these restrictions:

- all control-flow bodies must use braces
- single-line `if`, `else`, `for`, and `while` bodies are forbidden
- assignment inside conditions is forbidden
- declaration interleaving is forbidden within a scope
- programmers who want a new RAII lifetime mid-function must create a new nested scope
- implicit `switch` fallthrough is forbidden
- every `switch` must include `default`
- declaration lists such as `int a, b, c;` are forbidden
- nested `if` depth greater than two levels is forbidden
- `continue` is forbidden
- `for` is restricted to a simple counted form with exactly one init expression, one condition expression, and one step expression
- declarations are not allowed inside a `for` header
- comma expressions are not allowed in a `for` header
- `++` and `--` operators are forbidden
- `for` step expressions must use ordinary assignment-style updates such as `i += 1` or `i = i + 1`

These restrictions are intended to improve readability as well as simplify transpilation.

### Type and Expression Policy

`c+` should guide developers toward explicit, portable types and low-surprise expressions.

Preferred builtin scalar types provided by the transpiler:

- `u8`
- `u16`
- `u32`
- `u64`
- `i8`
- `i16`
- `i32`
- `i64`
- `f32`
- `f64`

Built-in special generic-like type:

- `maybe<T>`

Rules:

- use of fixed-width transpiler-provided types is preferred in `c+` source
- the transpiler should warn on use of legacy C scalar types such as `char`, `short`, `int`, `long`, `float`, and `double`
- character buffers should prefer `u8[]`-style storage over `char[]`
- implicit narrowing conversions should be rejected
- mixed signed/unsigned arithmetic or comparison should require an explicit cast
- all local variables must be initialized before use
- the ternary operator is forbidden
- the comma operator is forbidden everywhere
- duplicate writes to the same variable within a single expression should produce a warning at minimum
- const-correctness should be enforced where possible
- general templates are forbidden
- `maybe<T>` is the only allowed template-like construct in v0

### `maybe<T>`

`maybe<T>` is a built-in special form, not general template support.

Example:

```c+
maybe<UartConfig> maybe_config;
if (maybe_config.has_value()) {
    UartConfig config = maybe_config.value();
}
```

Allowed operations:

- `has_value()`
- `value()`

Rules:

- `maybe<T>` may hold either no value or one value of type `T`
- calling `.value()` is only valid when the transpiler can prove that `.has_value()` was checked first on the same object along the active control-flow path
- otherwise transpilation fails
- this check is compile-time, not runtime

This is intended to prevent common embedded bugs and make code intent more obvious.

### Preprocessor and Macro Policy

The C preprocessor is available for interoperability, but `c+` itself should avoid adding more macro-heavy behavior.

Rules:

- function-like macro definitions are forbidden in `.cp` and `.hp`
- object-like macros may still appear where needed for compatibility
- external C headers may continue to use macros normally

### Recursion

Recursion is forbidden in v0.

The transpiler should reject direct and indirect recursion in user-authored `c+` functions and methods.

## Error Handling

The language does not use exceptions.

Recommended style:

- return status codes
- return `bool`
- return tagged result structs through libraries if needed
- use RAII for cleanup, not for exception unwinding

Open point:

- whether the language should eventually include a lightweight `defer` in addition to RAII, or whether RAII alone is sufficient

## Interoperability with C

This is a core requirement.

`c+` must work on top of existing C frameworks and generated code, including:

- STM32Cube
- vendor HAL and LL libraries
- FreeRTOS
- CMSIS
- existing board support packages

### Interop Expectations

- raw C headers can be included
- raw C functions can be declared and called
- raw C structs, enums, and macros remain usable
- generated C should compile cleanly with ordinary embedded toolchains

### ABI Expectations

The transpiler should not invent a custom ABI.

- structs follow C layout
- function calls use standard C calling conventions
- symbol names are ordinary C symbols with namespace prefixes

## Memory Model

Initial intent:

- values and pointers behave like C unless explicitly extended
- no hidden copies beyond normal C semantics
- no garbage collection
- no reference counting in the language core

Open points:

- whether references should exist as syntax sugar over pointers
- whether move-only semantics are needed for some RAII types
- whether assertion/fail-fast behavior should be configurable by build mode or transpiler flags

## Compilation Model

The `c+` tool performs a source transformation pass.

Example pipeline:

```text
foo.hp  --\
          -> c+ transpiler -> foo.h
foo.cp  --/                  foo.c

foo.c + vendor C sources + generated C sources -> gcc/clang/arm-none-eabi-gcc
```

Recommended first-version responsibilities for the transpiler:

- parse `.hp` and `.cp`
- validate classes, interfaces, namespaces, and visibility
- generate `.h` and `.c`
- inject cleanup code for RAII
- verify interface fulfillment
- preserve line mapping where possible for debuggability

## Suggested Minimal Syntax Surface for v0

This is a proposed first milestone, not a final mandate.

Include:

- namespaces
- classes
- enums
- `public` and `private`
- methods
- fields
- interfaces with `implements`
- `implementation` markers on interface methods
- RAII via `construct` and `destroy`
- nested member construction and destruction
- static methods
- class static variables with internal linkage
- constructor declaration sugar such as `Foo x(args);`
- overloaded `construct(...)` support only
- built-in `maybe<T>` special form
- ordinary C-like statements and expressions
- direct C interop
- mandatory braces on all control-flow bodies
- no assignment in conditions
- no declaration interleaving within a scope
- no implicit `switch` fallthrough
- `switch` must always include `default`
- no `continue`
- restricted counted `for` only
- no `++` or `--`
- maximum `if` nesting depth of two
- warnings for legacy scalar types instead of transpiler-provided fixed-width types
- no implicit narrowing conversions
- no implicit signed/unsigned mixing
- all locals initialized before use
- no ternary operator
- no comma operator anywhere
- no function-like macro definitions in `c+` source
- recursion forbidden
- warn on duplicate writes to the same variable in one expression
- enforced CamelCase for namespaces, classes, interfaces, enums, methods, and free functions
- enforced `snake_case` for variables
- enforced `_snake_case` for conceptually private fields
- enum members in `kConstantEnumValue` style with mandatory trailing `...N`
- implicit enum `ToString`

Exclude:

- inheritance
- templates other than built-in `maybe<T>`
- operator overloading
- exceptions
- RTTI
- `goto`
- implicit heap allocation
- method overloading outside `construct`
- default arguments
- macros in the c+ language itself beyond passing through C preprocessor support
- single-line control-flow bodies
- declaration lists such as `int a, b;`
- assignment expressions inside conditions
- declaration interleaving inside a scope
- `continue`
- `switch` without `default`
- declarations inside `for` headers
- comma expressions in `for` headers
- `++` and `--`
- `if` nesting deeper than two levels
- ternary operator
- comma operator anywhere
- function-like macro definitions in `c+` source
- recursion
- style-violating identifiers
- unsafe `maybe<T>.value()` access without proven `has_value()`

## Example

```c+
namespace board::spi {

interface SpiBus {
    fn transfer(byte* tx, byte* rx, usize len) -> int;
}

class CubeSpi implements SpiBus {
private:
    SPI_HandleTypeDef* _handle;

public:
    fn construct(SPI_HandleTypeDef* h);
    implementation fn transfer(byte* tx, byte* rx, usize len) -> int;
}

}
```

Conceptual generated C:

```c
typedef struct board___spi___CubeSpi {
    SPI_HandleTypeDef* _handle; // private
} board___spi___CubeSpi;

void board___spi___CubeSpi___construct(
    board___spi___CubeSpi* self,
    SPI_HandleTypeDef* h
);

int board___spi___CubeSpi___transfer(
    board___spi___CubeSpi* self,
    byte* tx,
    byte* rx,
    usize len
);
```

## Open Questions

These need decisions before the language definition is stable enough for transpiler implementation.

1. What surface syntax should object construction use in user code?
   Resolved: constructor declaration sugar such as `Foo x(args);` is part of v0.
2. Resolved: `implementation` methods must use the exact same name as the interface method.
3. Resolved: conceptually private fields use leading-underscore naming and are emitted with trailing `// private` comments.
4. Resolved: underscore-prefixed fields remain a documented convention only.

## Working Position

A good first implementation target for MCU work would be:

- namespaces as symbol prefixes
- classes as `struct + functions`
- methods with implicit `self`
- all instance fields public in generated structs
- leading-underscore fields treated as conceptually private and commented as such in generated code
- private methods emitted as `static`
- class static variables emitted as internal `static` storage in `.c`
- interfaces checked at compile time with mandatory `implements`
- interface methods explicitly marked `implementation`
- interface implementation methods required to use exact interface method names
- RAII via `construct` / `destroy` and structured cleanup insertion
- infallible construction with assert/fail-fast handling for invalid conditions
- constructor declaration sugar for local and member object setup
- overloaded `construct(...)` support only
- exactly one top-level namespace block per file
- braces required on all control-flow bodies
- no assignment in conditions
- no declaration interleaving within a scope
- no implicit `switch` fallthrough
- `switch` must always include `default`
- no `continue`
- restricted counted `for` only
- no `++` or `--`
- maximum `if` nesting depth of two
- warnings for legacy scalar types instead of builtin fixed-width types
- no implicit narrowing conversions
- no implicit signed/unsigned mixing without explicit cast
- all locals initialized before use
- no ternary operator
- no comma operator anywhere
- no function-like macro definitions in `c+` source
- recursion forbidden
- warn on duplicate writes to the same variable in one expression
- enforce CamelCase for namespaces, classes, interfaces, enums, methods, and free functions
- enforce `snake_case` for variables
- enforce `_snake_case` for conceptually private fields
- enforce enum members in `kConstantEnumValue` style with trailing `...N`
- generate enum `ToString`
- support built-in `maybe<T>` with compile-time checked `.has_value()` / `.value()` usage
- boot-time oriented object lifetime assumptions for embedded targets
- full C interoperability

That keeps the model simple enough to build, while still providing real value over plain C for embedded projects.
