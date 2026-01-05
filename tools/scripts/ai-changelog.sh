#!/bin/bash
# AI Changelog Analyzer
# Extracts git diff for Claude to analyze and append to CHANGELOG

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$(dirname "$SCRIPT_DIR")")"
CHANGELOG_DIR="$PROJECT_ROOT/docs/CHANGELOG"

# Default commit to process
COMMIT="${1:-HEAD}"

# Get commit hash
HASH=$(git rev-parse --short "$COMMIT")

# Find the changelog file for this commit
CHANGELOG_FILE=$(find "$CHANGELOG_DIR" -name "*_${HASH}_*.md" 2>/dev/null | head -1)

if [ -z "$CHANGELOG_FILE" ]; then
    echo -e "${RED}✗ No CHANGELOG found for commit ${HASH}${NC}"
    echo "Run ./tools/scripts/changelog.sh $COMMIT first"
    exit 1
fi

echo -e "${BLUE}=== AI Changelog Analysis ===${NC}"
echo -e "${BLUE}Commit: ${YELLOW}${HASH}${NC}"
echo -e "${BLUE}CHANGELOG: ${YELLOW}${CHANGELOG_FILE}${NC}"
echo ""

# Extract diff for analysis
echo -e "${GREEN}→ Extracting diff...${NC}"

# Get changed files
CHANGED_FILES=$(git diff-tree --no-commit-id --name-only -r "$COMMIT" | tr '\n' ' ')

# Get full diff
DIFF_OUTPUT=$(git diff-tree --no-commit-id -p "$COMMIT")

# Calculate stats
STATS=$(git diff-tree --no-commit-id --shortstat -r "$COMMIT")
FILES_CHANGED=$(echo "$STATS" | grep -oE '[0-9]+ file' | grep -oE '[0-9]+' || echo "0")
INSERTIONS=$(echo "$STATS" | grep -oE '[0-9]+ insertion' | grep -oE '[0-9]+' || echo "0")
DELETIONS=$(echo "$STATS" | grep -oE '[0-9]+ deletion' | grep -oE '[0-9]+' || echo "0")
TOTAL_CHANGES=$((INSERTIONS + DELETIONS))

# Get commit subject for context
SUBJECT=$(git log -1 --format=%s "$COMMIT")
TYPE=$(echo "$SUBJECT" | cut -d: -f1)

echo -e "${GREEN}→ Files changed: ${YELLOW}${FILES_CHANGED}${NC}"
echo -e "${GREEN}→ Lines changed: ${YELLOW}${TOTAL_CHANGES}${NC} (+${INSERTIONS}, -${DELETIONS})"
echo ""

# Create analysis prompt file
PROMPT_FILE="$PROJECT_ROOT/.git/ai-changelog-prompt.md"

cat > "$PROMPT_FILE" << 'PROMPT_EOF'
# AI Changelog Analysis Request

You are analyzing a git commit to generate an intelligent CHANGELOG entry.

## Task
Analyze the git diff below and append an "## AI Analysis" section to the CHANGELOG file.

## Output Format
```markdown
## AI Analysis

### 📝 Change Summary
[2-3 sentences describing what was changed and why]

### 🔍 Technical Details
[Key technical points about the implementation]

### 📊 Impact Analysis
[What areas of the codebase are affected]

### ⚠️ Notes
[Any warnings, todos, or potential issues]
```

## Adaptive Length Guidelines
- **Small change** (< 50 lines): Brief summary only, skip Technical Details
- **Medium change** (50-200 lines): Full format with concise details
- **Large change** (200+ lines): Detailed analysis with code examples

## Diagram Guidelines
Include mermaid diagrams when appropriate:
- **Architecture changes**: Use `graph TB` or `C4Context`
- **Flow changes**: Use `flowchart TD`
- **Class changes**: Use `classDiagram`
- **Sequence changes**: Use `sequenceDiagram`

## Commit Context
- **Type**: TYPE_PLACEHOLDER
- **Subject**: SUBJECT_PLACEHOLDER
- **Files**: FILES_PLACEHOLDER
- **Changes**: FILES_CHG files, +INS ins, -DEL del

## Git Diff
\`\`\`diff
DIFF_PLACEHOLDER
\`\`\`

## Instructions
1. Read the CHANGELOG file first to understand existing content
2. Analyze the diff focusing on:
   - What problem was solved
   - How the solution works
   - Key code changes
   - Architecture impact
3. Generate the "## AI Analysis" section
4. Use Edit tool to append it to the CHANGELOG file (before the "## Stats" section)

Write the analysis in Chinese, with technical terms in English.
PROMPT_EOF

# Replace placeholders
sed -i "s/TYPE_PLACEHOLDER/${TYPE}/" "$PROMPT_FILE"
sed -i "s|SUBJECT_PLACEHOLDER|${SUBJECT}|" "$PROMPT_FILE"
sed -i "s|FILES_PLACEHOLDER|${CHANGED_FILES}|" "$PROMPT_FILE"
sed -i "s/FILES_CHG/${FILES_CHANGED}/" "$PROMPT_FILE"
sed -i "s/INS/${INSERTIONS}/" "$PROMPT_FILE"
sed -i "s/DEL/${DELETIONS}/" "$PROMPT_FILE"

# Escape diff for sed (use a different approach)
awk -v diff="$DIFF_OUTPUT" '
    BEGIN { printing = 1 }
    /DIFF_PLACEHOLDER/ { print "```"; print diff; print "```"; printing = 0; next }
    printing { print }
' "$PROMPT_FILE" > "$PROMPT_FILE.tmp" && mv "$PROMPT_FILE.tmp" "$PROMPT_FILE"

echo -e "${GREEN}→ Analysis prompt prepared: ${YELLOW}${PROMPT_FILE}${NC}"
echo ""
echo -e "${BLUE}=== Next Steps ===${NC}"
echo "1. Read the prompt file:"
echo -e "   ${YELLOW}cat ${PROMPT_FILE}${NC}"
echo ""
echo "2. Read the current CHANGELOG:"
echo -e "   ${YELLOW}cat ${CHANGELOG_FILE}${NC}"
echo ""
echo "3. Use Edit tool to append '## AI Analysis' section before '## Stats'"
echo ""

# Optional: auto-display the prompt
if [ "$0" = "$BASH_SOURCE" ]; then
    echo -e "${BLUE}=== Prompt Preview ===${NC}"
    cat "$PROMPT_FILE"
fi
