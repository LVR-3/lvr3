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
