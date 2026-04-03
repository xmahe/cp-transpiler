# src

This folder contains the `c+` transpiler.

If you do not know much about compilers, the important idea is this:

- the transpiler does not do everything in one giant pass
- it transforms the code step by step
- each folder is one step in that transformation

Think of it like a factory line:

1. read the source files
2. break text into small pieces
3. understand the structure
4. check that the structure makes sense
5. rewrite `c+` ideas into plain C ideas
6. print `.h` and `.c`

That is why the code is split by compiler stage instead of by language feature.

## One Tiny Example

Suppose the input is:

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

The stages roughly see it like this:

- `lex`: `"namespace"`, `"Board"`, `"{"`, `"class"`, `"Thermometer"`, ...
- `parse`: "this is a namespace, containing a class and a function definition"
- `sema`: "the out-of-class `Thermometer::Reset` matches the declared method"
- `lower`: "methods become plain C functions, `reading` becomes `self->reading`"
- `lower`: "`inject` slots are replaced by concrete bound types, and constructor member initializers become explicit `Construct` calls"
- `emit`: write something like:

```c
typedef struct Board___Thermometer {
    u32 reading;
} Board___Thermometer;

void Board___Thermometer___Reset(Board___Thermometer* self) {
    self->reading = 0;
}
```

That is the whole job of this tree.

## The Folders

### [main.cpp](/Users/magnus/c+/src/main.cpp)

This is the executable entry point.

It should stay small.

Its job is:

- read command-line arguments
- create the diagnostics object
- call the driver

If language logic starts piling up here, something is in the wrong place.

### [driver](/Users/magnus/c+/src/driver)

This is the conductor.

It does not implement most language rules. It just decides:

- which files belong to one module
- when lex/parse/sema/lower/emit run
- where generated files are written

Current example:

- `demo.hp` and `demo.cp` with the same stem are treated as one logical module
- the driver reads both, parses both, merges them, then emits one `demo.h` and one `demo.c`
- `inject` and `bind` are resolved during analysis and lowering so interface slots become concrete fields in generated C

If you want to understand the whole compiler flow first, start here.

### [support](/Users/magnus/c+/src/support)

Small shared helpers.

This is the "boring but necessary" folder:

- file reading
- string helpers
- diagnostics printing

Example:

- `Diagnostics` turns internal compiler errors into readable terminal output like:
  `error: file.cp:12:5: goto is forbidden`

### [lex](/Users/magnus/c+/src/lex)

The lexer.

The lexer turns raw text into tokens.

Example input:

```c
fn Reset() -> void;
```

Example token stream:

- `fn`
- `Reset`
- `(`
- `)`
- `->`
- `void`
- `;`

The lexer does not know what a valid method is.
It only knows how to split text into useful pieces.

If a problem is about characters and spelling, it is usually here.

### [ast](/Users/magnus/c+/src/ast)

The AST is the syntax tree.

AST means "abstract syntax tree", but you can just think:

- "the structured result of parsing"

This is still very close to the original `c+` source.

Example:

- a namespace node
- containing a class node
- containing field nodes and method declaration nodes

The AST should answer:

- what did the source say?

It should not try to answer:

- is it legal?
- what exact C should this become?

### [parse](/Users/magnus/c+/src/parse)

The parser.

The parser reads tokens and builds the AST.

This is where syntax rules live.

Examples:

- how `namespace X { ... }` is recognized
- how `fn Name(args) -> Type` is recognized
- how class bodies are split into fields and methods

Good question for `parse`:

- "does this text fit the grammar?"

Bad question for `parse`:

- "does this class really implement that interface?"

That second question belongs in `sema`.

### [sema](/Users/magnus/c+/src/sema)

Semantic analysis.

This is where the compiler starts understanding meaning.

Examples:

- interface fulfillment
- `inject` slot binding checks
- constructor member-initializer validation
- enum naming rules
- style rules
- `maybe<T>` restrictions
- duplicate symbol detection

Example distinction:

- `parse` says: "yes, this looks like `fn Thermometer::Reset() -> void`"
- `sema` says: "yes, there really is a `Thermometer` class and this definition matches it"

If you are asking "is this allowed?", the answer is often in `sema`.

### [lower](/Users/magnus/c+/src/lower)

Lowering is the most compiler-ish part.

This is where `c+` constructs are rewritten into simpler C-shaped constructs.

Examples of lowering:

- namespaces become prefixes like `Board___`
- methods become plain C functions with an explicit `self`
- field access becomes `self->field`
- same-class calls become `Class___Method(self, ...)`
- object method calls become `OtherClass___Method(&object, ...)`
- constructor member initializers become ordered `Construct` calls on the generated struct fields

Example:

```c
reading = 0;
Reset();
logger.Flush();
```

can become:

```c
self->reading = 0;
Board___Thermometer___Reset(self);
Board___Logger___Flush(&self->logger);
```

If the emitter has to know too much about `c+`, lowering is not doing enough work.

### [emit](/Users/magnus/c+/src/emit)

This is the final printer.

It takes the lowered internal form and writes actual `.h` and `.c` files.

By the time code reaches this layer, most hard decisions should already be done.

This layer should mostly be:

- print struct
- print enum
- print function declaration
- print function body lines

If `emit` starts having lots of special-case language logic, that is usually a warning sign.

## What To Open First

If you want to change...

- CLI or module/file handling: start in [driver](/Users/magnus/c+/src/driver)
- token recognition: start in [lex](/Users/magnus/c+/src/lex)
- language grammar: start in [parse](/Users/magnus/c+/src/parse)
- legality checks: start in [sema](/Users/magnus/c+/src/sema)
- how `c+` turns into C: start in [lower](/Users/magnus/c+/src/lower)
- final text formatting: start in [emit](/Users/magnus/c+/src/emit)

## A Practical Example: Adding A Feature

Suppose you add a new language rule:

- local class objects should clean up on `return`

The likely work split is:

1. `parse`
   If new syntax is needed, teach the parser about it.

2. `sema`
   Check that the rule is legal.
   Example: only real class types with `Destruct` participate.

3. `lower`
   Rewrite the body so generated C has cleanup labels and destructor calls.

4. `emit`
   Print the lowered cleanup code.

5. `tests`
   Add one small CTest case and one realistic end-to-end example.
   Shell wrappers are optional convenience helpers now, not the main test path.

That is the normal way to grow this compiler.

## The Most Important Mental Model

When you are unsure where something belongs, ask:

- is this about raw text?
  Then it is probably `lex`.
- is this about grammar shape?
  Then it is probably `parse`.
- is this about whether the code is valid?
  Then it is probably `sema`.
- is this about translating `c+` ideas into C ideas?
  Then it is probably `lower`.
- is this just printing text?
  Then it is probably `emit`.

## Current Reality

The codebase is now a real working pre-v1.0 compiler, not just a scaffold.

What is already real:

- pass-oriented `lex` / `parse` / `sema` / `lower` / `emit`
- paired `.hp` / `.cp` module handling
- namespace lowering
- interfaces plus compile-time `inject` / `bind`
- constructor member initializers
- several RAII lowering slices
- meaningful end-to-end coverage through CTest

Important remaining limitations before v1.0:

- body handling is still partly source-preserving, not fully AST-driven
- RAII cleanup lowering is only partly implemented
- C interop is important, but not yet deeply modeled in `sema`
- `maybe<T>` checking is still lighter than a full control-flow proof
- some emitted bodies still rely on simple rewriting instead of full expression lowering

So think of the current compiler as:

- a real transpiler with meaningful language support already working
- strong enough for guarded experiments and fixture-driven development
- not yet a complete v1.0 compiler

That is fine for where the project is.
