# [58] feat: add CHANGELOG system with automatic git hook

**Commit**: `27e3c59` ([`27e3c592d113812d7950657d915b5eb3000655b6`](https://github.com/anthropics/cosy-zero/commit/27e3c592d113812d7950657d915b5eb3000655b6))
**Date**: 2026-01-02 10:37:35 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Implemented automatic CHANGELOG generation on every git commit:
- Created docs/CHANGELOG/ directory for per-commit records
- Added changelog.sh script to generate markdown entries
- Installed post-commit git hook for automatic generation
- Updated CMakeLists.txt with changelog utilities target
- Updated WORKFLOWS with CHANGELOG documentation

File naming:序号_commitHash_简述.md
Example: 00002_6c36489_reorganize_documentation_into_.md

Generated CHANGELOGs include:
- Commit hash, date, author
- Full commit message
- Changed files (added/modified/deleted)
- File statistics

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `docs/CHANGELOG/00002_6c36489_reorganize_documentation_into_.md`
- `docs/CHANGELOG/00003_dba0871_parser_single-char_operator_pr.md`
- `docs/CHANGELOG/00004_189ce6a_reorganize_todo_and_bugs_into_.md`
- `docs/CHANGELOG/00010_c5cb10a_add_bug_recording_principle_to.md`
- `docs/CHANGELOG/00013_32c749e_add_known_bugs_section_to_clau.md`
- `docs/CHANGELOG/00014_452f724_add_cosy2_synthesis_tool_binar.md`
- `docs/CHANGELOG/00015_26f6531_add_todo_completion_workflow_t.md`
- `docs/CHANGELOG/00016_624209e_add_examples_folder_with_ltlf_.md`
- `docs/CHANGELOG/README.md`
- `scripts/changelog.sh`
- `scripts/post-commit-hook.sh`


### Modified
- `CMakeLists.txt`
- `docs/WORKFLOWS/commit_practice.md`
- `docs/WORKFLOWS/todo_management.md`


## Stats

- **14** files changed
- **740** insertions(+)
- **9** deletions(-)
