#pragma once

/// Core schema types: options, commands, routing tree, and handler type.
///
/// Goal: describe the CLI as data (`Schema` / `Command` / `Option`) independent of parsing.
/// Why: lets `parse`, `help_render`, and completion generators share one representation.
/// How: POD-like structs plus small lookup helpers for child commands and option definitions.

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace argsbarg {

/// Forward declaration: full definition in `context.hpp`.
class Context;

/// User callback invoked with a `Context` after successful routing to a leaf.
using Handler = std::function<void(Context&)>;

/// Whether an option is a flag, string-valued, or parsed as a number.
enum class OptionKind { Presence, String, Number };

/// How an optional default command participates when argv is empty or the first token is unknown.
enum class FallbackMode {
    MissingOnly,
    MissingOrUnknown,
    UnknownOnly,
};

/// One CLI option or positional tail definition (see `Opt` / `Arg` builders).
struct Option {
    std::string name;
    std::string description;
    OptionKind kind{OptionKind::Presence};
    char short_name{'\0'};
    bool positional{false};
    int arg_min{1};
    int arg_max{1};
};

/// Either a routing group (`children` non-empty) or a leaf (`handler` set).
struct Command {
    std::string name;
    std::string description;
    std::string notes;
    std::vector<Option> options;
    std::vector<Option> positionals;
    std::vector<Command> children;
    std::optional<Handler> handler;
};

/// Root application: global options, top-level commands, and optional default command.
struct Schema {
    std::string name;
    std::string description;
    std::vector<Option> options;
    std::vector<Command> commands;
    std::optional<std::string> fallback_command;
    FallbackMode fallback_mode{FallbackMode::MissingOnly};
};

namespace {

/// Linear scan for a command by name within one routing layer.
inline const Command* find_cmd(const std::vector<Command>& cmds, std::string_view name) {
    for (const auto& c : cmds) {
        if (c.name == name) {
            return &c;
        }
    }
    return nullptr;
}

/// Linear scan for an option definition by long name in one scope.
inline const Option* find_opt(const std::vector<Option>& defs, std::string_view name) {
    for (const auto& o : defs) {
        if (o.name == name) {
            return &o;
        }
    }
    return nullptr;
}

} // namespace

/// Finds a direct child command by name (public wrapper used by parsers and help).
[[nodiscard]] inline const Command* find_child(const std::vector<Command>& cmds,
                                               std::string_view name) {
    return find_cmd(cmds, name);
}

/// Finds an option definition in a scope's option list (public wrapper).
[[nodiscard]] inline const Option* find_option_by_name(const std::vector<Option>& defs,
                                                       std::string_view name) {
    return find_opt(defs, name);
}

} // namespace argsbarg
