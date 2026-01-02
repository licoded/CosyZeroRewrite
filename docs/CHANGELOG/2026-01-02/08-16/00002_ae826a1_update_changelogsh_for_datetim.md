# [2] fix: update changelog.sh for date/time slot directory structure

**Commit**: `ae826a1` ([`ae826a14083bf8400d21ad2d811cd74b508236da`](https://github.com/anthropics/cosy-zero/commit/ae826a14083bf8400d21ad2d811cd74b508236da))
**Date**: 2026-01-02 10:59:58 +0800
**Author**: licoded <busy.li@foxmail.com>

## Description

- Add time slot calculation (00-08, 08-16, 16-24)
- Create date-based subdirectories automatically
- Update ai-changelog.sh to search in new structure
- Update README with new directory structure docs

## Changes

### Added
- `docs/CHANGELOG/2026-01-02/08-16/00001_d195dfd_reorganize_changelog_by_date_a.md`


### Modified
- `scripts/changelog.sh`


### Deleted
- `docs/CHANGELOG/2026-01-02/08-16/00001_059f9a0_add_changelog_and_refine_skip_.md`
- `docs/CHANGELOG/2026-01-02/08-16/00002_184c70a_prevent_changelog_infinite_loo.md`
- `docs/CHANGELOG/2026-01-02/08-16/00003_189ce6a_reorganize_todo_and_bugs_into_.md`
- `docs/CHANGELOG/2026-01-02/08-16/00004_26f6531_add_todo_completion_workflow_t.md`
- `docs/CHANGELOG/2026-01-02/08-16/00005_27e3c59_add_changelog_system_with_auto.md`
- `docs/CHANGELOG/2026-01-02/08-16/00006_32c749e_add_known_bugs_section_to_clau.md`
- `docs/CHANGELOG/2026-01-02/08-16/00007_452f724_add_cosy2_synthesis_tool_binar.md`
- `docs/CHANGELOG/2026-01-02/08-16/00008_624209e_add_examples_folder_with_ltlf_.md`
- `docs/CHANGELOG/2026-01-02/08-16/00009_6c36489_reorganize_documentation_into_.md`
- `docs/CHANGELOG/2026-01-02/08-16/00010_7749377_add_changelog_for_docs_commit.md`
- `docs/CHANGELOG/2026-01-02/08-16/00011_97e2f8f_correct_accepting_state_condit.md`
- `docs/CHANGELOG/2026-01-02/08-16/00012_9977380_add_ai-powered_changelog_analy.md`
- `docs/CHANGELOG/2026-01-02/08-16/00013_b36f4d8_add_changelog_for_previous_com.md`
- `docs/CHANGELOG/2026-01-02/08-16/00014_c5cb10a_add_bug_recording_principle_to.md`
- `docs/CHANGELOG/2026-01-02/08-16/00015_cf9166d_document_changelog_infinite_lo.md`
- `docs/CHANGELOG/2026-01-02/08-16/00016_dba0871_parser_single-char_operator_pr.md`


## Stats

- **18** files changed
- **72** insertions(+)
- **569** deletions(-)
