#!/bin/bash
# Changelog Generator for CosyZeroRewrite
# Generates markdown changelog entries from git commits

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CHANGELOG_DIR="$PROJECT_ROOT/docs/CHANGELOG"
COUNTER_FILE="$PROJECT_ROOT/.git/changelog-counter"

# Default commit to process
COMMIT="${1:-HEAD}"

# Check if we should skip (NO_CHANGELOG env var)
if [ "$NO_CHANGELOG" = "1" ]; then
    echo "CHANGELOG generation skipped (NO_CHANGELOG=1)"
    exit 0
fi

# Get commit info
HASH=$(git rev-parse --short "$COMMIT")
FULL_HASH=$(git rev-parse "$COMMIT")
AUTHOR_NAME=$(git log -1 --format=%an "$COMMIT")
AUTHOR_EMAIL=$(git log -1 --format=%ae "$COMMIT")
DATE=$(git log -1 --format=%ci "$COMMIT")
DATE_ONLY=$(git log -1 --format=%cd --date=short "$COMMIT")

# Get commit message
SUBJECT=$(git log -1 --format=%s "$COMMIT")
BODY=$(git log -1 --format=%b "$COMMIT")

# Extract type and create description
TYPE=$(echo "$SUBJECT" | cut -d: -f1)
DESC_RAW=$(echo "$SUBJECT" | cut -d: -f2 | sed 's/^[[:space:]]*//')

# Clean description for filename: lowercase, remove special chars, replace spaces with _
# Remove common Chinese characters and special chars, keep alphanumeric and underscore
DESC_CLEAN=$(echo "$DESC_RAW" | \
    sed 's/[[:space:]]\+/_/g' | \
    sed 's/[^a-zA-Z0-9_-]//g' | \
    tr '[:upper:]' '[:lower:]' | \
    cut -c1-30)  # Limit to 30 chars

# Use TYPE if description is empty after cleaning
if [ -z "$DESC_CLEAN" ]; then
    DESC_CLEAN=$(echo "$TYPE" | tr '[:upper:]' '[:lower:]')
fi

# Get current counter (only for generating filename)
mkdir -p "$(dirname "$COUNTER_FILE")"
if [ -f "$COUNTER_FILE" ]; then
    COUNTER=$(cat "$COUNTER_FILE")
else
    # Count existing changelog files
    COUNTER=$(ls -1 "$CHANGELOG_DIR"/*.md 2>/dev/null | wc -l)
fi

# Format counter as 5 digits (tentative, will increment if file doesn't exist)
COUNTER_FMT=$(printf "%05d" "$((COUNTER + 1))")

# Create filename (tentative)
FILENAME="${COUNTER_FMT}_${HASH}_${DESC_CLEAN}.md"
FILEPATH="$CHANGELOG_DIR/$FILENAME"

# Check if file already exists - if so, skip entirely
if [ -f "$FILEPATH" ]; then
    echo -e "${YELLOW}CHANGELOG already exists: $FILENAME${NC}"
    exit 0
fi

# Only increment counter if we're actually creating a new file
COUNTER=$((COUNTER + 1))
echo "$COUNTER" > "$COUNTER_FILE"
COUNTER_FMT=$(printf "%05d" "$COUNTER")

# Update filename with confirmed counter
FILENAME="${COUNTER_FMT}_${HASH}_${DESC_CLEAN}.md"
FILEPATH="$CHANGELOG_DIR/$FILENAME"

# Get file changes
CHANGES=$(git diff-tree --no-commit-id --name-status -r "$COMMIT" | sort)

# Categorize files
ADDED=""
MODIFIED=""
DELETED=""

while IFS=$'\t' read -r status file; do
    case "$status" in
        A)  ADDED="$ADDED- \`$file\`\n" ;;
        M)  MODIFIED="$MODIFIED- \`$file\`\n" ;;
        D*) DELETED="$DELETED- \`$file\`\n" ;;
        R*)  MODIFIED="$MODIFIED- \`$file\` (renamed)\n" ;;
    esac
done <<< "$CHANGES"

# Get stats (use --no-commit-id to avoid hash prefix)
STATS=$(git diff-tree --no-commit-id --shortstat -r "$COMMIT")
FILES_CHANGED=$(echo "$STATS" | grep -oE '[0-9]+ file' | grep -oE '[0-9]+' || echo "0")
INSERTIONS=$(echo "$STATS" | grep -oE '[0-9]+ insertion' | grep -oE '[0-9]+' || echo "0")
DELETIONS=$(echo "$STATS" | grep -oE '[0-9]+ deletion' | grep -oE '[0-9]+' || echo "0")

# Set defaults if empty
: "${FILES_CHANGED:=0}"
: "${INSERTIONS:=0}"
: "${DELETIONS:=0}"

# Create markdown content
cat > "$FILEPATH" << EOF
# [${COUNTER}] ${SUBJECT}

**Commit**: \`${HASH}\` ([\`${FULL_HASH}\`](https://github.com/anthropics/cosy-zero/commit/${FULL_HASH}))
**Date**: ${DATE}
**Author**: ${AUTHOR_NAME} <${AUTHOR_EMAIL}>

## Description

${BODY}
EOF

# Add Changes section if there are changes
if [ -n "$CHANGES" ]; then
    cat >> "$FILEPATH" << EOF

## Changes
EOF

    if [ -n "$ADDED" ]; then
        echo -e "\n### Added\n${ADDED}" >> "$FILEPATH"
    fi

    if [ -n "$MODIFIED" ]; then
        echo -e "\n### Modified\n${MODIFIED}" >> "$FILEPATH"
    fi

    if [ -n "$DELETED" ]; then
        echo -e "\n### Deleted\n${DELETED}" >> "$FILEPATH"
    fi
fi

# Add stats section
cat >> "$FILEPATH" << EOF

## Stats

- **${FILES_CHANGED}** files changed
- **${INSERTIONS}** insertions(+)
- **${DELETIONS}** deletions(-)
EOF

echo -e "${GREEN}✓ CHANGELOG created: ${FILENAME}${NC}"
echo -e "  ${GREEN}→ ${FILEPATH}${NC}"

# Also print a summary
echo ""
echo -e "Summary: ${YELLOW}[${COUNTER}]${NC} ${HASH} ${SUBJECT}"
