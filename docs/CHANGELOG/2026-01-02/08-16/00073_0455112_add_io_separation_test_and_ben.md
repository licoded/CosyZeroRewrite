# [73] feat: add I/O separation test and benchmark runner

**Commit**: `0455112` ([`0455112924f2567eb58bf4ef9772ecc2e234bcf4`](https://github.com/licoded/CosyZeroRewrite/commit/0455112924f2567eb58bf4ef9772ecc2e234bcf4))
**Date**: 2026-01-02 11:50:23 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Add io_separation_test.cpp to verify input/output variable handling
  - Tests .part file parsing (SMv2 format)
  - Tests variable ID ordering (outputs first, then inputs)
  - Tests synthesis with separated variables

- Add benchmark_runner.cpp for SMv1000 benchmark testing
  - Loads .ltlf and .part files from benchmark directories
  - Compares with expected results from results.csv
  - Saves detailed results to CSV with timing
  - Preprocesses -> (implication) syntax

- Add visualize_results.py for result visualization
  - Generates confusion matrix, time distribution charts
  - Creates per-folder analysis
  - Produces HTML report with all visualizations
  - Uses matplotlib for charts

- Update CMakeLists.txt:
  - Enable BUILD_BENCHMARK by default
  - Change benchmark_runner path to tools/benchmark_runner.cpp

- Update cmake/Tests.cmake to include io_separation_test

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `tests/io_separation_test.cpp`
- `tools/benchmark_runner.cpp`
- `tools/visualize_results.py`


### Modified
- `CMakeLists.txt`
- `cmake/Tests.cmake`
- `.gitignore`


## Stats

- **6** files changed
- **986** insertions(+)
- **3** deletions(-)
