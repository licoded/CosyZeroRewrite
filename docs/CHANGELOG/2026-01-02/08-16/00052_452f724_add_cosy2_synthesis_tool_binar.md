# [52] feat: add Cosy2 synthesis tool binary

**Commit**: `452f724` ([`452f724d695683a155452704ad34dacac423ed8d`](https://github.com/anthropics/cosy-zero/commit/452f724d695683a155452704ad34dacac423ed8d))
**Date**: 2026-01-02 10:11:40 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Create Cosy2 - LTLf synthesis command-line tool:
- Support -f for formula file, -p for partition file
- Support inline formula strings
- Display parsed formula and realizability result
- Install to bin/ directory

Examples updated:
- response.ltlf/part: Response pattern with G(!quest | F(ack))
- Note: parser bug with 'r'/'f'/'g' as variable start

Known issues:
- Parser treats 'r' as Release operator, 'f' as Finally
- Multi-char variables starting with these chars fail
- TODO: Fix parser to check for full identifier before keywords

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `src/cosy2.cpp`


### Modified
- `CMakeLists.txt`
- `examples/response.ltlf`
- `examples/response.part`


## Stats

- **4** files changed
- **250** insertions(+)
- **3** deletions(-)
