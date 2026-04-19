# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.5.0] - 2026-04-19

### Changed

- `scripts/release.py`: stage all changes (`git add -A`), no clean-tree requirement, dated `CHANGELOG` section without placeholder bullets, always `git push` and `gh release create --notes-from-tag`.
- Built-in `completion bash` / `completion zsh`: scripts print to stdout only; zsh leaf help shows the app-specific zsh completion filename.

### Fixed

- `Context::number_opt` uses `strtod` so libc++ (e.g. macOS CI) builds where `std::from_chars` has no floating-point overload.
- `builtins.hpp` formatting for CI `clang-format --dry-run --Werror`.

## [0.4.1] - 2026-04-19

### Changed

- Tooling tweaks

## [0.4.0] - 2026-04-19

## [0.3.0] - 2026-04-18

### Added

- GitHub Actions workflow: configure, build, `ctest`, install smoke, and `clang-format --dry-run`.
- `CONTRIBUTING.md`, `SECURITY.md`, `CODE_OF_CONDUCT.md`.
- `feature-spec.md` normative notes for exit codes and fallback behavior.
- Conan 2 `conanfile.py` plus `test_package/` consumer smoke.
- `.clang-format` for consistent C++ style.

### Changed

- CMake minimum **3.21**; `project(… HOMEPAGE_URL …)` metadata.
- Exported target **`argsbarg::argsbarg`** now sets `INTERFACE` **C++23** compile features for `find_package` consumers.
- When **not** the top-level project, `ARGSBARG_BUILD_EXAMPLES` and `ARGSBARG_BUILD_TESTS` default to **OFF** (still **ON** at repo root).
- README: friendlier tone, richer feature bullets, softer platform wording; `just release` no longer auto-pushes (prints push command instead).

## [0.2.0] - 2026-04-18

### Added

- Rounded box-drawing borders in help output (`╭`, `╮`, `│`, `╰`, `╯`, `─`) matching nim-argsbarg style.
- `cmake --install` support: headers + `argsbargConfig.cmake` / `argsbargConfigVersion.cmake` exported to `${CMAKE_INSTALL_LIBDIR}/cmake/argsbarg/`; install component `argsbarg_Development`.
- `argsbarg::argsbarg` ALIAS target so `FetchContent` and `find_package` consumers use the same target name.
- `just install` recipe (installs to `PREFIX`, default `$HOME/.local`).
- `ARGSBARG_BUILD_EXAMPLES` and `ARGSBARG_BUILD_TESTS` CMake options (default **ON** at top level) to opt out when embedding.
- CMake project `VERSION` is now read automatically from `version()` in `argsbarg.hpp` — single source of truth.

### Changed

- Renamed CMake option `ARGS_ENABLE_COVERAGE` → `ARGSBARG_ENABLE_COVERAGE` for naming consistency.
- `visible_width` advances by UTF-8 code units so multibyte box glyphs count as one column.
- `GNUInstallDirs` included before `add_library`; `INSTALL_INTERFACE` uses `${CMAKE_INSTALL_INCLUDEDIR}`.
- `add_subdirectory(examples)` gated in root `CMakeLists.txt`; `examples/CMakeLists.txt` is unconditional.
- README: Install section restructured into three numbered options (FetchContent, cmake --install, optional Conan); Developing section removed; placeholder repo URL fixed to real GitHub URL.

## [0.1.2] - 2026-04-18

### Changed

- Small documentation and packaging notes while iterating toward `cmake --install` / export layout.

## [0.1.1] - 2026-04-18

### Changed

- Library is **header-only**: implementations live under `include/argsbarg/` (with `include/argsbarg/detail/*` for large inline units). CMake exposes an **`INTERFACE`** target named `argsbarg` (no separate `.a` / `.lib` to link).

## [0.1.0] - 2026-04-18

### Added

- Initial library: nested subcommands, POSIX-style options, contextual help, fallback modes, bash/zsh completion generation.
- Fluent authoring API: `Leaf`, `Group`, `Opt`, and `Arg` builders (`include/argsbarg/builders.hpp`).
- `Application` fluent host, `Context` for handlers, `parse` / `schema_validate`, and umbrella header `argsbarg.hpp`.

[Unreleased]: https://github.com/bdombro/cpp-argsbarg/compare/v0.5.0...HEAD
[0.5.0]: https://github.com/bdombro/cpp-argsbarg/compare/v0.4.1...v0.5.0
[0.4.1]: https://github.com/bdombro/cpp-argsbarg/compare/v0.4.0...v0.4.1
[0.4.0]: https://github.com/bdombro/cpp-argsbarg/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/bdombro/cpp-argsbarg/compare/v0.2.0...v0.3.0
[0.2.0]: https://github.com/bdombro/cpp-argsbarg/compare/v0.1.2...v0.2.0
[0.1.2]: https://github.com/bdombro/cpp-argsbarg/compare/v0.1.1...v0.1.2
[0.1.1]: https://github.com/bdombro/cpp-argsbarg/compare/v0.1.0...v0.1.1
[0.1.0]: https://github.com/bdombro/cpp-argsbarg/releases/tag/v0.1.0
