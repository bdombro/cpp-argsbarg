set shell := ["bash", "-eu", "-o", "pipefail", "-c"]

_:
    @just --list

configure:
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug

build: configure
    cmake --build build -j

test: build
    cd build && ctest --output-on-failure

completion-check: build
    @mkdir -p build
    ./build/examples/minimal/minimal_example completion bash > build/_argsbarg_completion_check.sh
    bash -n build/_argsbarg_completion_check.sh

# Format C++ under include/, tests/, examples/ (excludes in-tree example build trees).
format:
    #!/usr/bin/env bash
    set -euo pipefail
    repo_root="{{justfile_directory()}}"
    CFMT="${CLANG_FORMAT:-clang-format}"
    if ! command -v "$CFMT" >/dev/null 2>&1 && command -v xcrun >/dev/null 2>&1; then
      CFMT="$(xcrun -find clang-format 2>/dev/null || true)"
    fi
    if ! command -v "$CFMT" >/dev/null 2>&1; then
      echo "error: clang-format not found (set CLANG_FORMAT=...)" >&2
      exit 1
    fi
    cd "$repo_root"
    find include tests examples \( -name '*.hpp' -o -name '*.cpp' \) \
      ! -path '*/build/*' ! -path '*/_deps/*' -print0 | xargs -0 "$CFMT" -i

clean:
    rm -rf build build-coverage build-install

# Install headers + CMake config under PREFIX (default: $HOME/.local). Example: PREFIX=/opt/myapp just install
install:
    #!/usr/bin/env bash
    set -euo pipefail
    repo_root="{{justfile_directory()}}"
    PREFIX="${PREFIX:-$HOME/.local}"
    cmake -S "$repo_root" -B "$repo_root/build-install" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX="$PREFIX" \
      -DARGSBARG_BUILD_EXAMPLES=OFF \
      -DARGSBARG_BUILD_TESTS=OFF
    cmake --build "$repo_root/build-install" -j
    cmake --install "$repo_root/build-install" --component argsbarg_Development

example-minimal *ARGS:
    just -f examples/minimal/justfile run {{ARGS}}

example-nested *ARGS:
    just -f examples/nested/justfile run {{ARGS}}

# HTML coverage report under build-coverage/coverage/html/index.html (Apple Clang / LLVM).
coverage:
    #!/usr/bin/env bash
    set -euo pipefail
    rm -rf build-coverage
    cmake -S . -B build-coverage -DCMAKE_BUILD_TYPE=Debug -DARGSBARG_ENABLE_COVERAGE=ON
    cmake --build build-coverage -j
    (cd build-coverage && mkdir -p coverage/raw && LLVM_PROFILE_FILE="$PWD/coverage/raw/cov-%m.profraw" ctest --output-on-failure)
    if command -v xcrun >/dev/null 2>&1 && xcrun --find llvm-profdata >/dev/null 2>&1; then
      LLVM_PROFDATA=(xcrun llvm-profdata)
      LLVM_COV=(xcrun llvm-cov)
    else
      LLVM_PROFDATA=(llvm-profdata)
      LLVM_COV=(llvm-cov)
    fi
    mapfile -t profraws < <(find build-coverage -name '*.profraw' -print)
    if [[ ${#profraws[@]} -eq 0 ]]; then echo "No .profraw files found"; exit 1; fi
    "${LLVM_PROFDATA[@]}" merge -sparse "${profraws[@]}" -o build-coverage/coverage/merged.profdata
    objs=()
    for x in build-coverage/tests/test_*; do
      if [[ -f "$x" && -x "$x" ]]; then objs+=("$x"); fi
    done
    objs+=("build-coverage/examples/minimal/minimal_example" "build-coverage/examples/nested/nested_example")
    mkdir -p build/coverage
    "${LLVM_COV[@]}" show "${objs[@]}" \
      -instr-profile=build-coverage/coverage/merged.profdata \
      -format=html \
      -output-dir=build/coverage/html \
      -ignore-filename-regex='catch2|_deps'
    echo "Open build/coverage/html/index.html"  # instruments tests + examples (header-only lib has no .a)

# Publish a new release
release kind:
    python3 "{{justfile_directory()}}/scripts/release.py" "{{kind}}"
