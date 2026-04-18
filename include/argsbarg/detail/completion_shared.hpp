#pragma once

#include "argsbarg/schema.hpp"

#include <cctype>
#include <string>
#include <string_view>
#include <vector>

namespace argsbarg::detail {

struct ScopeRec {
    std::vector<Command> kids;
    std::vector<Option> opts;
    std::string path;
    bool wants_files{false};
};

[[nodiscard]] inline std::string esc_shell_single_quoted(std::string_view s) {
    std::string r;
    r.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '\'') {
            r += "'\\''";
        } else if (c == '\n') {
            r += ' ';
        } else {
            r += c;
        }
    }
    return r;
}

[[nodiscard]] inline bool has_positional_arguments(const Command& cmd) {
    for (const auto& a : cmd.positionals) {
        if (a.positional) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] inline std::string ident_token(std::string_view s) {
    std::string r;
    for (char c : s) {
        if (std::isalnum(static_cast<unsigned char>(c)) != 0) {
            r += c;
        } else {
            r += '_';
        }
    }
    return r;
}

inline void walk_scopes(const std::string& cmd_path, const Command& cmd, std::vector<ScopeRec>& acc) {
    acc.push_back(ScopeRec{
        .kids = cmd.children,
        .opts = cmd.options,
        .path = cmd_path,
        .wants_files = has_positional_arguments(cmd),
    });
    for (const auto& ch : cmd.children) {
        const auto next_path = cmd_path.empty() ? ch.name : (cmd_path + "/" + ch.name);
        walk_scopes(next_path, ch, acc);
    }
}

[[nodiscard]] inline std::vector<ScopeRec> collect_scopes(const Schema& schema) {
    std::vector<ScopeRec> acc;
    acc.push_back(ScopeRec{
        .kids = schema.commands,
        .opts = schema.options,
        .path = "",
        .wants_files = false,
    });
    for (const auto& c : schema.commands) {
        walk_scopes(c.name, c, acc);
    }
    return acc;
}

} // namespace argsbarg::detail
