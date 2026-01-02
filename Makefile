.PHONY: help changelog ai-changelog install-changelog-hook test clean build

help: ## Show this help message
	@echo "CosyZeroRewrite - Available commands:"
	@echo ""
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-15s\033[0m %s\n", $$1, $$2}'

changelog: ## Generate CHANGELOG for latest commit
	./scripts/changelog.sh HEAD

changelog-all: ## Generate CHANGELOG for all untracked commits
	@for commit in $$(git log --format=%H --reverse HEAD@{1}..HEAD 2>/dev/null); do \
		./scripts/changelog.sh "$$commit" || true; \
	done

ai-changelog: ## Run AI analysis on latest commit CHANGELOG
	./scripts/ai-changelog.sh HEAD

install-changelog-hook: ## Install post-commit hook for auto CHANGELOG
	cp scripts/post-commit-hook.sh .git/hooks/post-commit
	chmod +x .git/hooks/post-commit
	@echo "✓ Hook installed"

test: ## Run all tests
	@if [ -d build ]; then \
		cd build && ctest --output-on-failure; \
	else \
		echo "Build directory not found. Run 'make build' first."; \
	fi

build: ## Build the project
	@mkdir -p build
	@cd build && cmake .. && make

clean: ## Clean build artifacts
	@rm -rf build

.DEFAULT_GOAL := help
