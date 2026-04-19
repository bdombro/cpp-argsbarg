#pragma once

/// Fluent façade over `Schema` that mirrors the Nim-style `Application` host API.
///
/// Goal: ergonomic authoring of app metadata and commands without touching `Schema` fields.
/// Why: keeps call sites readable (`Application{"app"}.command(...).run(argc, argv)`).
/// How: mutates an internal `Schema`, then forwards to `argsbarg::run` on `run()`.

#include "schema.hpp"

#include <string>
#include <utility>
#include <vector>

namespace argsbarg {

/// Forward declaration; implementation lives in `argsbarg.hpp` after all helpers are visible.
void run(const Schema& schema, int argc, const char* const argv[]);

/// Builder for the root application: name, description, root options, commands, and fallback.
class Application {
  public:
    /// Sets the program name used in help and completion output.
    explicit Application(std::string app_name) { schema_.name = std::move(app_name); }

    /// One-line summary shown at the root of help.
    [[nodiscard]] Application& description(std::string desc) {
        schema_.description = std::move(desc);
        return *this;
    }

    /// Options parsed before any subcommand token (global flags).
    [[nodiscard]] Application& root_options(std::vector<Option> opts) {
        schema_.options = std::move(opts);
        return *this;
    }

    /// Registers a top-level `Leaf` or `Group` command.
    [[nodiscard]] Application& command(Command cmd) {
        schema_.commands.push_back(std::move(cmd));
        return *this;
    }

    /// Default command when argv is empty or unknown, depending on `FallbackMode`.
    [[nodiscard]] Application& fallback(std::string cmd, FallbackMode mode) {
        schema_.fallback_command = std::move(cmd);
        schema_.fallback_mode = mode;
        return *this;
    }

    /// Snapshot of the built schema (for tests or custom hosts).
    [[nodiscard]] Schema to_schema() const { return schema_; }

    /// Runs the full parse → help/error/completion/handler pipeline (see `run` in `argsbarg.hpp`).
    void run(int argc, const char* const argv[]) const { argsbarg::run(schema_, argc, argv); }

  private:
    Schema schema_{};
};

} // namespace argsbarg
