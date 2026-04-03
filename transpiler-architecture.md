# c+ Transpiler Architecture Guide

This document explains how the transpiler is supposed to be built, in practical terms.

If `language-definition.md` says what the language is, this document says how the compiler should think about it.

## The Main Job

The transpiler takes:

- `.hp`
- `.cp`

and produces:

- `.h`
- `.c`

Example:

- `thermo.hp`
- `thermo.cp`

become:

- `thermo.h`
- `thermo.c`

The output should look like ordinary human-readable C, not like unreadable compiler sludge.

## The Pipeline

Recommended pipeline:

1. discover matching `.hp` / `.cp` module files
2. lex each file into tokens
3. parse each file into syntax trees
4. merge the module pieces into one logical program model
5. run semantic checks
6. lower `c+` constructs into C-shaped constructs
7. emit `.h`
8. emit `.c`

Think of it like:

- understand the source first
- then rewrite it
- then print it

Do not try to print C directly from raw source text.

## Module Model

Matching basenames are one logical module.

Example:

- `uart.hp` + `uart.cp`

The driver layer should:

- collect both files
- parse both
- merge declarations and definitions
- emit one pair of outputs

This is very important, because many language features depend on it:

- class declarations in `.hp`
- out-of-class method definitions in `.cp`
- free function declarations in `.hp`
- free function bodies in `.cp`
- compile-time DI bindings that may live beside the class they configure
- `export_c` free functions that form a clean C ABI boundary

Without module pairing, the language becomes awkward very quickly.

During lowering and emission, `export_c` free functions bypass normal namespace mangling and keep a plain C-callable symbol name.

In practice, this means a build can provide a small wiring module that binds interface slots to concrete classes for a specific board or firmware variant.

## What Each Stage Should Care About

### 1. Lexer

Input:

- raw source text

Output:

- tokens

It should answer:

- where are the identifiers?
- where are the keywords?
- where are the punctuation marks?

It should not answer:

- does this class exist?
- is this call legal?

### 2. Parser

Input:

- tokens

Output:

- AST

The parser should understand shapes like:

- namespace block
- class declaration
- interface declaration
- enum
- function declaration
- out-of-class method definition
- statement blocks
- expressions

The parser should answer:

- what did the source say?

not:

- was it meaningful?

### 3. Semantic Analysis

Input:

- AST / model

Output:

- validated model + diagnostics

Examples of semantic questions:

- does this interface implementation match?
- does this method definition match a declared class method?
- are all `inject` slots bound exactly once?
- does the binding target implement the requested interface?
- does this enum end in `...N`?
- is this name style allowed?
- is `maybe<T>.value()` being used unsafely?
- does this constructor need explicit member initializers for non-default-constructible subobjects?

This stage is the “does this make sense?” stage.

### 4. Lowering

Input:

- validated `c+` program

Output:

- a simpler C-like IR

This is where the language really becomes C-shaped.

Examples:

- `namespace Board` becomes `Board___`
- methods become plain C functions with `self`
- `reading` becomes `self->reading`
- `Reset()` becomes `Board___Thermometer___Reset(self)`
- `logger.Flush()` becomes `Board___Logger___Flush(&self->logger)`
- `inject` slots become concrete fields chosen by `bind`
- constructor member initializers become explicit ordered `Construct` calls in generated C

For compile-time DI, lowering should resolve slot bindings before emission so the generated C only sees concrete field types.
That keeps the backend simple and avoids runtime interface machinery.

For constructor member initializers, lowering should emit member construction in declaration order and destruct members in reverse order when cleanup is needed.

This is the most important translation stage.

### 5. Emission

Input:

- lowered C-like IR

Output:

- actual `.h` and `.c` text

This stage should be boring.

If the emitter is still trying to understand lots of `c+` semantics, lowering is too weak.

## Why Lowering Exists

Without lowering, the emitter would have to know everything about:

- classes
- interfaces
- namespaces
- `maybe<T>`
- method calls
- RAII cleanup

That is a bad design.

Lowering exists so the emitter mostly sees plain C-shaped pieces like:

- struct
- enum
- function
- global variable

## Practical Example

Input:

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

Lowered idea:

```c
typedef struct Board___Logger {
} Board___Logger;

typedef struct Board___Thermometer {
    Board___Logger logger;
    u32 reading;
} Board___Thermometer;

void Board___Logger___Flush(Board___Logger* self);
void Board___Thermometer___Reset(Board___Thermometer* self);
void Board___Thermometer___Apply(Board___Thermometer* self);

void Board___Thermometer___Reset(Board___Thermometer* self) {
    self->reading = 0;
}

void Board___Thermometer___Apply(Board___Thermometer* self) {
    Board___Logger___Flush(&self->logger);
    Board___Thermometer___Reset(self);
}
```

That is what the architecture should help us reach cleanly.

## Important Internal Data

Useful things for the compiler to track:

- token stream
- AST
- symbol table
- namespace stack
- class metadata
- interface metadata
- type information
- lowered IR

Useful class metadata includes:

- class name
- namespace path
- field list
- static field list
- method list
- constructor list
- whether `Destruct` exists

That metadata becomes very useful once method-body rewriting and RAII lowering get more advanced.

## Name Mangling

Namespaces and methods lower to predictable C names.

Examples:

- `Board::Thermometer` -> `Board___Thermometer`
- `Board::Thermometer.Reset` -> `Board___Thermometer___Reset`

This should be deterministic and boring.

Boring is good here.

## Header Generation

The generated `.h` should contain:

- visible structs
- visible enums
- public function declarations
- public method declarations

It should not expose:

- private methods
- class static variables

Example:

- a private method becomes `static` in the generated `.c`
- not a public declaration in the `.h`

## Source Generation

The generated `.c` should contain:

- private method bodies
- public method bodies
- free function bodies
- file-scope static variables
- enum `ToString` helpers

It should include the matching generated header first.

## RAII Strategy

This is still one of the major unfinished areas, but the architecture should support it.

The intended model:

- track local class-type objects by lexical scope
- inject `Destruct` calls in reverse order
- rewrite `return` into cleanup + final return

That may use compiler-generated `goto` in the emitted C.

That is acceptable, because the source language still forbids user-written `goto`.

## C Interop

The architecture must leave room for plain C interop.

This is not optional.

Examples that must be possible:

- calling HAL functions from method bodies
- using vendor structs and handles in fields
- including existing C headers
- declaring raw C things in `c+` files when needed

So the architecture should stay compatible with:

- raw C declarations
- passthrough C calls
- readable generated C

## What Is Already Working

The current compiler already has several real pieces in place:

- paired module handling
- interface checking
- `maybe<T>` support
- method lowering
- out-of-class method definitions
- basic body rewriting for:
  - `self->field`
  - same-class calls
  - some object method calls

That means this is no longer just a paper design.

## What Still Needs Work

The biggest remaining compiler tasks are:

- fuller AST-based body analysis
- RAII cleanup lowering
- richer object/method call lowering
- stronger C interop modeling
- less text-driven semantic checking

So the architecture should keep pointing toward:

- simpler driver
- smarter lowering
- boring emitter

That is the healthiest direction for this project.
