# c+ Transpiler Architecture

## Goal

Build a v0 transpiler that converts:

- `.hp` -> `.h`
- `.cp` -> `.c`

while preserving compatibility with ordinary embedded C toolchains and existing frameworks such as STM32Cube.

## High-Level Pipeline

Recommended pipeline:

1. Discover module pairs by basename
2. Lex and parse `.hp` / `.cp`
3. Build per-module ASTs
4. Resolve namespaces and symbols
5. Validate classes, interfaces, and constructor overloads
6. Perform lifetime lowering for RAII
7. Generate `.h`
8. Generate `.c`
9. Emit dependency and diagnostic information

## Module Model

Matching basenames form one logical module:

- `uart.hp` + `uart.cp`
- `spi.hp` + `spi.cp`

Recommended assumptions:

- `.hp` owns exported declarations
- `.cp` owns method and function definitions
- `.cp`-only modules are allowed for internal code

## Core Data Structures

The transpiler should maintain at least these internal models:

- token stream
- AST
- symbol table
- namespace context
- type table
- interface contract table
- class metadata table
- module graph

Useful class metadata:

- class name
- namespace
- fields
- static variables
- public methods
- private methods
- constructors
- destructor presence
- implemented interfaces

Useful field metadata:

- name
- type
- underscore-private marker
- declaration order
- whether the field type is another class

## AST Shape

Recommended top-level nodes:

- `Module`
- `NamespaceDecl`
- `ClassDecl`
- `InterfaceDecl`
- `FunctionDecl`
- `RawCDecl`

Recommended class subnodes:

- `FieldDecl`
- `StaticFieldDecl`
- `MethodDecl`
- `ConstructorDecl`
- `DestructorDecl`

Recommended statement nodes:

- `BlockStmt`
- `DeclStmt`
- `ExprStmt`
- `IfStmt`
- `ForStmt`
- `WhileStmt`
- `SwitchStmt`
- `BreakStmt`
- `ContinueStmt`
- `ReturnStmt`

## Parsing Strategy

Use a normal lexer and parser rather than regex-based rewriting.

Recommended parser split:

1. Parse file-level items
2. Parse class/interface bodies with dedicated rules
3. Reuse C-like expression parsing for method bodies and function bodies

Suggested parsing technique:

- recursive descent for declarations/statements
- Pratt parser or precedence parser for expressions

This is enough for a weekend prototype if the expression grammar stays close to C.

## Name Mangling

Namespaces and methods should lower to deterministic C symbol names.

Recommended scheme:

- namespace separator: `___`
- method separator: `___`

Examples:

- `board::spi::CubeSpi` -> `board___spi___CubeSpi`
- `board::spi::CubeSpi.transfer` -> `board___spi___CubeSpi___transfer`

Static methods use the same naming scheme, just without `self`.

## Header Generation

For each class, emit:

- `typedef struct ...`
- visible instance fields
- trailing `// private` comments on underscore-prefixed fields
- declarations for public methods
- declarations for static methods

Do not emit:

- private method declarations
- class static variables

For each interface, emit either:

- nothing in generated C headers for v0, or
- comments/documentation declarations only

I recommend comments only in v0, since interfaces are compile-time-only and have no runtime ABI.

## Source Generation

For each class, emit:

- definitions for public methods
- `static` definitions for private methods
- file-scope `static` storage for class static variables

For each free function, emit a normal C function definition.

Include the generated matching header first.

## Constructor Lowering

Construction sugar:

```c+
Uart uart(1, 115200);
```

lowers to:

```c
board___Uart uart;
board___Uart___construct(&uart, 1, 115200);
```

Rules:

- zero-arg declarations lower to zero-arg `construct()` if present
- constructor overload resolution happens during semantic analysis
- no constructor may fail in v0

## Destructor Lowering

Local objects of class type require scope-exit cleanup.

Example:

```c+
fn boot() -> void {
    A a;
    B b;
    return;
}
```

lowers conceptually to:

```c
void boot(void) {
    A a;
    A___construct(&a);
    B b;
    B___construct(&b);
    B___destroy(&b);
    A___destroy(&a);
    return;
}
```

In practice, returns need rewriting so cleanup runs before the emitted `return`.

## RAII Lowering Strategy

Recommended implementation:

1. Walk each function body
2. Track class-type locals per lexical scope
3. On scope exit, inject reverse-order destroy calls
4. On `return`, rewrite to:
   - assign return value to a temp if needed
   - emit cleanup for active scopes
   - emit final return

Important simplification for v0:

- do not add special cleanup rewriting for `break` and `continue`
- only ordinary scope-end and `return` need explicit handling

### Generated `goto` for Cleanup

User-authored `goto` is forbidden in `c+`, but generated C may use `goto` for structured cleanup.

This is the recommended lowering strategy for early returns because it avoids repeated destroy sequences and keeps generated C readable.

Example pattern:

```c
int fn(void) {
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

Key rules:

- generate as many cleanup labels as needed for distinct live-object sets
- place cleanup labels at the end of the function
- chain labels so later cleanup states fall through into earlier ones
- never jump to a label that destroys an object that has not yet been constructed
- labels and temporaries should use reserved compiler-generated names

## Member Subobject Lowering

If a class contains class-typed fields, its generated `construct` and `destroy` need augmentation.

Example:

```c+
class Device {
public:
    Spi spi;
    Gpio cs;
    fn construct();
    fn destroy();
}
```

Generated `construct` should:

1. call `Spi___construct(&self->spi, ...)` if required by definition
2. call `Gpio___construct(&self->cs, ...)`
3. execute user-authored constructor body

Generated `destroy` should:

1. execute user-authored destructor body
2. destroy `cs`
3. destroy `spi`

This is one of the trickiest parts of the transpiler because constructor arguments for member objects need a language rule. For v0, the simplest assumption is:

- member subobjects are default-constructed automatically if they have zero-arg `construct`
- otherwise the enclosing `construct` must initialize them explicitly through ordinary method calls or assignment conventions added later

That detail should be nailed down before implementation.

## Interface Validation

For each `class ... implements ...`:

1. collect required interface methods
2. collect class methods marked `implementation`
3. verify exact name match
4. verify compatible signatures
5. verify all requirements are satisfied
6. verify no extra unmatched `implementation` methods exist

No runtime code is emitted for interfaces in v0.

## Access and Privacy Handling

Privacy is mostly a documentation and codegen concern in v0:

- private methods become `static` in generated `.c`
- underscore-prefixed fields are emitted normally
- underscore-prefixed fields receive trailing `// private` comments in generated `.h`

No hard field-access restriction needs to be enforced in semantic analysis.

## Naming and Enum Validation

The transpiler should validate enforced naming conventions.

Required checks:

- namespaces are `CamelCase`
- classes are `CamelCase`
- interfaces are `CamelCase`
- enums are `CamelCase`
- methods are `CamelCase`
- free functions are `CamelCase`
- variables are `snake_case`
- conceptually private fields are `_snake_case`
- enum members use `kConstantEnumValue` style
- every enum ends with a trailing `...N` member

Enums also require implicit `ToString` generation.

## `maybe<T>` Validation

`maybe<T>` is the only built-in template-like construct in v0.

The transpiler should treat it as a special semantic form rather than general template support.

Required checks:

- only `maybe<T>` is allowed; arbitrary templates are rejected
- `.value()` calls must be dominated by a provable `.has_value()` check on the same object
- if that proof is not available on the active control-flow path, transpilation fails

## Interop Handling

The transpiler must preserve interoperability with C:

- pass through preprocessor directives
- allow raw C declarations
- preserve external includes
- avoid non-portable generated constructs

Generated C should be boring.

That is a design goal, not just an implementation side effect.

## Diagnostics

The transpiler should report:

- parse errors with file/line/column
- unknown types
- invalid `implementation` markers
- constructor ambiguity
- missing interface methods
- duplicate members
- forbidden overloads
- forbidden `goto`
- assignment inside conditions
- declaration interleaving inside a scope
- missing braces on control-flow bodies
- implicit `switch` fallthrough
- `if` nesting deeper than two levels
- use of discouraged legacy scalar types
- implicit narrowing conversions
- mixed signed/unsigned arithmetic or comparison without explicit cast
- uninitialized local variables
- ternary operator
- comma operator
- function-like macro definitions in `c+` source
- direct or indirect recursion
- duplicate writes to the same variable in one expression
- naming-style violations
- missing enum `...N` terminator member
- invalid enum member naming
- const-correctness violations where analyzable
- unsupported template usage
- unsafe `maybe<T>.value()` access without proven `has_value()`

Source mapping quality matters because debugging happens in generated C and on target firmware.

## Suggested Implementation Order

Weekend-project order:

1. File discovery and module pairing
2. Lexer
3. Parser for namespaces, classes, interfaces, and method signatures
4. Basic `.hp` -> `.h` generation
5. Basic `.cp` -> `.c` generation without RAII rewriting
6. Semantic checks for interfaces and constructor overloads
7. Constructor sugar lowering
8. RAII return-path rewriting
9. Member subobject lifetime insertion

This sequence gets visible progress quickly while deferring the hardest lowering pass until the core model is stable.

## Recommended v0 Constraints

To keep the prototype realistic, I would enforce these constraints early:

- no templates
- no general method overloading
- no hard field privacy
- no runtime interfaces
- no dynamic allocation syntax
- no exception model
- no `goto` in source
- exactly one top-level namespace block per file
- braces required on all control-flow bodies
- no assignment expressions inside conditions
- no declaration interleaving within a scope
- no declaration lists
- no implicit `switch` fallthrough
- every `switch` must include `default`
- no `continue`
- restricted counted `for` only
- no declarations inside `for` headers
- no comma expressions in `for` headers
- no `++` or `--`
- maximum `if` nesting depth of two
- warn on legacy scalar types when fixed-width builtin types should be used
- no implicit narrowing conversions
- no implicit signed/unsigned mixing without explicit cast
- all locals initialized before use
- no ternary operator
- no comma operator anywhere
- no function-like macro definitions in `c+` source
- no recursion
- warn on duplicate writes to the same variable in one expression
- enforce CamelCase for namespaces, classes, interfaces, enums, methods, and free functions
- enforce `snake_case` for variables
- enforce `_snake_case` for conceptually private fields
- enforce enum members in `kConstantEnumValue` style with trailing `...N`
- generate enum `ToString`
- support built-in `maybe<T>` only, not general templates

## Open Implementation Questions

1. How should member subobjects with non-default constructors be initialized?
2. Should interfaces emit any generated C artifacts, or only drive validation?
3. Should raw C declarations be parsed loosely and preserved, or represented as opaque AST nodes?
4. Do you want line directives such as `#line` in generated output for debugger friendliness?
