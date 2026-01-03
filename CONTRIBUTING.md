# Contributing to OpenChordix

This file describes contribution workflow and repo-specific expectations.

## Where to contribute

- **Bug reports / questions**: open a GitHub Issue with:
  - OS + compiler (and versions)
  - steps to reproduce
  - expected vs actual behavior
  - logs / screenshots if relevant
- **Feature proposals**: open an Issue first. For larger changes, describe:
  - the problem being solved
  - proposed approach
  - impact on dependencies / platforms
  - what you plan to test

## Branching policy

- **Base branch for PRs:** `master`.
- Keep PRs focused: one feature/fix per PR whenever possible.

## Commit messages (required)

This repository uses **Conventional Commits**.

- Use Conventional Commit messages for every commit.
- If your PR contains multiple logical changes, split into multiple commits.
- If a change affects both build/infra and code, prefer separate commits.

## Dependencies

- Do not add or change third-party dependencies without discussion in an Issue first.
- When touching dependency setup, keep it compatible with the project.

## Build & verification (before opening a PR)

Before you open a PR, ensure:
- The project configures and builds successfully on at least one supported platform.
- Your change does not break compilation for other platforms (guard OS-specific code).

If your change is platform-specific, state what you tested (OS, compiler, audio backend, etc.).

## PR checklist

Include in the PR description:
- What problem you solved / what feature you added
- How to test it (exact steps)
- Platforms tested + compiler versions
- Any known limitations or follow-up work

## Review notes

- Expect maintainers to request changes if:
  - commit messages don’t follow Conventional Commits
  - changes are mixed/too broad
  - code style diverges from nearby code
  - new dependencies are introduced without prior discussion

## License

By contributing, you agree that your contributions are licensed under this repository’s license (see `LICENSE`).
