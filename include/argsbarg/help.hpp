#pragma once

/// Declarations for contextual help rendering (stdout vs stderr width and color).
///
/// Goal: share one formatter between implicit help, explicit `-h`, and error footers.
/// Why: stderr help should follow stderr TTY width for wrapped tables.
/// How: declares `help_render` / `help_render_stderr`; bodies live in
/// `detail/help_formatter_inline.hpp`.

#include "schema.hpp"

#include <string>
#include <vector>

namespace argsbarg {

/// Renders help for `help_path` using stdout TTY width when available.
[[nodiscard]] std::string help_render(const Schema& merged,
                                      const std::vector<std::string>& help_path);

/// Same layout as `help_render`, but terminal width and color follow stderr (for errors).
[[nodiscard]] std::string help_render_stderr(const Schema& merged,
                                             const std::vector<std::string>& help_path);

} // namespace argsbarg

#include "argsbarg/detail/help_formatter_inline.hpp"
