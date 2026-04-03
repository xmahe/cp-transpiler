# c+ Detailed Transpiler Implementation Plan

## Purpose

This document turns the architecture and implementation specs into an execution plan for building the first `c+` transpiler.

The goal is not to finish the whole compiler in one pass. The goal is to build a skeleton that is correctly shaped for:

- parsing `c+`
- validating the language rules
- lowering to ordinary C
- remaining small enough for a weekend project

## Implementation Language

Use modern C++ for the transpiler.

Reasons:

- `std::string` and `std::string_view` are good enough
- `std::vector` and hash maps are sufficient
- `std::filesystem` simplifies source discovery
- deployment stays simple in the same environments that already build C/C++

The compiler project should use C++ pragmatically, not as a playground for advanced C++.

## Top-Level Plan

Implementation order:

1. Create project skeleton under `src/`
2. Implement diagnostics and file handling
3. Implement token model and lexer
4. Implement AST
5. Implement parser
6. Implement symbol collection
7. Implement semantic analysis
8. Implement lifetime analysis
9. Implement lowering to a C-oriented IR
10. Implement `.h` and `.c` emission
11. Add snapshot-style tests later

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

### Milestone 0: Skeleton

Deliverables:

- `src/` tree exists
- main executable entrypoint exists
- modules compile in principle as a scaffold

Success criteria:

- file boundaries and module responsibilities are clear
- no pass is forced to know too much too early

### Milestone 1: Lexing and AST

Deliverables:

- token model
- lexer API
- AST node definitions
- parser entrypoint

Success criteria:

- a file can be tokenized
- parser can return a `Module` or diagnostics

### Milestone 2: Semantic Shape

Deliverables:

- symbol collector
- semantic analysis driver
- diagnostics for obvious rule violations

Initial rules to enforce first:

- one top-level namespace block per file
- naming style
- no forbidden tokens/operators
- enum trailing `...N`
- `implements` / `implementation` shape

### Milestone 3: Lowering and Emission

Deliverables:

- lowered IR
- header emitter
- source emitter

Initial generation target:

- enums
- plain structs
- public method declarations
- stub function bodies

### Milestone 4: RAII and `maybe<T>`

Deliverables:

- lifetime analysis
- cleanup label generation
- `maybe<T>` flow-sensitive safety check
- generated C representation for `maybe<T>`

This milestone is where the transpiler becomes genuinely interesting.

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

## Hard Problems to Defer

Do not solve these in the first scaffold:

- full expression parsing fidelity
- full C interop parsing
- complete recursion detection
- deep const inference
- full `maybe<T>` dominance analysis
- complex member-subobject constructor argument binding

Stub the hooks now, solve the algorithms later.

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

## Deliverables For This Turn

This implementation pass should leave the repo with:

- a detailed implementation plan
- a populated `src/` tree
- pass-oriented C++ scaffolding
- clear API seams between frontend, semantics, lowering, and emission

It does not need to be feature-complete yet.

## Next Step After Scaffolding

After the scaffold is in place, the next serious implementation task should be:

1. finish the lexer
2. parse namespace/class/interface/enum declarations
3. enforce naming and structural rules
4. emit minimal `.h` / `.c`

That will prove the architecture before the harder RAII and `maybe<T>` flow analysis work.
