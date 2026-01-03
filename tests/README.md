# Tests Directory Structure

This directory contains all test code organized by category.

## Directory Layout

```
tests/
├── formula/          # Core formula tests
│   ├── formula.cpp            # Basic formula operations
│   └── transformation.cpp     # Transformation equivalence tests
├── parser/           # Parser tests
│   └── parser.cpp             # Formula parser and checker
├── transformation/    # LTL transformation tests
│   ├── nnf.cpp                 # Negation Normal Form
│   ├── xnf.cpp                 # neXt Normal Form
│   └── next.cpp                # Next operator tests
├── automata/         # Automata construction tests
│   ├── dfa.cpp                 # DFA construction
│   └── tarjan.cpp              # Tarjan SCC algorithm
├── synthesis/        # Synthesis algorithm tests
│   ├── synthesis.cpp           # Basic synthesis
│   └── on_the_fly.cpp          # On-the-fly synthesis
├── integration/      # Integration tests
│   ├── prop_atoms.cpp          # Propositional atoms
│   ├── io_separation.cpp       # I/O separation
│   └── strategy.cpp            # Strategy extraction
├── fuzz/             # Fuzzing tests (standalone)
│   ├── nnf.cpp                 # NNF fuzzer
│   ├── xnf.cpp                 # XNF fuzzer
│   ├── random.cpp              # Random formula fuzzer
│   ├── driver.cpp              # Fuzz driver
│   ├── equivalence.cpp         # Equivalence checker
│   ├── harness.cpp             # Fuzz harness
│   ├── parser.cpp              # Parser fuzzer
│   └── transformations.cpp     # Transformation fuzzer
├── debug/            # Development/debug tests
│   ├── eventually_contradiction.cpp
│   └── failing.cpp
└── bench/            # Benchmark/stress tests
    ├── benchmark.cpp           # Benchmark runner
    └── stress.cpp              # Long-running stress test
```

## Output Structure

After building, executables are organized in `build/`:

```
build/
├── output/           # Tools
│   ├── Cosy2
│   └── benchmark_runner
└── tests/            # Test executables (mirrors tests/ structure)
    ├── formula/
    ├── parser/
    ├── transformation/
    ├── automata/
    ├── synthesis/
    ├── integration/
    ├── fuzz/
    ├── debug/
    └── bench/
```

## Running Tests

```bash
# From project root
cd build && cmake .. && make

# Run all tests
make test

# Run specific test
./tests/formula/formula
./tests/synthesis/on_the_fly

# Run with verbose output
./tests/parser/parser -s
```

## Naming Conventions

- **No `_test` suffix** - file names match directory context
- **`*.cpp`** - All test source files use `.cpp` extension
- **Disabled tests** - use `.disabled` extension (not compiled)
