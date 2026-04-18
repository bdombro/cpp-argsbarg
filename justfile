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

# Bump version, update CHANGELOG stub, commit, and tag. Requires a clean git working tree.
# Usage: `just release patch` | `just release minor` | `just release major` | `just release 1.2.3`
release kind:
    #!/usr/bin/env bash
    set -euo pipefail
    # Shebang recipes run from a temp script; BASH_SOURCE is not the repo. Use the justfile dir.
    repo_root="{{justfile_directory()}}"
    cd "$repo_root"
    if ! git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
      echo "error: not a git repository" >&2
      exit 1
    fi
    if [[ -n "$(git status --porcelain)" ]]; then
      echo "error: working tree is not clean; commit or stash changes before release" >&2
      exit 1
    fi
    hdr="$repo_root/include/argsbarg/argsbarg.hpp"
    readme="$repo_root/README.md"
    changelog="$repo_root/CHANGELOG.md"
    cur="$(sed -n 's/.*return "\([0-9][0-9]*\.[0-9][0-9]*\.[0-9][0-9]*\)".*/\1/p' "$hdr" | head -1)"
    if [[ -z "$cur" ]]; then
      echo "error: could not read semver from $hdr" >&2
      exit 1
    fi
    IFS=. read -r maj min pat <<<"$cur"
    case "{{kind}}" in
      major) new="$((maj + 1)).0.0" ;;
      minor) new="$maj.$((min + 1)).0" ;;
      patch) new="$maj.$min.$((pat + 1))" ;;
      *)
        if [[ "{{kind}}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
          new="{{kind}}"
        else
          echo "usage: just release major|minor|patch|X.Y.Z" >&2
          exit 1
        fi
        ;;
    esac
    if [[ "$new" == "$cur" ]]; then
      echo "error: already at v$new (use patch/minor/major or a higher X.Y.Z to bump)" >&2
      exit 1
    fi
    today="$(date +%Y-%m-%d)"
    perl -pi -e 's/return "[0-9.]+"/return "'"$new"'"/' "$hdr"
    perl -pi -e 's/(GIT_TAG\s+)v[0-9.]+/${1}v'"$new"'/' "$readme"
    python3 -c 'import sys; from pathlib import Path; nl=chr(10); p,v,d=Path(sys.argv[1]),sys.argv[2],sys.argv[3]; t=p.read_text(encoding="utf-8"); m="## [Unreleased]"+nl+nl; assert m in t, "missing ## [Unreleased] in CHANGELOG.md"; b=m+"## ["+v+"] - "+d+nl+nl+"### Changed"+nl+nl+"- (Summarize this release; amend commit before push if needed.)"+nl+nl; p.write_text(t.replace(m,b,1),encoding="utf-8")' "$changelog" "$new" "$today"
    git add "$hdr" "$readme" "$changelog"
    git commit -m "chore: release v$new"
    git tag -a "v$new" -m "Release v$new"
    echo "Created commit and tag v$new. Push with: git push && git push origin v$new"
    git push
    git push origin v$new
