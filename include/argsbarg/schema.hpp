#pragma once

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace argsbarg {

class Context;

using Handler = std::function<void(Context&)>;

enum class OptionKind { Presence, String, Number };

enum class FallbackMode {
    MissingOnly,
    MissingOrUnknown,
    UnknownOnly,
};

struct Option {
    std::string name;
    std::string description;
    OptionKind kind{OptionKind::Presence};
    char short_name{'\0'};
    bool positional{false};
    int arg_min{1};
    int arg_max{1};
};

struct Command {
    std::string name;
    std::string description;
    std::string notes;
    std::vector<Option> options;
    std::vector<Option> positionals;
    std::vector<Command> children;
    std::optional<Handler> handler;
};

struct Schema {
    std::string name;
    std::string description;
    std::vector<Option> options;
    std::vector<Command> commands;
    std::optional<std::string> fallback_command;
    FallbackMode fallback_mode{FallbackMode::MissingOnly};
};

namespace {

inline const Command* find_cmd(const std::vector<Command>& cmds, std::string_view name) {
    for (const auto& c : cmds) {
        if (c.name == name) {
            return &c;
        }
    }
    return nullptr;
}

inline const Option* find_opt(const std::vector<Option>& defs, std::string_view name) {
    for (const auto& o : defs) {
        if (o.name == name) {
            return &o;
        }
    }
    return nullptr;
}

} // namespace

[[nodiscard]] inline const Command* find_child(const std::vector<Command>& cmds,
                                               std::string_view name) {
    return find_cmd(cmds, name);
}

[[nodiscard]] inline const Option* find_option_by_name(const std::vector<Option>& defs,
                                                       std::string_view name) {
    return find_opt(defs, name);
}

} // namespace argsbarg
