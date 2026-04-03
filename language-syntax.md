# c+ Syntax Guide

This document shows what `c+` code is supposed to look like.

It is not a full formal grammar.
It is a practical “how do I write this language?” guide.

## The Smallest Useful Shape

Example:

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

That already shows the main style:

- one top-level namespace block
- classes with C-like fields
- `fn Name(args) -> ReturnType`
- out-of-class method definitions allowed in `.cp`

## File Structure

A file can contain:

- `#include` lines
- one top-level namespace block
- classes
- interfaces
- enums
- free functions
- raw C declarations

Typical header:

```c
#include "stm32f4xx_hal.h"

namespace Board {

interface SerialDriver {
    fn Start() -> void;
}

class Uart implements SerialDriver {
public:
    UART_HandleTypeDef* handle;
    fn Construct(UART_HandleTypeDef* handle) -> void;
    implementation fn Start() -> void;
}

}
```

Typical source:

```c
namespace Board {

fn Uart::Construct(UART_HandleTypeDef* handle) -> void {
    self_handle = handle;
}

fn Uart::Start() -> void {
    HAL_UART_Transmit(handle, buffer, len, 100);
}

}
```

## Namespaces

Syntax:

```c
namespace Drivers {
}

namespace Board::Spi {
}
```

Nested blocks are also fine:

```c
namespace Board {
    namespace Spi {
    }
}
```

Rules:

- exactly one top-level namespace block per file
- namespaces are compile-time only
- they become symbol prefixes in generated C

## Classes

Syntax:

```c
class Logger {
private:
    fn FlushInternal() -> void;

public:
    u32 count;
    fn Construct() -> void;
    fn Destroy() -> void;
    fn Reset() -> void;
}
```

Rules:

- fields use C-style declaration shape
- methods use `fn Name(args) -> ReturnType`
- `private:` only really matters for methods
- fields are always visible in generated structs

## Interfaces

Syntax:

```c
interface SpiBus {
    fn Transfer(u8* tx, u8* rx, u32 len) -> i32;
    fn Flush() -> void;
}
```

Rules:

- methods only
- no fields
- no bodies
- no constructors

## Implementing Interfaces

Example:

```c
class MockSpi implements SpiBus {
public:
    implementation fn Transfer(u8* tx, u8* rx, u32 len) -> i32;
    implementation fn Flush() -> void;
}
```

Rules:

- `implements` must be explicit
- implemented methods must use `implementation`
- method names must match exactly

## Compile-Time Dependency Injection

`inject` declares an interface-typed slot inside a class:

```c
class Device {
public:
    inject SpiBus control_spi;
    inject SpiBus data_spi;
}
```

`bind` chooses the concrete implementation for that slot:

```c
bind Device.control_spi = SpiA;
bind Device.data_spi = SpiB;
```

Rules:

- binding is by slot, not just by interface type
- multiple slots of the same interface are allowed
- the transpiler rewrites the slot to the bound concrete type

This is the compile-time way to say: “this class depends on an interface, but this build uses that specific implementation.”

Example with two `SpiBus` dependencies:

```c
class Device {
public:
    inject SpiBus control_spi;
    inject SpiBus log_spi;
}

bind Device.control_spi = SpiA;
bind Device.log_spi = SpiB;
```

That keeps the source written against `SpiBus`, but the generated C uses concrete fields and direct calls.

## Constructor Member Initializers

Constructor declarations can end with a member-initializer list:

```c
class Device {
public:
    Motor motor;
    Sensor sensor;
    fn Construct(u32 speed) -> void : motor(speed), sensor();
}
```

Rules:

- the initializer list only belongs on `Construct`
- each item is `field_name(args)`
- the transpiler lowers the initializer list into ordered constructor calls for the generated struct fields
- it is intended for non-default-constructible member subobjects

This is the `c+` answer to “this member cannot be default-constructed, so initialize it here instead.”

Example:

```c
class Device {
public:
    Motor motor;
    Sensor sensor;
    fn Construct(u32 speed) -> void : motor(speed), sensor();
}
```

That means `motor` is built with `speed`, `sensor` is default-constructed, and then the body of `Construct` runs.

## Methods

Method declarations:

```c
fn Reset() -> void;
fn Write(u8* data, u32 len) -> i32;
static fn DelayMs(u32 ms) -> void;
```

Out-of-class definitions:

```c
fn Logger::Reset() -> void {
    count = 0;
}
```

The current compiler already supports this style and rewrites method bodies toward C form.

## Constructors and Destructors

Reserved method names:

- `Construct`
- `Destroy`

Examples:

```c
fn Construct() -> void;
fn Construct(u32 baud_rate) -> void;
fn Destroy() -> void;
```

Construction sugar:

```c
Logger logger();
Uart uart(115200);
```

That means:

- declare the object
- call the matching `Construct`

`Construct` may be overloaded.
`Destroy` may not.

If a member field cannot be default-constructed, the constructor should initialize it explicitly with a member initializer.

## Fields

Instance fields:

```c
u32 count;
Logger logger;
UART_HandleTypeDef* handle;
u32 _port_id;
```

Rules:

- ordinary names use `snake_case`
- conceptually private fields use `_snake_case`
- underscore privacy is documentation-only in v0

Static fields:

```c
static u32 _instance_count;
```

These become file-scope `static` variables in generated C.

## Enums

Syntax:

```c
enum DriverMode {
    kDriverModePolling,
    kDriverModeInterrupt,
    kDriverModeDma,
    kDriverModeN,
}
```

Rules:

- enum names are `CamelCase`
- members use `kConstantEnumValue`
- final member must be `...N`

## Free Functions

Syntax:

```c
fn Boot() -> void;
fn NormalizeBaudRate(u32 baud_rate) -> u32;
export_c fn Main() -> i32;
```

Definition:

```c
fn NormalizeBaudRate(u32 baud_rate) -> u32 {
    return baud_rate;
}
```

These lower to plain namespaced C functions.

`export_c` exposes a free function with a plain C symbol:

```c
namespace Board {

export_c fn Main() -> i32 {
    return 0;
}

fn InternalBoot() -> void {
}

}
```

That lowers like:

- `Main` -> `Main`
- `InternalBoot` -> `Board___InternalBoot`

## maybe<T>

Examples:

```c
maybe<u32> next_id;
fn AcquireId() -> maybe<u32>;
```

This is the only template-like form allowed in the language.

## Control Flow Style

Allowed:

```c
if (ready) {
    Start();
} else {
    Stop();
}
```

Restrictions:

- braces required
- no single-line control-flow bodies
- no assignment in conditions
- no `continue`
- no `goto`
- no `++` or `--`
- no ternary `?:`

## for Loops

`for` exists, but in a smaller form than C.

Allowed:

```c
u32 i;
for (i = 0; i < count; i += 1) {
}
```

Not allowed:

```c
for (u32 i = 0; i < count; ++i) {
}
```

because:

- no declarations inside the header
- no `++`

## Plain C Calls

This is important.

Calling existing C from `.cp` code is expected:

```c
fn Start() -> void {
    HAL_UART_Transmit(handle, buffer, len, 100);
}
```

That is a major design goal of the language.

## A More Realistic Example

```c
namespace Board {

class Logger {
public:
    fn Flush() -> void;
}

class Thermometer {
public:
    Logger logger;
    u32 reading;
    fn Reset() -> void;
    fn Apply() -> void;
}

fn Thermometer::Reset() -> void {
    reading = 0;
}

fn Thermometer::Apply() -> void {
    logger.Flush();
    Reset();
}

}
```

This is a good example because it shows several important rewrites:

- `reading` should become `self->reading`
- `logger.Flush()` should become a plain C function call on `&self->logger`
- `Reset()` should become a plain C function call on `self`

That is exactly the kind of source form `c+` is trying to make nicer than plain C.
