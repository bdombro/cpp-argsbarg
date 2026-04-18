# argsbarg feature specification

Normative behavior for the high-level host APIs (`Application::run`, `run(schema, argc, argv)`) and built-ins. Low-level helpers (`parse`, `help_render`, …) perform no I/O and do not exit.

## 1. Scope

- **Language:** C++23.
- **Platforms:** Unix-like systems with POSIX APIs used for TTY detection and completion install paths (see [`README.md`](README.md)). This document does not define Windows behavior.

## 2. Schema and built-ins

- `merge_builtins(schema)` injects reserved commands for `--help` / `-h` handling and the `completion` group (`completion bash`, `completion zsh`, …).
- `schema_validate(merged)` rejects invalid trees (duplicate names, missing handlers, invalid fallback configuration, reserved name collisions).

## 3. Parsing model (summary)

- Options are POSIX-style (`-x`, `-abc` bundles for presence flags, `--long`, `--long=value`).
- Routing resolves `Group` vs `Leaf`; positionals are configured with `Arg` builders.
- Detailed parse rules are covered by unit tests under `tests/`; this spec focuses on externally visible outcomes (exit status, help, fallback).

## 4. Fallback modes (`FallbackMode`)

See the table in [`README.md`](README.md#fallback-modes-fallbackmode). In short:

| Mode | Empty `argv` (no tokens) | Unknown first command token |
| --- | --- | --- |
| `MissingOnly` | Run default command | Error |
| `MissingOrUnknown` | Run default command | Run default command; unknown token starts argv for the default |
| `UnknownOnly` | Implicit root help (non-zero exit via `run`) | Run default command |

## 5. Default command and root argv

### 5.1 argv passed to the default command

When the default command runs, the `Context` receives options and positionals parsed for that leaf, consistent with the routed path.

### 5.2 Root options

Options declared on the application root are consumed before subcommand routing when they appear before the first positional or subcommand token, subject to parser rules.

### 5.3 Fallback and unrecognized root flags (`MissingOrUnknown`, `UnknownOnly`)

When fallback mode is `MissingOrUnknown` or `UnknownOnly`:

- If the first token is **not** a known root subcommand and is **not** consumed as a valid root option, parsing may **stop consuming root flags** and treat the remainder of the command line as input to the **default command** (including an initial token that would otherwise look like an unknown global flag).
- This allows patterns such as a file path or passthrough argv for a default subcommand without requiring a literal subcommand name.

Exact token classification is implemented in `parse` / `post_parse_validate`; regressions are guarded by tests (e.g. `tests/test_parse.cpp`, `tests/test_smoke.cpp`).

## 6. Help

- Explicit `-h` / `--help` produces help on **stdout** and exits with status **0**.
- Implicit help (e.g. empty argv where the mode implies help, or incomplete routing) produces help on **stdout** and exits with status **1** when routed through `run()` / `Application::run()`.

## 7. Errors

- Parse / validation errors print a message and contextual help on **stderr** and exit with status **1** (TTYs may get color via `isatty` on stderr).
- `err_with_help(ctx, "message")` prints the message and contextual help on **stderr** and exits with status **1**.

## 8. Process exit codes (`run` / `Application::run`)

All paths below refer to `std::exit` status from `run` in [`include/argsbarg/argsbarg.hpp`](include/argsbarg/argsbarg.hpp) unless otherwise noted.

| Status | Condition |
| ---: | --- |
| 0 | Successful handler return (implicit `std::exit(0)` after the leaf handler runs). |
| 0 | `completion bash` script printed to stdout. |
| 0 | `completion zsh` completed successfully (install or `--print`). |
| 0 | Explicit help (`-h` / `--help` with `help_explicit == true`). |
| 1 | Implicit help (`help_explicit == false`). |
| 1 | Parse / post-parse error. |
| 1 | `err_with_help`. |
| 1 | Internal inconsistency (missing handler for resolved path). |

**Note:** Handlers that invoke `std::exit` or `std::terminate` themselves are outside this table.

## 9. Completion scripts

- `completion bash` emits a script suitable for `bash` / `bash -n` validation.
- `completion zsh` can write under `~/.zsh/completions/` or print to stdout when `--print` is set; behavior is defined by the completion helpers in [`include/argsbarg/completion.hpp`](include/argsbarg/completion.hpp).
