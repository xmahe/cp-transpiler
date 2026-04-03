# c+ Syntax Sketch

## Scope

This document defines a practical v0 syntax sketch for `c+`. It is not yet a full formal grammar, but it is concrete enough to drive parser and transpiler work.

The language is intentionally close to C, with a small number of new constructs:

- `namespace`
- `class`
- `interface`
- `implements`
- `implementation`
- `public` / `private`
- constructor sugar via `Type name(args);`

## Lexical Notes

- C-style identifiers, literals, comments, and operators are assumed unless overridden here
- the C preprocessor is assumed to remain available
- raw C declarations are allowed for interoperability
- field names beginning with `_` indicate conceptual privacy

## Naming

Rules:

- namespaces use `CamelCase`
- classes use `CamelCase`
- interfaces use `CamelCase`
- enums use `CamelCase`
- methods use `CamelCase`
- free functions use `CamelCase`
- variables use `snake_case`
- conceptually private fields use `_snake_case`

## File-Level Structure

A file may contain:

- preprocessor directives
- namespace declarations
- interface declarations
- class declarations
- free functions
- raw C declarations

Example:

```c+
#include "stm32f4xx_hal.h"

namespace board::spi {

interface SpiBus {
    fn transfer(byte* tx, byte* rx, usize len) -> int;
}

class CubeSpi implements SpiBus {
private:
    static uint32 _instances;
    fn lock_bus() -> void;

public:
    SPI_HandleTypeDef* _handle;

    fn construct(SPI_HandleTypeDef* h);
    fn destroy();
    implementation fn transfer(byte* tx, byte* rx, usize len) -> int;
}

}
```

## Namespaces

Syntax:

```c+
namespace drivers {
}

namespace board::spi {
}
```

Rules:

- namespace scope begins at `{` and ends at the matching `}`
- nested namespace syntax uses `::`
- namespaces affect generated symbol prefixes only
- exactly one top-level namespace block is allowed per file
- nested namespaces inside that block are allowed

## Enums

Syntax:

```c+
enum DeviceMode {
    kDeviceModeOff,
    kDeviceModeOn,
    kDeviceModeN,
}
```

Rules:

- enum names use `CamelCase`
- enum members use `kConstantEnumValue` style
- every enum must end with a trailing `...N` member
- every enum gets an implicit generated `ToString` helper

## Interfaces

Syntax:

```c+
interface Name {
    fn method(Type arg) -> ReturnType;
}
```

Rules:

- interfaces contain method signatures only
- no fields
- no method bodies
- no overloads
- no constructors or destructors

## Classes

Syntax:

```c+
class Name implements InterfaceA, InterfaceB {
private:
    static int _counter;
    fn helper() -> void;

public:
    int value;
    Handle* _handle;

    fn construct();
    fn construct(int port, int baud);
    fn destroy();
    fn write(byte* data, usize len) -> int;
    static fn delay_ms(uint32 ms) -> void;
    implementation fn transfer(byte* tx, byte* rx, usize len) -> int;
}
```

Rules:

- `implements ...` is optional unless the class fulfills interfaces
- methods may appear under `public:` or `private:`
- fields are always emitted into the generated struct
- `private:` only affects methods
- underscore-prefixed fields are documentation-only private intent

## Members

### Fields

Field declaration syntax follows C-style declarations:

```c+
int value;
SPI_HandleTypeDef* _handle;
```

Rules:

- all instance fields are part of the struct layout
- `_name` means conceptually private
- no enforced field privacy in v0
- non-private variable and field names use `snake_case`

### Static Variables

Syntax:

```c+
static int _counter;
```

Rules:

- allowed in class scope
- transpile to file-scope `static` variables in the `.c`
- not emitted in the generated `.h`

### Methods

Syntax:

```c+
fn read() -> int;
fn write(byte* data, usize len) -> int;
static fn delay_ms(uint32 ms) -> void;
implementation fn transfer(byte* tx, byte* rx, usize len) -> int;
```

Rules:

- instance methods receive implicit `self`
- static methods do not
- `implementation` is only valid on methods that fulfill an interface requirement
- `implementation` methods must exactly match the interface method name
- method overloading is forbidden except for `construct`

## Constructors and Destructors

### Declaration

Syntax:

```c+
fn construct();
fn construct(int port);
fn construct(int port, int baud);
fn destroy();
```

Rules:

- `construct` and `destroy` are reserved method names with lifetime meaning
- `construct` may be overloaded
- `destroy` may not be overloaded
- `construct` may not fail in v0
- invalid conditions inside `construct` are handled by assert/fail-fast policy

### Construction Sugar

Syntax:

```c+
Uart a;
Uart b(1);
Uart c(1, 115200);
```

Semantics:

- declare the object
- invoke the matching `construct(...)`

Equivalent conceptual lowering:

```c
Uart c;
Uart___construct(&c, 1, 115200);
```

## Functions

Free functions use C-like syntax with `fn`:

```c+
fn boot() -> void {
}

fn add(int a, int b) -> int {
    return a + b;
}
```

The language may also allow plain C function declarations directly for interop.

## Types

Preferred builtin scalar types:

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

Built-in special form:

- `maybe<T>`

Rules:

- the transpiler should warn on use of `char`, `short`, `int`, `long`, `float`, and `double` in `c+` source
- prefer `u8` for byte and character buffers
- implicit narrowing conversions are forbidden
- mixed signed/unsigned arithmetic and comparison require explicit casts
- local variables must be initialized before use
- const-correctness should be enforced where possible
- templates are otherwise forbidden

### `maybe<T>`

Syntax:

```c+
maybe<UartConfig> maybe_config;
```

Allowed methods:

- `has_value()`
- `value()`

Rule:

- `value()` may only be used after a provable `has_value()` check on the same object in the active control-flow path
- if the transpiler cannot prove that, transpilation fails

## Statements

Supported statements:

- declaration statements
- expression statements
- `if` / `else`
- `switch`
- `while`
- `for`
- `break`
- `return`
- block statements

Forbidden:

- `goto`

Additional restrictions:

- all control-flow bodies must use braces
- single-line control-flow forms are forbidden
- assignment inside conditions is forbidden
- declaration interleaving is forbidden within a scope
- declaration lists such as `int a, b;` are forbidden
- implicit `switch` fallthrough is forbidden
- every `switch` must include `default`
- `continue` is forbidden
- `for` is limited to a counted form with exactly one init expression, one condition expression, and one step expression
- declarations are not allowed inside a `for` header
- comma expressions are not allowed in a `for` header
- `++` and `--` are forbidden
- `for` step expressions must use assignment-style updates such as `i += 1` or `i = i + 1`
- nested `if` depth greater than two levels is forbidden
- ternary `?:` is forbidden
- comma operator is forbidden everywhere
- duplicate writes to the same variable in one expression should produce a warning

Example:

```c+
if (ready) {
    run();
} else {
    return;
}
```

Not allowed:

```c+
if (ready) run();
if (x = read()) {
}
int a, b;
for (int i = 0; i < n; i += 1) {
}
for (i = 0, j = 0; i < n; i += 1) {
}
for (i = 0; i < n; i++) {
}
a = cond ? x : y;
foo(a, b), bar(c);
```

## Macros

Rules:

- function-like macro definitions are forbidden in `c+` source files
- object-like macros may be tolerated for interoperability
- macros coming from included C headers remain allowed

## Recursion

Recursion is forbidden.

Both direct and indirect recursion should be rejected by the transpiler.

## RAII-Relevant Declarations

Examples:

```c+
fn boot() -> void {
    Spi spi(hspi1);
    LockGuard lock(&mutex);
    spi.transfer(tx, rx, len);
}
```

Semantics:

- local class objects are destroyed at scope end
- return paths trigger destruction of in-scope objects in reverse order

## Example Module

```c+
#include "stm32f4xx_hal.h"

namespace board::spi {

interface SpiBus {
    fn transfer(byte* tx, byte* rx, usize len) -> int;
}

class CubeSpi implements SpiBus {
private:
    static int _instance_count;
    fn check_ready() -> void;

public:
    SPI_HandleTypeDef* _handle;
    int port;

    fn construct(SPI_HandleTypeDef* h);
    fn destroy();
    implementation fn transfer(byte* tx, byte* rx, usize len) -> int;
}

fn board_init() -> void {
    CubeSpi spi(&hspi1);
    spi.transfer(0, 0, 0);
}

}
```

## Parsing Notes

A practical v0 parser can treat the language as:

- mostly C-like tokens and expressions
- with special handling for class/interface/member declarations
- plus constructor declaration sugar in local declarations

The hard parts are not expressions, but declaration context:

- class body parsing
- visibility sections
- `implementation` marker validation
- constructor overload resolution
- scope-based destructor insertion

The syntax restrictions above simplify parsing and lowering further:

- braces are always present around control-flow bodies
- declaration regions are easier to validate because declarations cannot be interleaved with later statements in the same scope
- condition parsing is simpler because assignment expressions are not legal there
