#pragma once

/// Declarations for bash/zsh completion script generation and zsh install helper.
///
/// Goal: ship shell integration without maintaining hand-written completion files per app.
/// Why: completions stay in sync with the live `Schema` tree.
/// How: string-generates shell script text; zsh path can install under `~/.zsh/completions/`.

#include "context.hpp"
#include "schema.hpp"

namespace argsbarg {

/// Full bash `complete -F` script for `schema.name`.
[[nodiscard]] std::string completion_bash_script(const Schema& merged);

/// `#compdef` zsh completion script body for `schema.name`.
[[nodiscard]] std::string completion_zsh_script(const Schema& merged);

/// Writes zsh script to `~/.zsh/completions/` or stdout when `--print` is set on the zsh leaf.
void completion_zsh_install_or_print(const Schema& merged, const Context& ctx);

} // namespace argsbarg

#include "argsbarg/detail/completion_bash_inline.hpp"
#include "argsbarg/detail/completion_zsh_inline.hpp"
