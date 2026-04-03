# c+ Detailed Transpiler Implementation Plan

## Purpose

This is the master roadmap for getting the `c+` transpiler to v1.0.

It is no longer a blank-slate scaffold plan. The compiler already exists, builds through CMake, and has real end-to-end coverage. This document should answer two questions:

- what is already done
- what still blocks v1.0

## v1.0 Status

### Done

- C++ compiler implementation under `src/`
- pass-oriented structure: discovery, lex, parse, sema, lower, emit
- dedicated lowered IR
- paired `.hp` / `.cp` module handling
- namespace lowering and mangling
- enums with generated `ToString`
- classes, interfaces, and exact-match interface fulfillment checks
- compile-time DI with `inject` and `bind`
- constructor member initializers
- several RAII lowering slices for returns, fallthrough, nested return cases, and member subobjects
- fixture-based end-to-end tests through CTest
- optional GoogleTest unit tests
- CMake build

### In Progress

- structural parsing and lowering of function/method bodies
- replacing source-preserving body handling with AST-driven lowering
- strengthening `maybe<T>` path safety checks
- broadening RAII from current slices to a more complete scope model

### Remaining For v1.0

- robust statement/expression parsing for the supported language subset
- AST-driven body semantic analysis instead of text-based fallbacks
- fuller method-body lowering, especially beyond simple rewrites
- clearer and more reliable C interop rules
- a defined “supported real file” boundary with at least one realistic embedded example that stays green
- documentation cleanup so the language guide and compiler status match the implementation exactly

### Post-v1.0

- full dominance-style `maybe<T>` proof
- deeper const inference
- richer C parsing and analysis
- broader unit-test coverage
- anything that starts turning the transpiler into a much larger compiler project

## Implementation Language

Use modern C++ for the transpiler.

Reasons:

- `std::string` and `std::string_view` are enough
- `std::vector`, `std::optional`, and standard containers are enough
- `std::filesystem` simplifies discovery and output handling
- deployment stays simple on the same systems that already build embedded C/C++

The compiler project should use C++ pragmatically, not as a language playground.

## Pass Order

The transpiler should run in this order:

1. Discovery
2. Lex
3. Parse
4. Normalize
5. Collect Symbols
6. Semantic Analysis
7. Lifetime Analysis
8. Lower
9. Emit Header
10. Emit Source

This order matters.

### Why Namespaces Are Not “First” or “Last”

Namespaces should be handled in three stages:

1. parsed as syntax
2. resolved as semantic context
3. mangled during lowering/emission

So the answer is:

- parse namespace blocks early
- validate namespace naming and scope rules during semantic analysis
- apply `___` symbol mangling late

## Agent-Based Work Split

The initial implementation should be done with disjoint write ownership.

### Agent 1: Frontend and Support

Ownership:

- `src/main.cpp`
- `src/driver/*`
- `src/lex/*`
- `src/parse/*`
- `src/ast/*`
- `src/support/*`

Responsibilities:

- CLI entrypoint
- source discovery
- diagnostics sink
- token definitions
- lexer
- parser shell
- AST data structures

### Agent 2: Semantics and Backend

Ownership:

- `src/sema/*`
- `src/lower/*`
- `src/emit/*`

Responsibilities:

- symbol tables
- semantic analysis entrypoints
- naming validation hooks
- enum and `maybe<T>` analysis hooks
- lowered C IR
- RAII lowering hooks
- header/source emitters

### Main Integrator

Responsibilities:

- keep specs aligned
- write plan and architecture docs
- integrate both agent outputs
- check interface consistency between modules
- decide remaining API seams

## Milestone Breakdown

### Milestone 0: Skeleton `[done]`

- `src/` tree exists
- main executable entrypoint exists
- module boundaries are clear

### Milestone 1: Frontend Basics `[done]`

- token model
- lexer API
- AST node definitions
- parser entrypoint

### Milestone 2: Semantic Shape `[done]`

- symbol collection
- core semantic analysis driver
- diagnostics for naming, namespace shape, forbidden constructs, enum rules, interfaces, and several class rules

### Milestone 3: Lowering and Emission `[done]`

- lowered IR
- header emitter
- source emitter
- enum lowering
- namespace mangling
- class and method lowering

### Milestone 4: RAII, `maybe<T>`, and DI `[in progress]`

- generated C representation for `maybe<T>` `[done]`
- lightweight `maybe<T>` safety checking `[done]`
- compile-time DI with `inject`/`bind` `[done]`
- cleanup label generation for current supported cases `[done]`
- broader AST-driven lifetime handling `[todo]`
- stronger control-flow proof for `maybe<T>` `[todo]`

### Milestone 5: Real Body Support `[in progress]`

- body AST exists in part `[done]`
- some body rewrites already work `[done]`
- full supported statement/expression subset still needs completion `[todo]`
- semantic checks still need to move fully off text-based fallbacks `[todo]`

### Milestone 6: v1.0 Hardening `[todo]`

- document supported language boundary precisely
- harden C interop expectations
- expand realistic fixture coverage
- close the biggest body-lowering gaps

## Immediate Coding Priorities

The first code should not attempt to solve everything.

Priority order:

1. Diagnostics
2. Token and AST definitions
3. Parser surface skeleton
4. Semantic pipeline shell
5. Lowered IR shell
6. Emit shell

That creates the compiler shape early.

## Internal APIs

Keep APIs narrow and boring.

Recommended interfaces:

- `driver::RunCli(...)`
- `lex::LexFile(...)`
- `parse::ParseModule(...)`
- `sema::CollectSymbols(...)`
- `sema::AnalyzeModule(...)`
- `lower::LowerModule(...)`
- `emit::EmitHeader(...)`
- `emit::EmitSource(...)`

Diagnostics should be threaded through all major passes.

## Key Data Structures

### Source and Diagnostics

- `SourceFile`
- `SourceSpan`
- `Diagnostic`
- `DiagnosticSink`

### Frontend

- `TokenKind`
- `Token`
- `Lexer`
- `Parser`
- `ast::Module`

### Semantics

- `SymbolTable`
- `ResolvedType`
- `AnalysisResult`

### Lowering

- `CModule`
- `CStruct`
- `CEnum`
- `CFunction`

## Main v1.0 Blockers

These are the main things still standing between the current compiler and a credible v1.0:

- function and method bodies are still partly source-preserving
- body semantic checks are not fully AST-driven yet
- RAII lowering does not yet cover the full intended scope model
- `maybe<T>` analysis is still lightweight
- C interop exists as a design goal, but is not deeply modeled

## Post-v1.0 Problems To Defer

- full expression parsing fidelity beyond the supported subset
- full C interop parsing
- complete recursion detection
- deep const inference
- full `maybe<T>` dominance analysis
- more advanced member-subobject constructor binding rules

## Rules That Must Be Represented Early

Even if not fully enforced yet, these need explicit places in the code:

- namespace stack handling
- naming conventions
- enum `...N`
- enum `ToString`
- `maybe<T>` special treatment
- constructor-only overloading
- RAII cleanup states
- exact-match interface fulfillment

## Immediate v1.0 Priorities

1. Finish the supported body AST and parser slice
2. Move body checks from text matching to AST-based analysis
3. Expand method-body lowering beyond current rewriting shortcuts
4. Harden RAII cleanup planning across scopes
5. Tighten C interop behavior and tests
