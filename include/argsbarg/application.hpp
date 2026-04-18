#pragma once

#include "schema.hpp"

#include <string>
#include <utility>
#include <vector>

namespace argsbarg {

void run(const Schema& schema, int argc, const char* const argv[]);

class Application {
  public:
    explicit Application(std::string app_name) { schema_.name = std::move(app_name); }

    [[nodiscard]] Application& description(std::string desc) {
        schema_.description = std::move(desc);
        return *this;
    }

    [[nodiscard]] Application& root_options(std::vector<Option> opts) {
        schema_.options = std::move(opts);
        return *this;
    }

    [[nodiscard]] Application& command(Command cmd) {
        schema_.commands.push_back(std::move(cmd));
        return *this;
    }

    [[nodiscard]] Application& fallback(std::string cmd, FallbackMode mode) {
        schema_.fallback_command = std::move(cmd);
        schema_.fallback_mode = mode;
        return *this;
    }

    [[nodiscard]] Schema to_schema() const { return schema_; }

    void run(int argc, const char* const argv[]) const { argsbarg::run(schema_, argc, argv); }

  private:
    Schema schema_{};
};

} // namespace argsbarg
