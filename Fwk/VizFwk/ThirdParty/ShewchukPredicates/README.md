# ShewchukPredicates

Jonathan Richard Shewchuk's adaptive precision floating-point arithmetic and fast robust geometric
predicates (`orient2d`, `orient3d`, `incircle`, `insphere`).

- Source: https://github.com/libigl/libigl-predicates
- Pinned commit: `decb7bc1260e689cbe008109e3cc5d3a5a433aea`
- Files: `predicates.c`, `predicates.h`, copied byte-for-byte from the commit above
- Original: https://www.cs.cmu.edu/~quake/robust.html
- License: public domain. The file header of `predicates.c` states "Placed in the public domain by
  Jonathan Richard Shewchuk". The libigl-predicates repository carries no SPDX license.

## Build notes

- Built as a static C library (`ShewchukPredicates`) with its own flags, outside of
  `CEE_STRICT_CXX_FLAGS`. Warnings are disabled, the code is vendored.
- FMA contraction must be off (`-ffp-contract=off` on GCC/Clang, `/fp:precise` on MSVC 2022). The
  error-free transformations in the code require every operation to be rounded individually.
  `-ffast-math` and `/fp:fast` must never be used.
- `C_STANDARD 99`: the K&R function definitions are rejected by C23 compilers.
- Do not call this library directly from application code. Use `cvfRobustPredicates.h` in
  `LibGeometry`, which handles `exactinit()` and provides a typed API.

## Updating

Replace the two files from a new pinned commit, update the hash above, and run
`LibGeometry_UnitTests` (`cvfRobustPredicates-Test.cpp`).

## Decisions (OPM/ResInsight#15163, step 1)

- **FMA contraction (OPM/ResInsight#15088):** `LibGeometry_UnitTests` and `ResInsight-tests` build the same
  `predicates.c` a second time with contraction enabled (`ShewchukPredicatesFmaProbe`, renamed symbols,
  `EXCLUDE_FROM_ALL`) and compare. On MSVC x64 (`/fp:contract /arch:AVX2`) the exact **sign** is identical on
  80000 random and nearly degenerate cases. The magnitude returned by `orient*Value` is only an estimate and
  differs in the last bits in about a quarter of the cases (adaptive stage A is evaluated differently), so only
  the sign (`cvf::robust::Orientation`) is part of the contract.
- **`custom-opm-common/.../external/resinsight` copies:** these live in the `CeetronSolutions/opm-common`
  submodule with their own copies of `LibCore`/`LibGeometry` sources and are not built against `LibGeometry`. They
  stay on the old (tolerance based) path. If the consumers in `ApplicationLibCode` (step 4) change behavior in
  `RigHexIntersectionTools`/`cvfGeometryTools`, the copies are updated separately in that submodule, or the
  predicates are linked via `ShewchukPredicates` at that time.
- **CI:** `LibGeometry_UnitTests` is not run in CI (only `AppFwkUnitTest.yml` for `Fwk/AppFwk`). The same test
  source is therefore also compiled into `ResInsight-tests` (`ApplicationLibCode/UnitTests/CMakeLists.txt`), which
  runs on all CI platforms.
- **Baseline timing (MSVC 2026 x64, /O2, 1M calls, UTM sized coordinates):**
  `orient3d` on generic points 15 ns/call (naive determinant 9 ns); on exactly coplanar points, which always
  reach the exact stage, 74 ns/call (naive 5 ns). The existing unit tests exercising well log extraction and hex
  intersection run in a few ms, so an end-to-end baseline (well log extraction, completion export) needs a real
  model and must be recorded before the step 4 consumers are changed.
- **Decision gate, stage B (C++23 port):** no. The vendored C library builds cleanly on all platforms with
  the flags above and the cost per call is small compared to the surrounding cell search. Revisit only if the
  step 4 benchmarks show the predicates as a measurable cost.