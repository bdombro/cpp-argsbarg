```
                          ___.                        
_____ _______  ____  _____\_ |__ _____ _______  ____  
\__  \\_  __ \/ ___\/  ___/| __ \\__  \\_  __ \/ ___\ 
 / __ \|  | \/ /_/  >___ \ | \_\ \/ __ \|  | \/ /_/  >
(____  /__|  \___  /____  >|___  (____  /__|  \___  / 
     \/     /_____/     \/     \/     \/     /_____/  
```
<!-- Big money NE - https://patorjk.com/software/taag/#p=testall&f=Bulbhead&t=shebangsy&x=none&v=4&h=4&w=80&we=false> -->

# argsbarg

**argsbarg** is a small C++23 header-only toolkit for pretty CLIs with sh completions. 

- Features:
  - Nested subcommands
  - POSIX-style options
  - Arg/Option validation
  - Contextual help
  - Default-command fallback
  - Shell completion scripts
  - NO macros
  - **no third-party runtime dependencies**

---

## Usage

```cpp
#include <argsbarg/argsbarg.hpp>
#include <iostream>

using namespace argsbarg;

int main(int argc, const char* argv[]) {
    auto greet = [](Context& ctx) {
        const auto name = ctx.string_opt("name").value_or("world");
        if (ctx.flag("verbose")) {
            std::cout << "verbose mode\n";
        }
        std::cout << "hello " << name << '\n';
    };

    Application{"helloapp"}
        .description("Tiny demo.")
        .fallback("hello", FallbackMode::MissingOrUnknown)
        .command(Leaf{"hello", "Say hello."}
            .handler(greet)
            .option(Opt{"name", "Who to greet."}.string().short_alias('n'))
            .option(Opt{"verbose", "Enable extra logging."}.short_alias('v')))
        .run(argc, argv);
}
```

---

## Install

The imported target is always `argsbarg::argsbarg`, regardless of which method you use.

### Option 1 — CMake `FetchContent` (no install step)

CMake downloads the source at configure time and builds it alongside your project. Good for small teams and single-repo setups.

```cmake
include(FetchContent)
FetchContent_Declare(argsbarg
    GIT_REPOSITORY https://github.com/bdombro/cpp-argsbarg.git
    GIT_TAG        v0.1.3)
# Optional: skip Catch2 download and example builds
set(ARGSBARG_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(ARGSBARG_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(argsbarg)

target_link_libraries(your_target PRIVATE argsbarg::argsbarg)
```

### Option 2 — `cmake --install` + `find_package`

Install once to a prefix, then any project on the machine can find it without cloning the repo.

**Install:**

```bash
# Clone and install to ~/.local (or any prefix you choose)
git clone https://github.com/bdombro/cpp-argsbarg.git
cd cpp-argsbarg
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --build build -j
cmake --install build --component argsbarg_Development
```

Or from inside the repo: `PREFIX="$HOME/.local" just install`.

**Consume:**

```cmake
list(APPEND CMAKE_PREFIX_PATH "$ENV{HOME}/.local") # match your install prefix
find_package(argsbarg CONFIG REQUIRED)             # append e.g. 0.1.3 to pin the version

target_link_libraries(your_target PRIVATE argsbarg::argsbarg)
```

### Option 3 — vcpkg / Conan

Once a port/recipe exists, consumers can install with `vcpkg install argsbarg` or `conan install argsbarg/[>=0.1 <1]`. See the maintainer guides for [vcpkg](https://learn.microsoft.com/en-us/vcpkg/contributing/maintainer-guide) and [Conan Center](https://github.com/conan-io/conan-center-index/blob/master/docs/how_to_add_packages.md).

---

## Built-ins

Every app processed by `run()` gains:

- `-h` / `--help` at any routing depth (scoped help).
- `completion bash` — prints a bash completion script to **stdout**.
- `completion zsh` — installs under `~/.zsh/completions/` or use `--print` for **stdout**.

The top-level name `completion` is reserved for the built-in group.

---

## How it works

1. Describe a tree with `Group` / `Leaf` and `Opt` / `Arg` fluent builders (see [`include/argsbarg/builders.hpp`](include/argsbarg/builders.hpp)).
2. `Application::run` or `run(schema, argc, argv)` merges built-ins, validates the schema, parses argv, renders help or errors, dispatches the leaf handler, and `std::exit`s with the codes in `feature-spec.md` §8.
3. From a handler, `err_with_help(ctx, "message")` prints a red error line plus contextual help on stderr and exits with status 1.
4. `parse(merged_schema, argv_words)` and `help_render(merged_schema, path)` are pure helpers for tests and custom hosts (no I/O, no exit).

### Fallback modes (`FallbackMode`)

| Mode | Empty argv | Unknown first token |
| --- | --- | --- |
| `MissingOnly` | Default command | Error |
| `MissingOrUnknown` | Default command | Default command (token becomes argv for the default) |
| `UnknownOnly` | Root help (exit 1) | Default command |

With `MissingOrUnknown` / `UnknownOnly`, unrecognized **root** flags stop root-flag consumption and the remainder is passed to the default command (see `feature-spec.md` §5.3).

### Positionals (help labels)

| Builder | Help label |
| --- | --- |
| `Arg{"n", "…"}` | `<n>` |
| `Arg{"n", "…"}.optional()` | `[n]` |
| `Arg{"f", "…"}.min(0)` | `[f...]` |
| `Arg{"f", "…"}.min(1)` | `<f...>` |
| `Arg{"f", "…"}.min(1).max(3)` | `<f...>` (one to three words) |

### Reading values (`Context`)

- `ctx.flag("verbose")` — presence options.
- `ctx.string_opt("name")` / `ctx.number_opt("count")` — `std::optional`.
- `ctx.args()` — positional words in order.
- `ctx.schema()` — merged schema (for help paths in errors).

---

## Examples

This repository ships two demos under `examples/`, each with its own `justfile` so you can work inside the example directory.

| Directory | Shows |
| --- | --- |
| `examples/minimal/` | Fluent `Application`, string + presence flags, `MissingOrUnknown` fallback. |
| `examples/nested/` | Nested `Group`s, `Arg` tails, `UnknownOnly` + path-like default to `read`. |

From the repo root:

```bash
just example-minimal ARGS='hello --name world'
just example-nested ARGS='stat owner lookup --user-name alice ./README.md'
```

Or enter an example directory:

```bash
cd examples/minimal && just build && just run ARGS='hello -h'
```

---

## Shell completions

```bash
myapp completion bash > ~/.bash_completion.d/myapp   # or: source <(myapp completion bash)
myapp completion zsh --print                         # inspect / redirect
myapp completion zsh                                 # install under ~/.zsh/completions/
```

---

## Public API overview

| Header | Contents |
| --- | --- |
| [`include/argsbarg/argsbarg.hpp`](include/argsbarg/argsbarg.hpp) | Umbrella: `version()`, `run`, `err_with_help`, includes the rest. |
| [`include/argsbarg/schema.hpp`](include/argsbarg/schema.hpp) | `Schema`, `Command`, `Option`, `FallbackMode`, `find_child`, `find_option_by_name`. |
| [`include/argsbarg/builders.hpp`](include/argsbarg/builders.hpp) | Fluent `Leaf`, `Group`, `Opt`, `Arg`. |
| [`include/argsbarg/application.hpp`](include/argsbarg/application.hpp) | Fluent `Application`. |
| [`include/argsbarg/context.hpp`](include/argsbarg/context.hpp) | `Context` passed to handlers. |
| [`include/argsbarg/parse.hpp`](include/argsbarg/parse.hpp) | `schema_validate`, `parse`, `post_parse_validate`. |
| [`include/argsbarg/help.hpp`](include/argsbarg/help.hpp) | `help_render`, `help_render_stderr`. |
| [`include/argsbarg/builtins.hpp`](include/argsbarg/builtins.hpp) | `merge_builtins`, completion path helpers. |
| [`include/argsbarg/completion.hpp`](include/argsbarg/completion.hpp) | `completion_*_script`, zsh install helper. |
| [`include/argsbarg/schema_error.hpp`](include/argsbarg/schema_error.hpp) | `SchemaError` (`std::logic_error`). |

---

## License

MIT — see [`LICENSE`](LICENSE).
