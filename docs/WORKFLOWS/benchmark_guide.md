# Benchmark Guide (2026-01-04)

## SMv2 Dataset Structure

The `benchmarks/sm1000/` directory contains 1000 LTLf synthesis formulas split across two directories:

```
benchmarks/sm1000/
├── bench1/           # First 500 formulas
│   ├── f1.ltlf
│   ├── f1.part
│   ├── f2.ltlf
│   ├── f2.part
│   ...
│   ├── f500.ltlf
│   └── f500.part
├── bench2/           # Second 500 formulas
│   ├── f1.ltlf
│   ├── f1.part
│   ...
│   └── f500.ltlf
└── results.csv       # Expected results (Realizable/Unrealizable)
```

**Important**: Both `bench1/` and `bench2/` contain formulas numbered `f1` through `f500`.
These are two distinct sets of 500 formulas each, totaling 1000 unique formulas.

## File Format

### `.ltlf` File (Formula)
Contains the LTLf formula as a single line:
```
(X(F(p1))) U (X(X(G(p1))))
```

### `.part` File (Partition)
Defines input/output variable partitioning:
```
.inputs: p1 p3
.outputs: p5 p7
```

**Rules**:
- `.inputs:` must come BEFORE `.outputs:`
- Even empty `.inputs:` must be written (as `.inputs: `)
- Variables appearing in the formula but not in the partition are treated as outputs

## Running Benchmarks

### Basic Usage

```bash
# Run all 1000 formulas (both bench1 and bench2)
./build/tests/bench/benchmark_test benchmarks/sm1000 all

# Run f1-f10 from both directories
./build/tests/bench/benchmark_test benchmarks/sm1000 all 1 10

# Run only bench1 formulas
./build/tests/bench/benchmark_test benchmarks/sm1000 1 1 500

# Run only bench2 formulas
./build/tests/bench/benchmark_test benchmarks/sm1000 2 1 500
```

### Command Line Arguments

```
./benchmark_test <base_dir> [bench_spec] [start] [end]
```

| Argument | Description | Default |
|----------|-------------|---------|
| base_dir | Benchmark directory (e.g., `benchmarks/sm1000`) | `benchmarks/sm1000` |
| bench_spec | `all`, `1`, or `2` - which bench directories to use | `all` |
| start | Starting formula number (1-500) | `1` |
| end | Ending formula number (1-500) | `500` |

### Output Format

```
OK: bench1/f1 (0.054ms)
  Formula: (X(F(p1))) U (X(X(G(p1))))
  Partition: outputs: [p1]
  Expected: Realizable

OK: bench2/f1 (0.032ms)
  Formula: (G(F(!(p3)))) R (F(G(!(p6))))
  Partition: inputs: [p3], outputs: [p6]
  Expected: Realizable

--- Progress: 10 formulas processed ---
```

## Current Status (2026-01-04)

### Test Results (1000 formulas)

| Metric | Count | Percentage |
|--------|-------|------------|
| **Parsed successfully** | 689 | 68.9% |
| **Parse failed** | 311 | 31.1% |
| **Total time** | 17.4ms | - |

### Parse Failure Reasons

Most parse failures are due to the `->` (implies) operator, which is not currently supported:

```
FAIL: bench1/f4 (parse error)
  Formula: ((F(G(!(p3)))) -> (G(F(!(p1))))) U ((p2) & (p5))
```

**Unsupported operators**:
- `->` (implies / IMPLIES)
- `<->` (iff / IFF)

**Workaround**: These can be manually rewritten using supported operators:
- `a -> b` ≡ `(!a) | b`
- `a <-> b` ≡ `(a & b) | ((!a) & (!b))`

## Integration with Cosy Reference

Cosy reference implementation path:
```
/home/lic/files/rewrite_ltlf_codes/Cosy_rewrite/Cosy <ltlf_file> <part_file> <comb_idx>
```

Where `comb_idx` is:
- `0` for Individual Composition
- `1` for Incremental Composition
