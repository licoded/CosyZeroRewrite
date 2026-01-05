#!/bin/bash
# Git post-commit hook for automatic CHANGELOG generation
# Installed at: .git/hooks/post-commit

set -e

# Skip if NO_CHANGELOG is set
if [ "$NO_CHANGELOG" = "1" ]; then
    exit 0
fi

# Get current commit info
PROJECT_ROOT="$(git rev-parse --show-toplevel)"
CHANGEGEN_SCRIPT="$PROJECT_ROOT/tools/scripts/changelog.sh"

# Get commit subject
SUBJECT=$(git log -1 --format=%s HEAD)

# Skip if this is a CHANGELOG-only commit (prevents infinite loop)
# Matches: "docs: add CHANGELOG for..." or "docs: update CHANGELOG for..."
if [[ "$SUBJECT" =~ ^docs:\ (add|update)\ CHANGELOG\ for ]] || [[ "$SUBJECT" =~ ^chore:\ (add|update)\ CHANGELOG\ for ]]; then
    echo "[CHANGELOG] Skipping CHANGELOG-only commit"
    exit 0
fi

# Run changelog generator and capture output
# Strip ANSI escape codes for easier parsing
CHANGELOG_OUTPUT=$("$CHANGEGEN_SCRIPT" HEAD 2>&1 | sed 's/\x1b\[[0-9;]*m//g')

# Extract the filepath from the output (line with → contains full path)
FULL_PATH=$(echo "$CHANGELOG_OUTPUT" | grep '→' | tail -1 | sed -E 's/.*→[[:space:]]+//')

if [ -n "$FULL_PATH" ] && [ -f "$FULL_PATH" ]; then
    # Convert to relative path if it's absolute
    if [[ "$FULL_PATH" == "$PROJECT_ROOT"* ]]; then
        NEW_CHANGELOG="${FULL_PATH#$PROJECT_ROOT/}"
    else
        NEW_CHANGELOG="$FULL_PATH"
    fi

    echo "[CHANGELOG] Adding new changelog to commit..."

    # Add the new changelog file
    cd "$PROJECT_ROOT"
    git add "$NEW_CHANGELOG"

    # Create a follow-up commit for the changelog
    # Set author to match the original commit
    GIT_AUTHOR_DATE=$(git log -1 --format=%aD HEAD)
    GIT_COMMITTER_DATE=$(git log -1 --format=%cD HEAD)
    export GIT_AUTHOR_DATE GIT_COMMITTER_DATE

    git commit -m "docs: add CHANGELOG for $(git rev-parse --short HEAD)

Auto-generated CHANGELOG entry for previous commit.

🤖 Generated with [Claude Code](https://claude.com/claude-code)

Co-Authored-By: Claude Opus 4.5 <noreply@anthropic.com>"

    echo "[CHANGELOG] Created follow-up commit for changelog"

    # Auto-push to remote dev branch (with buffer: keep 3 non-CHANGELOG commits local)
    CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
    if [ "$CURRENT_BRANCH" = "dev" ]; then
        # Check if remote exists and is ahead (e.g., after rebase/rollback)
        if git rev-parse --verify origin/dev >/dev/null 2>&1; then
            LOCAL_TIP=$(git rev-parse HEAD)
            REMOTE_TIP=$(git rev-parse origin/dev)
            # Check if remote is ahead of local (local is behind)
            if git merge-base --is-ancestor "$LOCAL_TIP" "$REMOTE_TIP" 2>/dev/null; then
                echo "[CHANGELOG] Remote is ahead of local, skipping auto-push"
                exit 0
            fi
        fi

        # Find the 3rd non-CHANGELOG commit from HEAD, then push its parent
        # This keeps the latest 3 real commits AND their CHANGELOGs local
        count=0
        target="HEAD"
        while [ $count -lt 3 ]; do
            target="${target}~1"
            subject=$(git log -1 --format=%s "$target" 2>/dev/null) || break
            # Only count non-CHANGELOG commits
            if [[ ! "$subject" =~ ^docs:\ (add|update)\ CHANGELOG\ for ]] && [[ ! "$subject" =~ ^chore:\ (add|update)\ CHANGELOG\ for ]]; then
                count=$((count + 1))
            fi
        done
        # Push the parent of the 3rd real commit (which includes its CHANGELOG)
        echo "[CHANGELOG] Pushing to remote dev branch (buffer: 3 real commits)..."
        git push origin "${target}~1:dev" --force || echo "[CHANGELOG] Push failed (may need manual push)"
        echo "[CHANGELOG] Pushed successfully (3 real commits kept local for amend/rebase)"
    fi
fi

# Exit silently even if changelog generation fails
# (don't block the commit)
exit 0
