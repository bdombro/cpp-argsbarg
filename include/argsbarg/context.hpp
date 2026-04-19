#pragma once

/// Per-invocation view of argv state passed into leaf handlers.
///
/// Goal: give handlers typed accessors without re-parsing argv.
/// Why: keeps handler signatures small (`void(Context&)`) while exposing path, flags, and args.
/// How: stores resolved option map and positional vector plus a pointer to the merged schema.

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace argsbarg {

struct Schema;

/// Read-only access to app name, routed path, merged schema, flags, and positional argv tail.
class Context {
  public:
    /// Program display name (same as `Schema::name` after merge).
    [[nodiscard]] std::string_view app_name() const { return app_name_; }

    /// Routed command segments from root to the current leaf.
    [[nodiscard]] const std::vector<std::string>& command_path() const { return command_path_; }

    /// Positional arguments for the current leaf after option consumption.
    [[nodiscard]] const std::vector<std::string>& args() const { return args_; }

    /// Merged schema (built-ins included) for help and error rendering.
    [[nodiscard]] const Schema& schema() const { return *schema_; }

    /// Captures the routed path, option map, and positional tail at dispatch time.
    Context(std::string app_name, std::vector<std::string> command_path,
            std::unordered_map<std::string, std::string> opts, std::vector<std::string> args,
            const Schema& merged_schema)
        : app_name_(std::move(app_name)), command_path_(std::move(command_path)),
          opts_(std::move(opts)), args_(std::move(args)), schema_(&merged_schema) {}

    /// True if a presence option (or any stored value) exists for `name`.
    [[nodiscard]] bool flag(std::string_view name) const {
        const auto it = opts_.find(std::string{name});
        return it != opts_.end();
    }

    /// String option value, or absent if not provided.
    [[nodiscard]] std::optional<std::string> string_opt(std::string_view name) const {
        const auto it = opts_.find(std::string{name});
        if (it == opts_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    /// Parsed numeric option; returns nullopt if missing or not a finite number string.
    [[nodiscard]] std::optional<double> number_opt(std::string_view name) const {
        const auto it = opts_.find(std::string{name});
        if (it == opts_.end()) {
            return std::nullopt;
        }
        const auto& s = it->second;
        errno = 0;
        char* end = nullptr;
        const double v = std::strtod(s.c_str(), &end);
        if (end == s.c_str() || end != s.c_str() + s.size() || errno == ERANGE ||
            !std::isfinite(v)) {
            return std::nullopt;
        }
        return v;
    }

  private:
    std::string app_name_;
    std::vector<std::string> command_path_;
    std::unordered_map<std::string, std::string> opts_;
    std::vector<std::string> args_;
    const Schema* schema_{nullptr};
};

} // namespace argsbarg
