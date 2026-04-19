#pragma once

/// Umbrella entry for argsbarg: high-level `run` / `err_with_help` and re-exports.
///
/// Goal: give apps a single `#include` that wires parsing, help, completions, and dispatch.
/// Why: consumers should not need to know internal header split across `parse`, `help`, etc.
/// How: includes the public headers, then implements `run` by composing merge, validate,
/// parse, help rendering, completion side paths, and handler invocation with `std::exit`.

#include "argsbarg/application.hpp"
#include "argsbarg/builders.hpp"
#include "argsbarg/builtins.hpp"
#include "argsbarg/completion.hpp"
#include "argsbarg/context.hpp"
#include "argsbarg/help.hpp"
#include "argsbarg/parse.hpp"
#include "argsbarg/schema.hpp"
#include "argsbarg/schema_error.hpp"

#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <utility>
#include <vector>

namespace argsbarg {

/// Library version string (semver-style), kept in sync with releases and CMake `project(VERSION)`.
[[nodiscard]] inline constexpr const char* version() noexcept {
    return "0.3.0";
}

/// Full host pipeline: merge built-ins, validate schema, parse argv, then help, errors,
/// completion scripts, or the matched leaf handler (always ends with `std::exit`).
inline void run(const Schema& schema, int argc, const char* const argv[]) {
    const Schema merged = merge_builtins(schema);
    schema_validate(merged);

    auto argv_words = [&] {
        std::vector<std::string> out;
        for (int i = 1; i < argc; ++i) {
            if (argv[i] != nullptr) {
                out.emplace_back(argv[i]);
            }
        }
        return out;
    }();

    auto pr = parse(merged, argv_words);
    pr = post_parse_validate(merged, pr);

    if (pr.kind == ParseKind::Help) {
        std::cout << help_render(merged, pr.help_path);
        std::exit(pr.help_explicit ? 0 : 1);
    }
    if (pr.kind == ParseKind::Error) {
        const bool color = isatty(STDERR_FILENO) != 0;
        std::cerr << (color ? style::red(pr.error_msg) : pr.error_msg) << '\n';
        std::cerr << help_render_stderr(merged, pr.error_help_path);
        std::exit(1);
    }

    if (is_builtin_completion_bash(pr.path)) {
        std::cout << completion_bash_script(merged);
        std::exit(0);
    }
    if (is_builtin_completion_zsh(pr.path)) {
        Context ctx(merged.name, pr.path, pr.opts, pr.args, merged);
        completion_zsh_install_or_print(merged, ctx);
        std::exit(0);
    }

    const std::vector<Command>* layer = &merged.commands;
    const Command* leaf = nullptr;
    for (const auto& seg : pr.path) {
        leaf = find_child(*layer, seg);
        if (leaf == nullptr) {
            std::cerr << "Internal error: missing handler for path.\n";
            std::exit(1);
        }
        layer = &leaf->children;
    }
    if (leaf == nullptr || !leaf->handler.has_value()) {
        std::cerr << "Internal error: missing handler for path.\n";
        std::exit(1);
    }

    Context ctx(merged.name, pr.path, std::move(pr.opts), std::move(pr.args), merged);
    (*leaf->handler)(ctx);
    std::exit(0);
}

/// Print `msg` and contextual help on stderr, then exit with status 1 (TTY-aware color).
[[noreturn]] inline void err_with_help(const Context& ctx, std::string msg) {
    const bool color = isatty(STDERR_FILENO) != 0;
    std::cerr << (color ? style::red(msg) : msg) << '\n';
    std::cerr << help_render_stderr(ctx.schema(), ctx.command_path());
    std::exit(1);
}

} // namespace argsbarg
