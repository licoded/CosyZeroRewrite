# Benchmarks TODO

## 2026-01-05 - benchmark_test Error

**Issue**: `./build/tests/bench/benchmark_test` has an error:
```
FAIL: bench2/f498 (file not found)
```

**Action items**:
1. Investigate the `bench2/f498` test case
2. Check if the file exists in the benchmark directory
3. Fix the file path or add the missing file
4. Re-run benchmark test to verify fix

**Command to reproduce**:
```bash
./build/tests/bench/benchmark_test tools/benchmarks/sm1000 1 1000
```
