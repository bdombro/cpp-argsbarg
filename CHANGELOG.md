# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.3] - 2026-04-18

### Changed

- (Summarize this release; amend commit before push if needed.)

## [0.1.2] - 2026-04-18

### Changed

- (Summarize this release; amend commit before push if needed.)

## [0.1.1] - 2026-04-18

### Changed

- (Summarize this release; amend commit before push if needed.)

### Changed

- Library is **header-only**: implementations live under `include/argsbarg/` (with `include/argsbarg/detail/*` for large inline units). CMake exposes an **`INTERFACE`** target named `argsbarg` (no separate `.a` / `.lib` to link).

## [0.1.0] - 2026-04-18

### Added

- Initial library: nested subcommands, POSIX-style options, contextual help, fallback modes, bash/zsh completion generation.
- Fluent authoring API: `Leaf`, `Group`, `Opt`, and `Arg` builders (`include/argsbarg/builders.hpp`).
- `Application` fluent host, `Context` for handlers, `parse` / `schema_validate`, and umbrella header `argsbarg.hpp`.
