#pragma once

/// Outcome of `parse` / `post_parse_validate`: success path, help, or structured error.
///
/// Goal: represent everything a host needs without printing or exiting.
/// Why: tests and custom CLIs can branch on `ParseKind` without pulling in `run`.
/// How: small POD-like struct with union-ish fields keyed by `kind`.

#include <string>
#include <unordered_map>
#include <vector>

namespace argsbarg {

/// Discriminator for which fields in `ParseResult` are meaningful.
enum class ParseKind { Ok, Help, Error };

/// Parsed argv state: command path, options map, positional tail, or help/error metadata.
struct ParseResult {
    ParseKind kind{ParseKind::Ok};
    // Ok:
    std::vector<std::string> path;
    std::unordered_map<std::string, std::string> opts;
    std::vector<std::string> args;
    // Help:
    bool help_explicit{false};
    std::vector<std::string> help_path;
    // Error:
    std::string error_msg;
    std::vector<std::string> error_help_path;
};

} // namespace argsbarg
