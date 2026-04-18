#pragma once

#include "argsbarg/builders.hpp"
#include "argsbarg/schema.hpp"
#include "argsbarg/schema_error.hpp"

#include <string>
#include <vector>

namespace argsbarg {

inline constexpr const char* k_builtin_completion = "completion";
inline constexpr const char* k_builtin_bash = "bash";
inline constexpr const char* k_builtin_zsh = "zsh";

[[nodiscard]] inline Schema merge_builtins(Schema schema) {
    for (const auto& c : schema.commands) {
        if (c.name == k_builtin_completion) {
            throw SchemaError(std::string("Reserved command name: ") + k_builtin_completion);
        }
    }
    schema.commands.push_back(
        Group{k_builtin_completion, "Generate the autocompletion script for shells."}
            .child(Leaf{k_builtin_bash, "Generate the autocompletion script for bash."}
                .handler(Handler{[](Context&) {}})
                .notes("Writes the completion script to stdout. Save to a file and source it from ~/.bashrc."))
            .child(Leaf{k_builtin_zsh, "Generate the autocompletion script for zsh."}
                .handler(Handler{[](Context&) {}})
                .option(Opt{"print", "Print script to stdout instead of installing to ~/.zsh/completions/."})
                .notes("Without --print, writes to ~/.zsh/completions/_{app}. Use --print to write to stdout.")));
    return schema;
}

[[nodiscard]] inline bool is_builtin_completion_bash(const std::vector<std::string>& path) {
    return path.size() == 2 && path[0] == k_builtin_completion && path[1] == k_builtin_bash;
}

[[nodiscard]] inline bool is_builtin_completion_zsh(const std::vector<std::string>& path) {
    return path.size() == 2 && path[0] == k_builtin_completion && path[1] == k_builtin_zsh;
}

} // namespace argsbarg
