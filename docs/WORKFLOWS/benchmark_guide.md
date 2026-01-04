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

## Current Status (2026-01-04 23:58)

### Test Results (1000 formulas)

| Metric | Count | Percentage |
|--------|-------|------------|
| **Parsed successfully** | 1000 | 100% |
| **Parse failed** | 0 | 0% |
| **Total time** | 24.3ms | - |

**✅ All formulas now parse successfully!** (After adding `->` operator support)

### Implies Operator Support (2026-01-04)

The `->` (implies) operator is now fully supported. During parsing, `a -> b` is
automatically converted to `!a | b` (material implication).

**Implementation Details**:
- **Lexer**: Recognizes `->` as `TokenType::Implies`
- **Parser**: `parse_implies_expr()` handles right-associative implies chains
- **Transformation**: `a -> b -> c` becomes `!a | (!b | c)` (right-associative)
- **Precedence**: Implies has lower precedence than `|`, higher than `U`/`R`

**Grammar Addition**:
```
implies_expr ::= or_expr ('->' or_expr)*
```

### Previously Failed Cases (Now Fixed)

The following cases that previously failed due to missing `->` support now parse correctly:

#### Simple Implies Cases (Now Fixed)

```
✓ bench1/f4: ((F(G(!(p3)))) -> (G(F(!(p1))))) U ((p2) & (p5))
  Partition: inputs: [p1, p2], outputs: [p3, p5]
  Expected: Unrealizable

✓ bench1/f5: (G(!(F(!(p4))))) -> (G(F(X(X(F(!(p2)))))))
  Partition: inputs: [p2], outputs: [p4]
  Expected: Realizable
```

#### Nested Implies Cases (Now Fixed)

```
✓ bench1/f445: (p9) -> ((!(p3)) -> ((p2) -> (p4)))
  Partition: inputs: [p2, p3], outputs: [p4, p9]
  Expected: Realizable

✓ bench1/f449: ((((F(p6)) -> (p9)) R (p1)) -> (p9)) | ((p6) R (G((F(p9)) U (p6))))
  Partition: inputs: [p1], outputs: [p6, p9]
  Expected: Realizable
```

#### Complex Implies with Temporal Operators (Now Fixed)

```
✓ bench1/f464: ((p8) U (!(p2))) U ((F(p1)) -> (!((X(p5)) U (G(F(p0))))))
  Partition: inputs: [p0, p1], outputs: [p2, p5, p8]
  Expected: Unrealizable

✓ bench2/f40: ((p0) R (F(G((p2) & (F(G(F(G(p7))))))))) -> (((p5) & (p8)) R ((F((p6) R (X(G(p0))))) U ((G(X(p9))) U (p1))))
  Partition: inputs: [p0, p1, p2, p5], outputs: [p6, p7, p8, p9]
  Expected: Realizable
```

#### Implies Inside X/G/F Operators (Now Fixed)

```
✓ bench2/f117: X(G(F((p6) -> (F(p6)))))
  Partition: outputs: [p6]
  Expected: Realizable

✓ bench2/f183: X((X(X(p3))) -> (X(F(p1))))
  Partition: inputs: [p1], outputs: [p3]
  Expected: Realizable
```

#### Implies Combined with `true`/`false` (Now Fixed)

```
✓ bench2/f82: (X((X((false))) U (F(p2)))) -> ((!((p3) | ((!(p2)) U (p2)))) U ((p1) R ((p1) | (X(p0)))))
  Partition: inputs: [p0, p1], outputs: [p2, p3]
  Expected: Realizable

✓ bench2/f120: ((X((false))) -> (!(p1))) -> (((p4) & (p9)) R (p6))
  Partition: inputs: [p1, p4], outputs: [p6, p9]
  Expected: Unrealizable
```

### Historical Note (Before Implies Support)

**Before** 2026-01-04 23:58, the parse failure distribution was:

| Directory | Failed | Total | Failure Rate |
|-----------|--------|-------|--------------|
| bench1    | ~155  | 500   | ~31%         |
| bench2    | ~156  | 500   | ~31%         |
| **Total** | **311** | **1000** | **31.1%** |

All 311 failures were due to missing `->` operator support.

## Integration with Cosy Reference

Cosy reference implementation path:
```
/home/lic/files/rewrite_ltlf_codes/Cosy_rewrite/Cosy <ltlf_file> <part_file> <comb_idx>
```

Where `comb_idx` is:
- `0` for Individual Composition
- `1` for Incremental Composition
