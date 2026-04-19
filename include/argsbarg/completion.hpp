#pragma once

/// Declarations for bash/zsh completion script generation.
///
/// Goal: ship shell integration without maintaining hand-written completion files per app.
/// Why: completions stay in sync with the live `Schema` tree.
/// How: string-generates shell script text for hosts to print (stdout / redirect).

#include "schema.hpp"

namespace argsbarg {

/// Full bash `complete -F` script for `schema.name`.
[[nodiscard]] std::string completion_bash_script(const Schema& merged);

/// `#compdef` zsh completion script body for `schema.name`.
[[nodiscard]] std::string completion_zsh_script(const Schema& merged);

} // namespace argsbarg

#include "argsbarg/detail/completion_bash_inline.hpp"
#include "argsbarg/detail/completion_zsh_inline.hpp"
