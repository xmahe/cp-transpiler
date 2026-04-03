# Tests

This folder holds manual tests plus CTest-driven automated tests for the `c+` transpiler.

How to run from the repository root:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

CTest fixture tests are always the primary automated suite.

If GoogleTest is available on the machine and `CPLUS_ENABLE_GTEST` is enabled, CMake also registers a small unit-test target automatically. If GTest is not available, the end-to-end fixture suite still runs normally.

Current layout:

- `smoke/basic/input/` contains a small end-to-end sample
- `smoke/basic/expected/` contains the current expected generated C output
- `CMakeLists.txt` registers fixture tests with CTest and optional unit tests with GoogleTest
- `ctest_fixture.sh` is the shared helper that runs the built compiler against a fixture directory and compares output
- `smoke/run_smoke.sh`, `run_tests.sh`, and the other `run_*.sh` files are convenience wrappers for manual reruns; they are no longer the primary automated entrypoint

Paired-module rule:

- matching `.hp` and `.cp` files with the same stem are one logical `c+` module
- the `.hp` file carries declarations and the `.cp` file carries definitions
- tests that want a full module should pass the directory that contains both files, not a single file

The field-access rewriting regression is still kept as a lightweight snapshot because its positive behavior is also covered by the out-of-class method tests.

The object-method-call regression is likewise a snapshot of desired emitted C. It pins `Board___Logger___Flush(&self->logger);` and `Board___Logger___Flush(&logger);` for the next lowering pass.

The smoke test is intentionally small. It exists to prove that the current vertical slice still works after changes to parsing, semantics, lowering, or emission.

Current status:

- the main suite is fixture-driven and runs through `ctest`
- unit coverage exists, but it is intentionally small compared to the fixture suite
- this test tree is aimed at guarding real transpiler output, not just helper functions
