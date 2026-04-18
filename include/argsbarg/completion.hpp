#pragma once

#include "context.hpp"
#include "schema.hpp"

namespace argsbarg {

[[nodiscard]] std::string completion_bash_script(const Schema& merged);
[[nodiscard]] std::string completion_zsh_script(const Schema& merged);

void completion_zsh_install_or_print(const Schema& merged, const Context& ctx);

} // namespace argsbarg

#include "argsbarg/detail/completion_bash_inline.hpp"
#include "argsbarg/detail/completion_zsh_inline.hpp"
