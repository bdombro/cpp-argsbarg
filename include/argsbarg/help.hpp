#pragma once

#include "schema.hpp"

#include <string>
#include <vector>

namespace argsbarg {

[[nodiscard]] std::string help_render(const Schema& merged, const std::vector<std::string>& help_path);

/// Same as `help_render`, but terminal width / color follow stderr (for errors).
[[nodiscard]] std::string help_render_stderr(const Schema& merged, const std::vector<std::string>& help_path);

} // namespace argsbarg

#include "argsbarg/detail/help_formatter_inline.hpp"
