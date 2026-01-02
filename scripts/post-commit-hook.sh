#!/bin/bash
# Git post-commit hook for automatic CHANGELOG generation
# Installed at: .git/hooks/post-commit

set -e

# Skip if NO_CHANGELOG is set
if [ "$NO_CHANGELOG" = "1" ]; then
    exit 0
fi

# Get project root
PROJECT_ROOT="$(cd "$(git rev-parse --show-toplevel)" && pwd)"

# Run changelog generator
"$PROJECT_ROOT/scripts/changelog.sh" HEAD

# Exit silently even if changelog generation fails
# (don't block the commit)
exit 0
