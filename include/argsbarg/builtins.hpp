#pragma once

/// Reserved `completion` command group and helpers to detect completion argv paths.
///
/// Goal: inject bash/zsh completion subcommands without colliding with user command names.
/// Why: every app should get completions from one predictable entry point.
/// How: appends a synthetic `Group` named `completion` with `bash` / `zsh` leaves to `Schema`.

#include "argsbarg/builders.hpp"
#include "argsbarg/schema.hpp"
#include "argsbarg/schema_error.hpp"

#include <string>
#include <vector>

namespace argsbarg {

inline constexpr const char* k_builtin_completion = "completion";
inline constexpr const char* k_builtin_bash = "bash";
inline constexpr const char* k_builtin_zsh = "zsh";

/// Returns a copy of `schema` with built-in completion commands merged (throws on name clash).
[[nodiscard]] inline Schema merge_builtins(Schema schema) {
    for (const auto& c : schema.commands) {
        if (c.name == k_builtin_completion) {
            throw SchemaError(std::string("Reserved command name: ") + k_builtin_completion);
        }
    }
    std::string zsh_completion_file = "_" + schema.name;
    for (char& c : zsh_completion_file) {
        if (c == '-') {
            c = '_';
        }
    }
    schema.commands.push_back(
        Group{k_builtin_completion, "Generate the autocompletion script for shells."}
            .child(
                Leaf{k_builtin_bash, "Generate the autocompletion script for bash."}
                    .handler(Handler{[](Context&) {}})
                    .notes("Prints the bash completion script to stdout only (no automatic "
                           "install).\n\n"
                           "Redirect to a file or use process substitution if needed."))
            .child(
                Leaf{k_builtin_zsh, "Generate the autocompletion script for zsh."}
                    .handler(Handler{[](Context&) {}})
                    .notes(
                        std::string("Prints the zsh #compdef script to stdout only (no automatic "
                                    "install).\n\n"
                                    "In ~/.zshrc you can use e.g. eval \"$(") +
                        schema.name +
                        std::string(" completion zsh)\" without adding a file to fpath.\n\n"
                                    "Or redirect to e.g. ~/.zsh/completions/") +
                        zsh_completion_file +
                        " on fpath, or use a temp file, as you prefer.")));
    return schema;
}

/// True when argv routed to `completion bash`.
[[nodiscard]] inline bool is_builtin_completion_bash(const std::vector<std::string>& path) {
    return path.size() == 2 && path[0] == k_builtin_completion && path[1] == k_builtin_bash;
}

/// True when argv routed to `completion zsh`.
[[nodiscard]] inline bool is_builtin_completion_zsh(const std::vector<std::string>& path) {
    return path.size() == 2 && path[0] == k_builtin_completion && path[1] == k_builtin_zsh;
}

} // namespace argsbarg
