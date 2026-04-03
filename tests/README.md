# Tests

This folder holds manual and future automated tests for the `c+` transpiler.

Current layout:

- `smoke/basic/input/` contains a small end-to-end sample
- `smoke/basic/expected/` contains the current expected generated C output
- `smoke/run_smoke.sh` rebuilds the transpiler, runs the smoke input, and compares output
- `run_tests.sh` runs the passing feature suite
- `run_private_methods.sh` checks private method lowering
- `run_body_parsing.sh` keeps the older direct body parsing slice
- `run_body_parsing_paired.sh` checks the real paired `.hp` + `.cp` module path
- `run_gap_tests.sh` pins spec gaps that are still missing from `src/`
- `run_out_of_class_methods.sh` checks paired-module out-of-class method definitions
- `run_field_access_rewriting_gap.sh` is a snapshot regression for `self->field` rewriting in method bodies
- `run_out_of_class_method_calls_gap.sh` checks same-class call rewriting in paired out-of-class method bodies
- `run_out_of_class_object_method_calls_gap.sh` checks method-call rewriting on object fields and local objects
- `run_raii_return.sh` checks first-slice RAII cleanup insertion on explicit `return`
- `run_out_of_class_object_method_calls_gap.sh` is a snapshot regression for object-field and local-object method-call lowering

Paired-module rule:

- matching `.hp` and `.cp` files with the same stem are one logical `c+` module
- the `.hp` file carries declarations and the `.cp` file carries definitions
- tests that want a full module should pass the directory that contains both files, not a single file

The field-access rewriting regression is still kept as a lightweight snapshot because its positive behavior is also covered by the out-of-class method tests.

The object-method-call regression is likewise a snapshot of desired emitted C. It pins `Board___Logger___Flush(&self->logger);` and `Board___Logger___Flush(&logger);` for the next lowering pass.

The smoke test is intentionally small. It exists to prove that the current vertical slice still works after changes to parsing, semantics, lowering, or emission.
