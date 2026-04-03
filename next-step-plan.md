# Next Step Plan

## Already Done

- pass-oriented compiler skeleton
- lexer, parser, sema, lowering, and emission pipeline
- lowered IR
- namespace, class, interface, enum, and `maybe<T>` support
- compile-time DI with `inject` / `bind`
- member initializer support
- quoted-path `import` plus import-closure semantic resolution
- qualified local object lowering
- several RAII slices
- CMake + CTest + optional GoogleTest

## Next Step

Move body handling further away from source-preserving text and toward a real AST-driven implementation.

The current highest-value slice is:

- complete the supported statement/expression AST
- attach that structure consistently to function and method bodies
- move body semantic checks away from text matching
- prepare fuller lowering of bodies into C IR

This is still the main bridge between the current vertical slice and a credible v1.0 transpiler.

## Implementation Order

### Phase 1: AST Expansion `[in progress]`

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

### Phase 2: Parser Entry `[in progress]`

Teach the parser to parse function and method bodies into the new AST.

Initial target:

- free function bodies
- later class method bodies when they get real definitions

The parser should still reject forbidden constructs, but now through syntax rather than text scanning where possible.

### Phase 3: Semantic Hooks `[todo]`

Move these checks onto the body AST:

- forbidden `goto`
- forbidden `continue`
- forbidden `++` / `--`
- forbidden ternary
- `switch` requires `default`
- `maybe<T>.value()` proof tracking

The current text-based checks can remain temporarily as a fallback, but the goal is to replace them.

### Phase 4: Lowering Hooks `[todo]`

Once function bodies exist as AST:

- lower declarations
- lower calls
- lower member access
- lower returns
- lower cleanup labels

This is where real transpilation starts.

### Phase 5: Tests `[todo]`

Add tests for:

- parsed `if` / `else`
- parsed `return`
- parsed variable declarations
- forbidden body constructs
- future RAII cleanup lowering
- future `maybe<T>` control-flow proof

## v1.0 Remainder

After this body-focused slice, the main v1.0 remainder is:

- stronger AST-driven RAII cleanup planning
- stronger `maybe<T>` analysis
- clearer and better-tested C interop behavior
- more realistic end-to-end examples
- final status/documentation cleanup around the supported feature boundary

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
- `import` graph loading and validation
- complete method-definition parsing outside declarations

## Short Version

The next step is still:

- finish the real body AST
- parse bodies more fully
- move semantics and lowering onto that structure

That is still the cleanest route from the current vertical slice to v1.0.
