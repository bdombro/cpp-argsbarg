#pragma once

#include <string>
#include <unordered_map>
#include <vector>

namespace argsbarg {

enum class ParseKind { Ok, Help, Error };

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
