# Contributing

Thanks for helping improve **argsbarg**.

## Prerequisites

- **CMake** 3.21 or newer
- A **C++23** toolchain (GCC 12+, Clang 16+, or recent Apple Clang)
- **bash** (used by smoke tests and example `justfile`s)

Supported platforms for the current release line are **Linux and macOS** (POSIX). Windows is not claimed yet.

## Build and test

From the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
```

Or, if you use [just](https://github.com/casey/just):

```bash
just test
```

## Formatting

The project uses **clang-format** (see [`.clang-format`](.clang-format)). CI runs a dry-run check.

Format sources locally:

```bash
just format
```

Or invoke `clang-format` directly:

```bash
find include tests examples \( -name '*.hpp' -o -name '*.cpp' \) \
  ! -path '*/build/*' ! -path '*/_deps/*' -print0 | xargs -0 clang-format -i
```

## Conan (optional)

A [Conan 2](https://docs.conan.io/2/) recipe lives at [`conanfile.py`](conanfile.py) with a [`test_package/`](test_package/) consumer smoke test.

```bash
conan create . --build=missing -s compiler.cppstd=23
```

For publishing to **Conan Center**, upstream the recipe via [conan-center-index](https://github.com/conan-io/conan-center-index) (this repo ships the recipe as a reference and for local `conan create`).

## Versioning and changelog

- Library version is defined once in `version()` inside [`include/argsbarg/argsbarg.hpp`](include/argsbarg/argsbarg.hpp); root `CMakeLists.txt` parses it for `project(VERSION …)`.
- Release notes go in [`CHANGELOG.md`](CHANGELOG.md) (Keep a Changelog + SemVer).

## Pull requests

- Prefer small, focused PRs with tests when behavior changes.
- Ensure `ctest` passes and `clang-format` is clean before opening a PR.
