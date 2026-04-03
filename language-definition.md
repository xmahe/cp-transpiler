# c+ Language Guide

This document explains what `c+` is trying to be, in plain English.

If you only remember one sentence, remember this:

- `c+` is a small embedded language that rewrites into ordinary C

It is meant for code that has to live comfortably on top of existing C ecosystems:

- STM32Cube
- vendor HALs
- board support packages
- RTOS APIs
- plain old C drivers

The goal is not to replace C.
The goal is to make embedded C code less ugly and less error-prone without dragging in the worst parts of C++.

## What c+ Is

`c+` sits between C and C++:

- as predictable as C
- more structured than C
- much smaller than C++
- designed for MCUs and systems code

The toolchain model is:

1. write `.hp` and `.cp`
2. transpile them into `.h` and `.c`
3. compile the generated C with `gcc`, `clang`, or whatever embedded toolchain you already use

So if you already have a working C build, `c+` is supposed to fit into it, not replace it.

## The Design Goal

The language tries to give you:

- namespaces
- classes without C++ runtime baggage
- compile-time-only interfaces
- RAII-style lifetime structure
- cleaner naming and style enforcement

But it deliberately avoids:

- exceptions
- RTTI
- inheritance-heavy object models
- general templates
- hidden heap allocation
- runtime virtual dispatch

In short:

- useful structure: yes
- runtime magic: no

## The Basic Shape

This:

```c
namespace Board {

class Thermometer {
public:
    u32 reading;
    fn Reset() -> void;
}

fn Thermometer::Reset() -> void {
    reading = 0;
}

}
```

is meant to turn into something conceptually like:

```c
typedef struct Board___Thermometer {
    u32 reading;
} Board___Thermometer;

void Board___Thermometer___Reset(Board___Thermometer* self) {
    self->reading = 0;
}
```

That is the entire philosophy of the language:

- keep the nice source form
- end up with plain understandable C

## File Model

`c+` uses two file types:

- `.hp` for declarations
- `.cp` for definitions

Matching basenames are one logical module.

Examples:

- `uart.hp` + `uart.cp`
- `spi_bus.hp` + `spi_bus.cp`

Think of it exactly like ordinary C header/source pairing.

### Practical Meaning

If you have:

- `thermo.hp`
- `thermo.cp`

then the transpiler treats them as one module and emits:

- `thermo.h`
- `thermo.c`

The `.hp` file declares the public surface.
The `.cp` file provides the bodies.

`.cp`-only modules are also allowed for internal code.

## Namespaces

Namespaces are compile-time only.

They do not exist at runtime.
They only turn into symbol prefixes.

Example:

```c
namespace Board::Sensors {
    class Imu {
    }
}
```

becomes names like:

```c
Board___Sensors___Imu
```

The separator is `___` on purpose, to reduce collisions with normal C names.

## Classes

`class` in `c+` does not mean “full C++ class system”.

It means:

- a struct
- plus functions that operate on that struct
- plus a cleaner place to define lifetime and interface rules

Example:

```c
class Logger {
public:
    u32 count;
    fn Reset() -> void;
}
```

Conceptually becomes:

```c
typedef struct Logger {
    u32 count;
} Logger;

void Logger___Reset(Logger* self);
```

So the class model is really:

- data + namespaced functions

not:

- runtime vtables
- hidden inheritance machinery

## Public and Private

Only methods can really be private in v0.

Fields are always visible in the generated struct.

That is intentional.
It keeps the generated C simple and easy to debug.

Example:

```c
class Uart {
private:
    fn Lock() -> void;

public:
    u32 _port_id;
    u32 baud_rate;
}
```

Generated idea:

- `Lock` becomes `static` in the `.c`
- `_port_id` still appears in the struct
- `_port_id` gets a `// private` comment in generated C where appropriate

The underscore on fields is documentation and intent, not true hiding.

## Interfaces

Interfaces are compile-time only.

That means:

- no runtime interface objects
- no vtables
- no dynamic dispatch

They exist so you can say:

- “this class must provide these methods”

Example:

```c
interface SerialDriver {
    fn Start() -> void;
    fn Stop() -> void;
    fn Send(u8* data, u32 length) -> i32;
}
```

And then:

```c
class Uart implements SerialDriver {
public:
    implementation fn Start() -> void;
    implementation fn Stop() -> void;
    implementation fn Send(u8* data, u32 length) -> i32;
}
```

Important rules:

- `implements` must be written explicitly
- interface methods must be marked `implementation`
- the method name must match exactly

This is meant for embedded “pick one concrete driver at compile time” use cases.

## Compile-Time Dependency Injection

`c+` uses `inject` and `bind` to wire interface dependencies at compile time.

Think of this as compile-time assembly wiring, not runtime dependency injection.
The source says what the dependency slots are, and a separate build-time binding says which concrete class fills each slot.

The source shape is:

- a class declares an `inject` slot with an interface type
- a `bind` statement chooses the concrete class for that slot
- the transpiler rewrites the slot to the bound concrete type during lowering

That means you can write code against an interface, while still generating plain C with concrete field types and direct calls.

Example:

```c
interface SpiBus {
    fn Transfer(u8* tx, u8* rx, u32 len) -> i32;
}

class Device {
public:
    inject SpiBus control_spi;
    inject SpiBus data_spi;
}

bind Device.control_spi = SpiA;
bind Device.data_spi = SpiB;
```

The important detail is that bindings are by slot, not just by interface type.
So one class can hold two `SpiBus` dependencies and map them to two different implementations.

This is still compile-time only:

- no runtime interface objects
- no vtables
- no dynamic dispatch

The slot name matters.
That is what lets you bind two `SpiBus` dependencies to two different implementations in the same class.

In practice this gives you a simple board-wiring model:

- one build config can bind `Device.control_spi` to `SpiA`
- another build config can bind `Device.control_spi` to `SpiB`
- the source code using `control_spi` does not need to change

## Constructors and Destructors

`c+` uses fixed lifetime names:

- `Construct`
- `Destroy`

Example:

```c
class Logger {
public:
    fn Construct() -> void;
    fn Destroy() -> void;
}
```

Construction sugar:

```c
Logger logger();
```

conceptually lowers to:

```c
Logger logger;
Logger___Construct(&logger);
```

Important v0 rule:

- `Construct` cannot fail

Constructor member initializers are allowed for non-default-constructible member subobjects.

Syntax example:

```c
class Device {
public:
    Motor motor;
    Sensor sensor;
    fn Construct(u32 speed) -> void : motor(speed), sensor();
}
```

This means:

- construct `motor` with `speed`
- construct `sensor` with its default constructor
- then run the body of `Device.Construct`

Example:

```c
class Device {
public:
    Motor motor;
    fn Construct(u32 speed) -> void : motor(speed);
}
```

This is the `c+` way to say:

- build `motor` as part of `Device` construction
- keep the final generated C concrete and explicit

For MCU code that is fine, because the intended use is mostly boot-time construction.
If something is impossible, the model is assert/fail-fast, not exception-like recovery.

## RAII

The language wants RAII-style cleanup.

That means:

- local class objects should clean up automatically
- nested member objects should be constructed and destroyed in order
- `Destroy` should run on normal scope exit and `return`

Example intent:

```c
fn Boot() -> void {
    Logger logger();
    return;
}
```

conceptually becomes:

```c
void Boot(void) {
    Logger logger;
    Logger___Construct(&logger);
    Logger___Destroy(&logger);
    return;
}
```

What is important here:

- source `c+` forbids user `goto`
- generated C is allowed to use internal cleanup `goto` labels

That is not a contradiction.
It is just a practical lowering strategy.

## Naming Rules

The language enforces naming style on purpose.

This is not “maybe later lint”.
It is part of the language.

Rules:

- namespaces: `CamelCase`
- classes: `CamelCase`
- interfaces: `CamelCase`
- enums: `CamelCase`
- methods: `CamelCase`
- free functions: `CamelCase`
- variables: `snake_case`
- conceptually private fields: `_snake_case`

This is meant to prevent style collapse early.

## Enums

Enums are intentionally strict.

Example:

```c
enum DriverMode {
    kDriverModePolling,
    kDriverModeInterrupt,
    kDriverModeDma,
    kDriverModeN,
}
```

Rules:

- members use `kConstantEnumValue`
- the final member must end with `N`
- every enum gets a generated `ToString`

The trailing `N` exists mainly because embedded code often loops over enum domains.

## maybe<T>

There are no general templates.

The only allowed template-like form is:

```c
maybe<u32>
maybe<SerialConfig>
```

This is the one sanctioned “templaty” feature.

The intent is similar to `std::optional`, but with much stricter language support.

Rules:

- general templates are forbidden
- `maybe<T>` is allowed
- `value()` should only be legal after a proven `has_value()` check

This is there because it is genuinely useful, especially in embedded code, without opening the template floodgates.

## C Interop

This part is essential.

Without C interop, `c+` is useless.

The language is meant to work on top of existing C code, not in isolation.

That means:

- raw C declarations are allowed
- generated output is plain C
- C calls from `.cp` bodies are expected to work
- existing HAL and vendor APIs must remain callable

Practical example:

```c
fn Boot() -> void {
    HAL_UART_Transmit(handle, buffer, len, 100);
}
```

This needs to survive into generated C in a boring way.
That is a first-class design goal, not a side feature.

## Style and Simplicity Restrictions

`c+` is intentionally stricter than C or C++.

Examples of restrictions:

- braces required on control-flow bodies
- no assignment inside conditions
- no `continue`
- no `goto`
- no `++` or `--`
- no ternary operator
- no comma operator
- no function-like macros in `c+` source
- no recursion
- exactly one top-level namespace block per file

Why?

Because the language is trying to optimize for:

- readability
- predictability
- simpler transpilation
- fewer bad habits in embedded code

## What Is Still In Progress

The language direction is clear, but the implementation is still catching up.

Important incomplete areas:

- full RAII cleanup lowering
- deeper AST-based body analysis
- stronger C interop modeling
- richer object and method-call lowering

So the right mental model is:

- the language design is opinionated and fairly stable
- the compiler is real
- but the compiler is not done yet

That is normal for the stage this project is in.
