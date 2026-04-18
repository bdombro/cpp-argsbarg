#pragma once

#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace argsbarg {

struct Schema;

class Context {
  public:
    [[nodiscard]] std::string_view app_name() const { return app_name_; }
    [[nodiscard]] const std::vector<std::string>& command_path() const { return command_path_; }
    [[nodiscard]] const std::vector<std::string>& args() const { return args_; }
    [[nodiscard]] const Schema& schema() const { return *schema_; }

    Context(std::string app_name, std::vector<std::string> command_path,
            std::unordered_map<std::string, std::string> opts, std::vector<std::string> args,
            const Schema& merged_schema)
        : app_name_(std::move(app_name)), command_path_(std::move(command_path)),
          opts_(std::move(opts)), args_(std::move(args)), schema_(&merged_schema) {}

    [[nodiscard]] bool flag(std::string_view name) const {
        const auto it = opts_.find(std::string{name});
        return it != opts_.end();
    }

    [[nodiscard]] std::optional<std::string> string_opt(std::string_view name) const {
        const auto it = opts_.find(std::string{name});
        if (it == opts_.end()) {
            return std::nullopt;
        }
        return it->second;
    }

    [[nodiscard]] std::optional<double> number_opt(std::string_view name) const {
        const auto it = opts_.find(std::string{name});
        if (it == opts_.end()) {
            return std::nullopt;
        }
        const auto& s = it->second;
        double v{};
        const auto* first = s.data();
        const auto* last = s.data() + s.size();
        const auto res = std::from_chars(first, last, v);
        if (res.ec != std::errc{} || res.ptr != last) {
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
