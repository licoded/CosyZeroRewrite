# [50] feat: add examples folder with LTLf formula samples

**Commit**: `624209e` ([`624209e6b2d1fd35cc549466340193d36fabe1bc`](https://github.com/anthropics/cosy-zero/commit/624209e6b2d1fd35cc549466340193d36fabe1bc))
**Date**: 2026-01-02 10:07:19 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

Add examples directory for testing synthesis tool:

- README.md: Formula syntax, semantics, usage instructions
- response.ltlf + response.part + response.desc: Response pattern (G(req -> F ack))
- sequence.ltlf: Sequential execution (p1 & X p2)
- mutex.ltlf: Mutual exclusion (!(crit1 & crit2))

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>

## Changes

### Added
- `examples/mutex.ltlf`
- `examples/README.md`
- `examples/response.desc`
- `examples/response.ltlf`
- `examples/response.part`
- `examples/sequence.ltlf`


## Stats

- **6** files changed
- **85** insertions(+)
- **0** deletions(-)
