# Performance baselines

Performance baselines are opt-in diagnostics for developers and CI maintainers.
They are recorded as JSON artifacts and are **not** pass/fail gates for normal
smoke CI.

## Local smoke baseline

A lightweight baseline can be recorded without large datasets:

```bash
python3 tests/performance/record_baseline.py \
  --output build/performance-baselines/smoke-baseline.json \
  --label local-generated-fixture-smoke
```

The checked-in `docs/performance/baselines/smoke-baseline.json` records an
initial local run of this deterministic smoke workload. Treat it as a reference
point for trend discussions, not a threshold.

## Build or test command baseline

The same recorder can wrap a real configure, build, or CTest command:

```bash
python3 tests/performance/record_baseline.py \
  --output build/performance-baselines/ctest-smoke.json \
  --label ctest-smoke -- \
  ctest --test-dir build-vcpkg-release --output-on-failure -L smoke
```

A non-zero wrapped command still produces the JSON record before returning the
wrapped command's exit code. Optional CI jobs that collect these artifacts use
`continue-on-error` / `allow_failure` so early performance movement does not gate
merges.

## CTest hook

Configure with `-DLVR2_ENABLE_PERFORMANCE_BASELINES=ON` to register
`lvr2_performance_baseline_smoke`. The test writes
`<build>/performance-baselines/smoke-baseline.json` and is labeled
`performance;baseline`; it is off by default.

## Scan-project storage baselines

The `BaseIO` replacement must compare the old CRTP scan-project path with the
new `lvr2::io::storage`/`lvr2::io::scan::ProjectStore` path using matching
fixtures and build settings. A generated-fixture baseline can wrap the existing
scan-project example before deletion:

```bash
cmake --preset vcpkg-release \
  -DLVR2_BUILD_EXAMPLES=ON \
  -DLVR2_BUILD_TESTS=ON
cmake --build --preset build-vcpkg-release --target lvr2_examples_scanprojects_simple

python3 tests/performance/record_baseline.py \
  --output build/performance-baselines/baseio-crtp-scanproject-simple.json \
  --label baseio-crtp-scanproject-simple -- \
  bash -lc 'rm -rf build/perf-baseio-old && mkdir -p build/perf-baseio-old && cd build/perf-baseio-old && ../../build-vcpkg-release/bin/lvr2_examples_scanprojects_simple'
```

After the storage service path exists, record the equivalent `ProjectStore`
Directory/HDF5 checks through CTest or a focused smoke binary:

```bash
python3 tests/performance/record_baseline.py \
  --output build/performance-baselines/storage-projectstore-scanproject-simple.json \
  --label storage-projectstore-scanproject-simple -- \
  ctest --test-dir build-vcpkg-release --output-on-failure -R 'storage.*project.*(directory|hdf5)'
```

See `docs/io/baseio-removal-inventory.md` for the inventory and the required
Directory, HDF5, and fake-backend smoke scenarios.
