# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.2.0] - 2026-04-18

### Changed

- (Summarize this release; amend commit before push if needed.)

## [0.1.3] - 2026-04-18

### Added

- Rounded box-drawing borders in help output (`╭`, `╮`, `│`, `╰`, `╯`, `─`) matching nim-argsbarg style.
- `cmake --install` support: headers + `argsbargConfig.cmake` / `argsbargConfigVersion.cmake` exported to `${CMAKE_INSTALL_LIBDIR}/cmake/argsbarg/`; install component `argsbarg_Development`.
- `argsbarg::argsbarg` ALIAS target so `FetchContent` and `find_package` consumers use the same target name.
- `just install` recipe (installs to `PREFIX`, default `$HOME/.local`).
- `ARGSBARG_BUILD_EXAMPLES` and `ARGSBARG_BUILD_TESTS` CMake options (both default `ON`) to opt out when embedding.
- CMake project `VERSION` is now read automatically from `version()` in `argsbarg.hpp` — single source of truth.

### Changed

- Renamed CMake option `ARGS_ENABLE_COVERAGE` → `ARGSBARG_ENABLE_COVERAGE` for naming consistency.
- `visible_width` advances by UTF-8 code units so multibyte box glyphs count as one column.
- `GNUInstallDirs` included before `add_library`; `INSTALL_INTERFACE` uses `${CMAKE_INSTALL_INCLUDEDIR}`.
- `add_subdirectory(examples)` gated in root `CMakeLists.txt`; `examples/CMakeLists.txt` is unconditional.
- README: Install section restructured into three numbered options (FetchContent, cmake --install, vcpkg/Conan); Developing section removed; placeholder repo URL fixed to real GitHub URL.

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
