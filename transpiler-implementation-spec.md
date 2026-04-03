# c+ Transpiler Implementation Spec

## Current Status

This document describes the intended compiler shape, but the project is no longer just a proposal.

What already exists:

- a working C++ compiler implementation
- a pass-oriented structure with `lex`, `parse`, `sema`, `lower`, and `emit`
- a lowered IR
- CMake build support
- CTest fixture tests
- optional GoogleTest unit tests
- real support for namespaces, classes, interfaces, `maybe<T>`, compile-time DI, member initializers, and several RAII cases

Main remaining gaps before v1.0:

- fuller AST-driven body parsing and lowering
- stronger `maybe<T>` control-flow analysis
- deeper C interop modeling
- broader RAII cleanup support beyond the current slices

Exported C entrypoints should stay simple:

- `export_c` applies only to free functions
- exported free functions bypass namespace mangling during lowering and emission

## Recommendation

Implement the transpiler in modern C++.

That is the pragmatic choice for a weekend project:

- easy string handling with `std::string` / `std::string_view`
- easy ownership with `std::unique_ptr`
- good containers in the STL
- simple file and path handling with `std::filesystem`
- easy deployment on the same systems that already build embedded C/C++

Rust could also work, but it will slow initial iteration unless you already want the compiler project itself to be a Rust exercise.

For v0, the transpiler should be:

- a normal command-line executable
- single-process
- source-to-source only
- deterministic

## Non-Goals

For the first version, do not try to build:

- an optimizing compiler
- a macro processor
- a custom backend
- a linker
- a runtime
- a build system

The transpiler should only turn `c+` into clean C and stop there.

## Core Implementation Choice

Use a classic multi-pass front end:

1. file discovery
2. lexing
3. parsing
4. AST building
5. semantic analysis
6. lowering
7. C emission

Do not try to emit C directly from tokens.
Do not try to fold semantic analysis into parsing.
Those shortcuts will make RAII lowering and diagnostics worse very quickly.

## Should Namespaces Run First or Last?

Neither. Namespaces should be parsed early and resolved during semantic analysis, then used during emission.

More precisely:

- parse namespace blocks into AST nodes first
- maintain a namespace stack during parsing
- attach fully qualified namespace information to declarations in the AST
- resolve and validate names during semantic analysis
- apply mangling only during lowering/emission

So:

- namespace syntax is handled early
- namespace validation happens in semantic analysis
- namespace-to-symbol-prefix conversion happens late

That is the clean split.

## Recommended Pass Order

### Pass 0: File Discovery

Input:

- root files or directories

Output:

- module list
- paired `.hp` / `.cp` modules by basename

Responsibilities:

- discover source files
- enforce one top-level namespace block per file
- pair `foo.hp` with `foo.cp`
- allow `.cp`-only internal modules

### Pass 1: Lexing

Output:

- token stream with file/line/column spans

Responsibilities:

- tokenize identifiers, keywords, punctuation, literals, comments, and preprocessor lines
- preserve enough trivia for readable re-emission if needed
- reject clearly forbidden tokens early, such as `goto`, `++`, `--`, `?:` fragments where practical

Recommended token categories:

- identifiers
- keywords
- integer literals
- float literals
- string literals
- char/byte literals
- operators
- delimiters
- preprocessor lines

### Pass 2: Parsing

Output:

- full AST per file

Responsibilities:

- parse namespace blocks
- parse classes, interfaces, enums, functions, and raw C declarations
- parse `maybe<T>` as a dedicated type form, not as general templates
- parse statements and expressions
- record exact source spans for diagnostics

Important parser behavior:

- keep the parser mostly syntax-only
- do not try to resolve types yet
- constructor sugar stays as a distinct AST node initially

### Pass 3: AST Normalization

Output:

- cleaned AST in a more uniform shape

Responsibilities:

- normalize namespace nesting into fully qualified namespace paths
- normalize visibility sections
- normalize constructor declarations
- normalize enum declarations
- normalize `implementation` markers

This pass should reduce later semantic special cases.

### Pass 4: Symbol Collection

Output:

- global and per-module symbol tables

Responsibilities:

- collect namespace symbols
- collect class names
- collect interface names
- collect enum names
- collect free function names
- collect method signatures
- collect constructor overload sets

No deep validation yet beyond duplicate-name detection.

### Pass 5: Semantic Analysis

This is the most important pass.

Responsibilities:

- resolve types
- resolve names
- validate naming style rules
- validate control-flow restrictions
- validate enum naming and trailing `...N`
- validate constructor overloads
- validate interface implementation
- validate const-correctness where analyzable
- detect recursion
- detect narrowing conversions
- detect signed/unsigned mixing
- detect uninitialized locals
- detect duplicate writes in a single expression
- validate `for` restrictions
- validate `switch default`
- validate no declaration interleaving
- validate built-in `maybe<T>` usage

This pass should annotate the AST rather than mutate it heavily.

Recommended outputs:

- resolved type on every expression
- resolved symbol on every identifier use
- class lifetime metadata
- per-scope declaration lists
- warning/error list

For `maybe<T>`, semantic analysis should also compute a lightweight flow-sensitive fact set:

- whether a specific `maybe` object is known to contain a value on a given control-flow edge

### Pass 6: Lifetime Analysis

Responsibilities:

- compute object lifetime boundaries for class-typed locals
- record which scopes own which destructible objects
- compute cleanup states for each early return path
- determine which cleanup labels are needed

This pass should still remain language-level, not C-level.

### Pass 7: Lowering

Transform AST from c+ surface constructs into a C-oriented IR or lowered AST.

Responsibilities:

- lower constructor sugar
- lower implicit enum `ToString`
- lower built-in `maybe<T>` into a concrete C representation
- lower methods into namespaced functions
- lower private methods into internal functions
- lower static class variables into file-scope `static`
- lower namespace-qualified names into mangled symbols
- lower RAII returns into cleanup-label form

At this point, the representation should be extremely close to C.

## `maybe<T>` Lowering

Recommended lowered representation:

```c
typedef struct {
    bool has_value;
    T value;
} __cplus_maybe_T;
```

The exact emitted type name can be mangled per `T`.

Recommended lowering rules:

- `maybe<T>` becomes a generated struct containing a presence flag and storage for `T`
- `.has_value()` lowers to reading the flag
- `.value()` lowers to reading the stored value
- compile-time safety is enforced before lowering; generated C does not need an additional proof system

## Lowered IR Recommendation

Do not emit C directly from the semantic AST.

Create a small lowered IR such as:

- `CModule`
- `CStruct`
- `CEnum`
- `CFunctionDecl`
- `CFunctionDef`
- `CStatement`
- `CExpr`

This IR does not need to represent all of C.
It only needs to represent the subset your transpiler emits.

This helps with:

- cleaner C generation
- easier testing
- easier RAII lowering
- easier future refactors

## Pass 8: Header Emission

Emit `.h` first from lowered IR.

Responsibilities:

- include guards or `#pragma once`
- struct definitions
- enum definitions
- public method declarations
- free function declarations
- generated enum `ToString` declarations
- comments for conceptually private fields

Do not emit:

- private method declarations
- class static variables

## Pass 9: Source Emission

Emit `.c` from lowered IR.

Responsibilities:

- include matching generated header first
- emit static variables
- emit private methods as `static`
- emit public method definitions
- emit free function definitions
- emit enum `ToString` implementations
- emit cleanup labels at end of functions where needed

## Namespace Handling

Use a LIFO namespace stack during parsing.

Example:

```c+
namespace Board {
    namespace Spi {
        class CubeSpi {
        }
    }
}
```

Parser behavior:

1. push `Board`
2. push `Spi`
3. parse `CubeSpi` with namespace path `[Board, Spi]`
4. pop `Spi`
5. pop `Board`

The compact form:

```c+
namespace Board::Spi {
}
```

should produce exactly the same namespace path.

Mangling should happen only during lowering/emission:

- `Board::Spi::CubeSpi` -> `Board___Spi___CubeSpi`

## Enum Handling

Enums should be processed in this order:

1. parse enum syntax
2. validate enum name style
3. validate each member name style
4. validate trailing `...N`
5. synthesize implicit `ToString`
6. emit enum and helper declaration/definition

Generated `ToString` should be a simple `switch` over enum values returning string literals.

## RAII Handling

RAII should be implemented after semantic analysis, not during parsing.

Recommended order:

1. resolve all local variable declarations
2. mark class-typed locals needing destruction
3. compute scope exit cleanup sequences
4. rewrite each `return`
5. emit chained cleanup labels at function end

This avoids mixing syntax parsing with control-flow rewriting.

## Suggested Internal Layout

Suggested C++ project layout:

```text
src/
  main.cpp
  driver/
    cli.cpp
    cli.h
  lex/
    token.h
    lexer.cpp
    lexer.h
  parse/
    parser.cpp
    parser.h
  ast/
    ast.h
  sema/
    symbols.cpp
    symbols.h
    analyze.cpp
    analyze.h
  lower/
    lower.cpp
    lower.h
    raii.cpp
    raii.h
  emit/
    emit_c.cpp
    emit_c.h
  support/
    diagnostics.cpp
    diagnostics.h
    files.cpp
    files.h
    strings.cpp
    strings.h
```

## STL Usage

Using STL here is fine.

Recommended:

- `std::vector` for ordered child lists
- `std::string` and `std::string_view` for text handling
- `std::unordered_map` for symbol lookup where order does not matter
- `std::map` only where stable ordering helps diagnostics or emission
- `std::optional` for nullable semantic results
- `std::variant` only if it genuinely simplifies AST or IR nodes

Avoid:

- overengineering with visitor hierarchies too early
- excessive template abstractions
- clever allocator work

Keep it boring.

## Diagnostics Strategy

Use one central diagnostics sink.

Every pass should be able to emit:

- error
- warning
- note

Diagnostics should include:

- file
- line
- column
- short message
- optional related span

Stop on parse errors.
Continue on most semantic errors so you can report several at once.

## Testing Strategy

For a weekend project, prioritize snapshot tests.

Useful test categories:

- lexing
- parsing
- name/style validation
- interface checks
- enum `ToString` generation
- constructor lowering
- RAII cleanup lowering
- full-file `.cp` -> `.c` snapshots
- `.hp` -> `.h` snapshots

Best simple test style:

- input file
- expected diagnostics
- expected generated C or header

## Recommendation Summary

Use C++ for the transpiler.

Do passes in this order:

1. discovery
2. lex
3. parse
4. normalize
5. collect symbols
6. semantic analysis
7. lifetime analysis
8. lowering
9. emit `.h`
10. emit `.c`

Namespaces are parsed early, resolved in semantics, and mangled late.
That is the right order.
