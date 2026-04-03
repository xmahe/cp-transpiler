# Next Step Plan

## Goal

Move the `c+` transpiler from declaration-only parsing toward real file support by parsing function and method bodies structurally instead of treating them as raw text.

Right now the compiler can validate and lower a narrow declaration slice. The next meaningful milestone is:

- parse statements
- parse expressions
- attach them to function and method bodies
- move semantic checks away from text matching
- prepare real lowering of bodies into C IR

## Why This Is The Next Step

Without body parsing, the compiler cannot honestly handle real source files.

Current limitations:

- function bodies are mostly opaque strings
- forbidden-Construct checks are text-based
- `maybe<T>` safety checks are text-based
- RAII lowering cannot be implemented cleanly
- method calls and expression rewriting do not really exist yet

So the next step is not “more declarations”. It is body structure.

## Implementation Order

### Phase 1: AST Expansion

Add syntax-tree nodes for:

- statements
- expressions
- block bodies

Minimum statement set:

- block statement
- declaration statement
- expression statement
- `if`
- `return`
- `while`
- restricted `for`
- `switch`
- `break`

Minimum expression set:

- identifier
- literal
- member access
- call
- assignment
- binary operator
- unary operator

Keep this small. The goal is not to parse all of C.

### Phase 2: Parser Entry

Teach the parser to parse function and method bodies into the new AST.

Initial target:

- free function bodies
- later class method bodies when they get real definitions

The parser should still reject forbidden constructs, but now through syntax rather than text scanning where possible.

### Phase 3: Semantic Hooks

Move these checks onto the body AST:

- forbidden `goto`
- forbidden `continue`
- forbidden `++` / `--`
- forbidden ternary
- `switch` requires `default`
- `maybe<T>.value()` proof tracking

The current text-based checks can remain temporarily as a fallback, but the goal is to replace them.

### Phase 4: Lowering Hooks

Once function bodies exist as AST:

- lower declarations
- lower calls
- lower member access
- lower returns
- lower cleanup labels

This is where real transpilation starts.

### Phase 5: Tests

Add tests for:

- parsed `if` / `else`
- parsed `return`
- parsed variable declarations
- forbidden body constructs
- future RAII cleanup lowering
- future `maybe<T>` control-flow proof

## Agent Split

### Agent A: AST + Parser

Ownership:

- `src/ast/*`
- `src/parse/*`

Responsibilities:

- statement AST
- expression AST
- parser stubs and minimal parsing for bodies

### Agent B: Tests

Ownership:

- `tests/body-*`
- `tests/run_*`

Responsibilities:

- fixtures for body parsing
- diagnostics expectations
- small smoke-style body tests

### Main Integrator

Responsibilities:

- keep plan and architecture aligned
- reconcile parser interfaces with existing driver/sema
- rebuild and validate

## Immediate Success Criteria

The next iteration is successful if:

1. function bodies are no longer only raw text
2. the AST can represent basic statements and expressions
3. the parser can build those nodes for a small subset
4. the compiler still builds
5. tests cover the new body-parsing slice

## Not Required Yet

These can wait one more round:

- full operator precedence coverage
- full RAII lowering
- full `maybe<T>` CFG analysis
- full C interop inside bodies
- complete method-definition parsing outside declarations

## Short Version

The next step is:

- build a real body AST
- parse bodies
- then move semantics and lowering onto that structure

That is the bridge from “language sketch that emits some C” to “real transpiler”.
