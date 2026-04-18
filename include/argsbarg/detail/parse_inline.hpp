#pragma once

#include "argsbarg/result.hpp"
#include "argsbarg/schema.hpp"
#include "argsbarg/schema_error.hpp"

#include <cstdlib>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace argsbarg {

namespace {

constexpr std::string_view k_help_long = "--help";
constexpr std::string_view k_help_short = "-h";

bool is_help_tok(std::string_view tok) {
    return tok == k_help_short || tok == k_help_long;
}

const Option* find_option_def_by_short(const std::vector<Option>& defs, char short_name) {
    for (const auto& o : defs) {
        if (!o.positional && o.short_name == short_name) {
            return &o;
        }
    }
    return nullptr;
}

struct ConsumeReport {
    std::optional<std::string> err;
    bool stopped_on_unknown{false};
};

ConsumeReport consume_options(const std::vector<Option>& defs, bool lenient_unknown, std::size_t& i,
                              const std::vector<std::string>& argv,
                              std::unordered_map<std::string, std::string>& opts) {

    auto consume_long = [&](std::string_view tok) -> std::optional<std::string> {
        // returns nullopt on ok, error message on err, use sentinel for lenient stop - encode as
        // optional with special - use ConsumeReport from inner

        std::string opt_name;
        std::string opt_val;
        const auto eq = tok.find('=');
        if (eq != std::string::npos) {
            opt_name = std::string(tok.substr(2, eq - 2));
            opt_val = std::string(tok.substr(eq + 1));
        } else {
            opt_name = std::string(tok.substr(2));
        }
        const auto* def = find_option_by_name(defs, opt_name);
        if (def == nullptr) {
            if (lenient_unknown) {
                return std::string{}; // empty string means lenient stop - caller checks
            }
            return std::string{"Unknown option: --" + opt_name};
        }
        if (eq == std::string::npos) {
            if (def->kind == OptionKind::Presence) {
                opt_val = "1";
            } else {
                ++i;
                if (i >= argv.size()) {
                    return std::string{"Missing value for option: --" + opt_name};
                }
                opt_val = argv[i];
            }
        } else if (def->kind == OptionKind::Presence) {
            // --flag=value for presence still ok
            opt_val = "1";
        }
        opts[def->name] = std::move(opt_val);
        ++i;
        return std::nullopt;
    };

    auto consume_short = [&](std::string_view tok) -> std::optional<std::string> {
        if (tok.size() < 2) {
            return std::string{"Unexpected option token: "} + std::string{tok};
        }
        const std::string_view shorts = tok.substr(1);
        for (std::size_t j = 0; j < shorts.size(); ++j) {
            const char short_name = shorts[j];
            const auto* def = find_option_def_by_short(defs, short_name);
            if (def == nullptr) {
                if (lenient_unknown) {
                    return std::string{};
                }
                return std::string{"Unknown option: -"} + short_name;
            }
            if (def->kind == OptionKind::Presence) {
                opts[def->name] = "1";
                continue;
            }
            if (shorts.size() != 1) {
                return std::string{"Short option -"} + short_name +
                       " requires a value and cannot be bundled: " + std::string{tok};
            }
            ++i;
            if (i >= argv.size()) {
                return std::string{"Missing value for option: -"} + short_name;
            }
            opts[def->name] = argv[i];
            ++i;
            return std::nullopt;
        }
        ++i;
        return std::nullopt;
    };

    while (i < argv.size()) {
        const auto& tok = argv[i];
        if (is_help_tok(tok)) {
            break;
        }
        if (!tok.starts_with('-')) {
            break;
        }
        if (tok.starts_with("--")) {
            const auto r = consume_long(tok);
            if (r.has_value()) {
                if (r->empty()) {
                    return {.stopped_on_unknown = true}; // i unchanged; unknown token remains
                }
                return {.err = r};
            }
        } else {
            const auto r = consume_short(tok);
            if (r.has_value()) {
                if (r->empty()) {
                    return {.stopped_on_unknown = true};
                }
                return {.err = r};
            }
        }
    }
    return {};
}

ParseResult finish_leaf(const Command& node, std::size_t& i, const std::vector<std::string>& argv,
                        const std::vector<std::string>& path,
                        const std::unordered_map<std::string, std::string>& opts) {

    auto error_result = [&](std::string msg) {
        ParseResult pr;
        pr.kind = ParseKind::Error;
        pr.error_msg = std::move(msg);
        pr.error_help_path = path;
        return pr;
    };

    std::vector<std::string> args;
    for (const auto& p : node.positionals) {
        if (!p.positional) {
            continue;
        }
        if (p.arg_max == 1) {
            if (p.arg_min >= 1) {
                if (i >= argv.size()) {
                    return error_result("Missing positional argument: " + p.name);
                }
                args.push_back(argv[i]);
                ++i;
            } else if (i < argv.size()) {
                args.push_back(argv[i]);
                ++i;
            }
            continue;
        }
        int count = 0;
        if (p.arg_max == 0) {
            while (i < argv.size()) {
                args.push_back(argv[i]);
                ++i;
                ++count;
            }
        } else {
            while (count < p.arg_max && i < argv.size()) {
                args.push_back(argv[i]);
                ++i;
                ++count;
            }
        }
        if (count < p.arg_min) {
            return error_result("Expected at least " + std::to_string(p.arg_min) +
                                " argument(s) for " + p.name + ", got " + std::to_string(count));
        }
    }
    if (i < argv.size()) {
        return error_result("Unexpected extra arguments");
    }
    ParseResult pr;
    pr.kind = ParseKind::Ok;
    pr.path = path;
    pr.opts = opts;
    pr.args = std::move(args);
    return pr;
}

} // namespace

inline ParseResult parse(const Schema& schema, const std::vector<std::string>& argv) {
    std::size_t i = 0;
    std::vector<std::string> path;
    std::unordered_map<std::string, std::string> opts;

    auto help_result = [&](std::vector<std::string> p, bool explicit_help) {
        ParseResult r;
        r.kind = ParseKind::Help;
        r.help_explicit = explicit_help;
        r.help_path = std::move(p);
        return r;
    };

    auto error_result = [&](std::string msg) {
        ParseResult r;
        r.kind = ParseKind::Error;
        r.error_msg = std::move(msg);
        r.error_help_path = path;
        return r;
    };

    const bool root_lenient = schema.fallback_command.has_value() &&
                              (schema.fallback_mode == FallbackMode::MissingOrUnknown ||
                               schema.fallback_mode == FallbackMode::UnknownOnly);

    const auto root_rep = consume_options(schema.options, root_lenient, i, argv, opts);
    if (root_rep.err.has_value()) {
        ParseResult r;
        r.kind = ParseKind::Error;
        r.error_msg = *root_rep.err;
        r.error_help_path = path;
        return r;
    }
    if (i < argv.size() && is_help_tok(argv[i])) {
        return help_result({}, true);
    }

    std::string cmd_name;
    const Command* node = nullptr;

    if (i >= argv.size()) {
        if (schema.fallback_command.has_value() &&
            (schema.fallback_mode == FallbackMode::MissingOnly ||
             schema.fallback_mode == FallbackMode::MissingOrUnknown)) {
            cmd_name = *schema.fallback_command;
            node = find_child(schema.commands, cmd_name);
            if (node == nullptr) {
                return error_result("Unknown command: " + cmd_name);
            }
        } else {
            return help_result({}, false);
        }
    } else {
        const auto& peek = argv[i];
        const auto* child_pick = find_child(schema.commands, peek);
        const bool can_route_unknown_to_default =
            schema.fallback_command.has_value() &&
            (schema.fallback_mode == FallbackMode::MissingOrUnknown ||
             schema.fallback_mode == FallbackMode::UnknownOnly);

        if (child_pick != nullptr) {
            cmd_name = peek;
            ++i;
            node = child_pick;
        } else if (can_route_unknown_to_default) {
            cmd_name = *schema.fallback_command;
            node = find_child(schema.commands, cmd_name);
            if (node == nullptr) {
                return error_result("Unknown command: " + cmd_name);
            }
            // Do not consume peek: unknown first token (§5) or unrecognized root flag tail (§5.3)
            // is parsed as argv for the default command.
        } else {
            cmd_name = peek;
            ++i;
            node = find_child(schema.commands, cmd_name);
            if (node == nullptr) {
                return error_result("Unknown command: " + cmd_name);
            }
        }
    }

    path.push_back(cmd_name);

    while (true) {
        const auto orep = consume_options(node->options, false, i, argv, opts);
        if (orep.err.has_value()) {
            ParseResult r;
            r.kind = ParseKind::Error;
            r.error_msg = *orep.err;
            r.error_help_path = path;
            return r;
        }
        if (i < argv.size() && is_help_tok(argv[i])) {
            return help_result(path, true);
        }

        if (i >= argv.size()) {
            if (!node->children.empty()) {
                return help_result(path, false);
            }
            return finish_leaf(*node, i, argv, path, opts);
        }

        const auto& tok = argv[i];
        if (tok.starts_with('-')) {
            ParseResult r;
            r.kind = ParseKind::Error;
            r.error_msg = "Unexpected option token: " + tok;
            r.error_help_path = path;
            return r;
        }

        const auto* child_opt = find_child(node->children, tok);
        if (child_opt != nullptr) {
            ++i;
            path.push_back(tok);
            node = child_opt;
            continue;
        }

        if (!node->children.empty()) {
            return error_result("Unknown subcommand: " + tok);
        }

        return finish_leaf(*node, i, argv, path, opts);
    }
}

inline ParseResult post_parse_validate(const Schema& merged, ParseResult pr) {
    if (pr.kind != ParseKind::Ok) {
        return pr;
    }

    std::vector<Option> defs = merged.options;
    auto cmds = merged.commands;
    for (const auto& seg : pr.path) {
        const auto* ch = find_child(cmds, seg);
        if (ch == nullptr) {
            ParseResult r;
            r.kind = ParseKind::Error;
            r.error_help_path = pr.path;
            r.error_msg = "Internal path error";
            return r;
        }
        defs.insert(defs.end(), ch->options.begin(), ch->options.end());
        defs.insert(defs.end(), ch->positionals.begin(), ch->positionals.end());
        cmds = ch->children;
    }

    for (const auto& [k, v] : pr.opts) {
        const auto* d = find_option_by_name(defs, k);
        if (d == nullptr) {
            ParseResult r;
            r.kind = ParseKind::Error;
            r.error_help_path = pr.path;
            r.error_msg = "Unknown option key: " + k;
            return r;
        }
        if (d->kind == OptionKind::Number) {
            char* end = nullptr;
            std::strtod(v.c_str(), &end);
            if (end != v.c_str() + v.size()) {
                ParseResult r;
                r.kind = ParseKind::Error;
                r.error_help_path = pr.path;
                r.error_msg = "Invalid number for option --" + k + ": " + v;
                return r;
            }
        }
    }
    return pr;
}

namespace {

void check_options(const std::vector<Option>& defs, const std::string& scope) {
    std::set<char> seen_shorts;
    for (const auto& d : defs) {
        if (d.short_name == '\0') {
            continue;
        }
        if (d.positional) {
            throw SchemaError("Positional arguments must not define short aliases: " + scope + "/" +
                              d.name);
        }
        if (d.short_name == 'h') {
            throw SchemaError("Short alias -h is reserved for help: " + scope + "/" + d.name);
        }
        if (seen_shorts.contains(d.short_name)) {
            throw SchemaError("Duplicate short alias -" + std::string(1, d.short_name) +
                              " in scope " + scope);
        }
        seen_shorts.insert(d.short_name);
    }
}

void check_positionals(const std::vector<Option>& defs, const std::string& scope) {
    std::vector<const Option*> pos;
    for (const auto& d : defs) {
        if (d.positional) {
            pos.push_back(&d);
        }
    }
    for (std::size_t idx = 0; idx < pos.size(); ++idx) {
        const auto& d = *pos[idx];
        if (d.arg_min < 0) {
            throw SchemaError("argMin must be >= 0 for positional " + scope + "/" + d.name);
        }
        if (d.arg_max < 0) {
            throw SchemaError("argMax must be >= 0 (use 0 for unlimited) for positional " + scope +
                              "/" + d.name);
        }
        if (d.arg_max > 0 && d.arg_min > d.arg_max) {
            throw SchemaError("argMin must not exceed argMax for positional " + scope + "/" +
                              d.name);
        }
        if (idx + 1 < pos.size() && d.arg_max == 0) {
            throw SchemaError("Unlimited positional (argMax == 0) must be last in scope " + scope);
        }
    }
    bool saw_optional = false;
    for (const auto* d : pos) {
        if (d->arg_min == 0) {
            saw_optional = true;
        } else if (saw_optional) {
            throw SchemaError("Required positional after optional in scope " + scope);
        }
    }
}

void walk_command(const Command& cmd) {
    if (!cmd.children.empty()) {
        if (cmd.handler.has_value()) {
            throw SchemaError("Routing command must not set handler: " + cmd.name);
        }
    } else {
        if (!cmd.handler.has_value()) {
            throw SchemaError("Leaf command requires handler: " + cmd.name);
        }
    }
    check_options(cmd.options, cmd.name);
    check_positionals(cmd.positionals, cmd.name);
    check_options(cmd.positionals, cmd.name);

    std::set<std::string> child_names;
    for (const auto& ch : cmd.children) {
        if (child_names.contains(ch.name)) {
            throw SchemaError("Duplicate child command name: " + cmd.name + "/" + ch.name);
        }
        child_names.insert(ch.name);
        walk_command(ch);
    }
}

} // namespace

inline void schema_validate(const Schema& merged) {
    if (!merged.fallback_command.has_value()) {
        if (merged.fallback_mode == FallbackMode::MissingOrUnknown ||
            merged.fallback_mode == FallbackMode::UnknownOnly) {
            throw SchemaError("this fallbackMode requires fallbackCommand");
        }
    }
    if (merged.fallback_command.has_value()) {
        const auto want = *merged.fallback_command;
        bool found = false;
        for (const auto& c : merged.commands) {
            if (c.name == want) {
                found = true;
                break;
            }
        }
        if (!found) {
            throw SchemaError("fallbackCommand not found in commands: " + want);
        }
    }

    std::set<std::string> top_names;
    for (const auto& c : merged.commands) {
        if (top_names.contains(c.name)) {
            throw SchemaError("Duplicate top-level command name: " + c.name);
        }
        top_names.insert(c.name);
        walk_command(c);
    }
}
} // namespace argsbarg
