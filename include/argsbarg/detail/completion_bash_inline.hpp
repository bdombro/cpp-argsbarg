#pragma once

/// Bash programmable completion script generation (`complete -F`).
///
/// Goal: emit a self-contained script that tracks argv position against the `Schema` tree.
/// Why: bash is the most common target for generated CLI completions in POSIX environments.
/// How: string-builds helper functions then registers `complete -F _<app> <app>`.

#include "argsbarg/detail/completion_shared.hpp"
#include "argsbarg/schema.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace argsbarg {

/// String fragments for bash `compgen` completion (internal to this header).
namespace bash_gen {

constexpr const char* k_help_long = "--help";
constexpr const char* k_help_short = "-h";

using detail::collect_scopes;
using detail::esc_shell_single_quoted;
using detail::ident_token;
using detail::ScopeRec;

/// Emits bash function `_<ident>_nac_consume_long` classifying `--long` tokens per scope id.
inline std::string emit_consume_long(std::string_view ident, const std::vector<ScopeRec>& scopes) {
    std::ostringstream o;
    o << "_" << ident << "_nac_consume_long() {\n";
    o << "  local sid=\"$1\" w=\"$2\" nw=\"$3\"\n";
    o << "  case $sid in\n";
    for (std::size_t i = 0; i < scopes.size(); ++i) {
        const auto& sc = scopes[i];
        o << "    " << i << ")\n";
        o << "      case $w in\n";
        o << "        " << k_help_long << "|" << k_help_long << "=*|" << k_help_short
          << ") echo 1 ;;\n";
        for (const auto& op : sc.opts) {
            if (op.positional) {
                continue;
            }
            const std::string base = "--" + op.name;
            if (op.kind == OptionKind::Presence) {
                o << "        " << base << "|" << base << "=*) echo 1 ;;\n";
            } else {
                o << "        " << base << "=*) echo 1 ;;\n";
                o << "        " << base << ") echo 2 ;;\n";
            }
        }
        o << "        *) echo 0 ;;\n";
        o << "      esac\n";
        o << "      ;;\n";
    }
    o << "    *) echo 0 ;;\n";
    o << "  esac\n";
    o << "}\n";
    return o.str();
}

/// Emits bash function `_<ident>_nac_consume_short` for bundled short options at a scope.
inline std::string emit_consume_short(std::string_view ident, const std::vector<ScopeRec>& scopes) {
    std::ostringstream o;
    o << "_" << ident << "_nac_consume_short() {\n";
    o << "  local sid=\"$1\" w=\"$2\"\n";
    o << "  case $sid in\n";
    for (std::size_t i = 0; i < scopes.size(); ++i) {
        const auto& sc = scopes[i];
        o << "    " << i << ")\n";
        o << "      local rest=${w#-}\n";
        o << "      local ch\n";
        o << "      local saw=0\n";
        o << "      while [[ -n $rest ]]; do\n";
        o << "        ch=${rest:0:1}\n";
        o << "        rest=${rest:1}\n";
        o << "        case $ch in\n";
        std::string bool_chars;
        for (const auto& op : sc.opts) {
            if (op.positional || op.short_name == '\0') {
                continue;
            }
            if (op.kind == OptionKind::Presence) {
                bool_chars += op.short_name;
                bool_chars += '|';
            } else {
                o << "          " << op.short_name << ")\n";
                o << "            if [[ $saw -ne 0 || -n $rest ]]; then echo 0; return; fi\n";
                o << "            echo 2; return ;;\n";
            }
        }
        if (!bool_chars.empty()) {
            bool_chars.pop_back();
            o << "          " << bool_chars << ") ;;\n";
        }
        o << "          *) echo 0; return ;;\n";
        o << "        esac\n";
        o << "        saw=1\n";
        o << "      done\n";
        o << "      echo 1\n";
        o << "      ;;\n";
    }
    o << "    *) echo 0 ;;\n";
    o << "  esac\n";
    o << "}\n";
    return o.str();
}

/// Emits bash function mapping a command token to the child scope index (or failure).
inline std::string emit_match_child(std::string_view ident, const std::vector<ScopeRec>& scopes,
                                    const std::unordered_map<std::string, int>& path_index) {
    std::ostringstream o;
    o << "_" << ident << "_nac_match_child() {\n";
    o << "  local sid=\"$1\" w=\"$2\"\n";
    o << "  case $sid in\n";
    for (std::size_t sid = 0; sid < scopes.size(); ++sid) {
        const auto& sc = scopes[sid];
        if (sc.kids.empty()) {
            continue;
        }
        o << "    " << sid << ")\n";
        o << "      case $w in\n";
        for (const auto& ch : sc.kids) {
            const std::string child_path = sc.path.empty() ? ch.name : (sc.path + "/" + ch.name);
            const auto it = path_index.find(child_path);
            const int cid = it != path_index.end() ? it->second : 0;
            o << "        " << ch.name << ") echo " << cid << "; return 0 ;;\n";
        }
        o << "      esac\n";
        o << "      ;;\n";
    }
    o << "  esac\n";
    o << "  return 1\n";
    o << "}\n";
    return o.str();
}

/// Emits bash function that replays argv up to `cword` and prints the final scope id.
inline std::string emit_simulate(std::string_view ident) {
    std::ostringstream o;
    o << "_" << ident << "_nac_simulate() {\n";
    o << "  local i=1 sid=0 w steps next\n";
    o << "  while (( i < cword )); do\n";
    o << "    w=${words[i]}\n";
    o << "    if [[ $w == " << k_help_short << " || $w == " << k_help_long << " ]]; then\n";
    o << "      ((i++)); continue\n";
    o << "    fi\n";
    o << "    if [[ $w == --* ]]; then\n";
    o << "      steps=$(_" << ident << "_nac_consume_long \"$sid\" \"$w\" \"${words[i+1]}\")\n";
    o << "      case $steps in\n";
    o << "        0) break ;;\n";
    o << "        1) ((i++)) ;;\n";
    o << "        2) ((i+=2)) ;;\n";
    o << "        *) break ;;\n";
    o << "      esac\n";
    o << "      continue\n";
    o << "    fi\n";
    o << "    if [[ $w == -* ]]; then\n";
    o << "      steps=$(_" << ident << "_nac_consume_short \"$sid\" \"$w\")\n";
    o << "      case $steps in\n";
    o << "        0) break ;;\n";
    o << "        1) ((i++)) ;;\n";
    o << "        2) ((i++)); break ;;\n";
    o << "        *) break ;;\n";
    o << "      esac\n";
    o << "      continue\n";
    o << "    fi\n";
    o << "    next=$(_" << ident << "_nac_match_child \"$sid\" \"$w\") || break\n";
    o << "    sid=$next\n";
    o << "    ((i++))\n";
    o << "  done\n";
    o << "  printf '%s\\n' \"$sid\"\n";
    o << "}\n";
    return o.str();
}

/// Emits the `_<app>` completion function plus `complete -F` registration line.
inline std::string emit_main_body(const Schema& schema, std::string_view ident,
                                  const std::vector<ScopeRec>& scopes) {
    std::string main_name = schema.name;
    for (auto& c : main_name) {
        if (c == '-') {
            c = '_';
        }
    }
    std::ostringstream o;
    o << "_" << main_name << "() {\n";
    o << "  local cur prev words cword split=false\n";
    o << "  _init_completion -s || return\n";
    for (std::size_t i = 0; i < scopes.size(); ++i) {
        const auto& sc = scopes[i];
        auto sorted_kids = sc.kids;
        std::sort(sorted_kids.begin(), sorted_kids.end(),
                  [](const Command& a, const Command& b) { return a.name < b.name; });
        o << "  local -a _" << ident << "_cmds_" << i << "=(";
        for (std::size_t k = 0; k < sorted_kids.size(); ++k) {
            if (k) {
                o << ' ';
            }
            o << '\'' << esc_shell_single_quoted(sorted_kids[k].name) << '\'';
        }
        o << ")\n";
        auto sorted_opts = sc.opts;
        std::sort(sorted_opts.begin(), sorted_opts.end(),
                  [](const Option& a, const Option& b) { return a.name < b.name; });
        o << "  local -a _" << ident << "_opts_" << i << "=(";
        o << '\'' << esc_shell_single_quoted(k_help_long) << "' '"
          << esc_shell_single_quoted(k_help_short) << '\'';
        for (const auto& op : sorted_opts) {
            if (op.positional) {
                continue;
            }
            o << ' ';
            if (op.kind == OptionKind::Presence) {
                o << '\'' << esc_shell_single_quoted("--" + op.name) << '\'';
            } else {
                o << '\'' << esc_shell_single_quoted("--" + op.name + "=") << '\'';
            }
            if (op.short_name != '\0') {
                o << " '" << esc_shell_single_quoted(std::string{'-', op.short_name}) << '\'';
            }
        }
        o << ")\n";
        o << "  local _" << ident << "_leaf_" << i << "=" << (sc.kids.empty() ? "1" : "0") << "\n";
        o << "  local _" << ident << "_pos_" << i << "=" << (sc.wants_files ? "1" : "0") << "\n";
    }
    o << "  local sid\n";
    o << "  sid=$(_" << ident << "_nac_simulate)\n";
    o << "  if [[ $cur == -* ]]; then\n";
    o << "    case $sid in\n";
    for (std::size_t i = 0; i < scopes.size(); ++i) {
        o << "      " << i << ") COMPREPLY=( $(compgen -W \"${_" << ident << "_opts_" << i
          << "[*]}\" -- \"$cur\") ) ;;\n";
    }
    o << "    esac\n";
    o << "  else\n";
    o << "    case $sid in\n";
    for (std::size_t i = 0; i < scopes.size(); ++i) {
        o << "      " << i << ")\n";
        o << "        if [[ ${_" << ident << "_leaf_" << i << "} -eq 0 ]]; then\n";
        o << "          COMPREPLY=( $(compgen -W \"${_" << ident << "_cmds_" << i
          << "[*]}\" -- \"$cur\") )\n";
        o << "        elif [[ ${_" << ident << "_pos_" << i << "} -eq 1 ]]; then\n";
        o << "          COMPREPLY=( $(compgen -f -- \"$cur\") )\n";
        o << "        fi ;;\n";
    }
    o << "    esac\n";
    o << "  fi\n";
    o << "}\n\n";
    o << "complete -F _" << main_name << " " << schema.name << "\n";
    return o.str();
}

} // namespace bash_gen

/// Returns the full bash completion script for `schema.name` (comments + functions + `complete`).
inline std::string completion_bash_script(const Schema& schema) {
    const auto ident = detail::ident_token(schema.name);
    const auto scopes = detail::collect_scopes(schema);
    std::unordered_map<std::string, int> path_index;
    for (std::size_t i = 0; i < scopes.size(); ++i) {
        path_index[scopes[i].path] = static_cast<int>(i);
    }
    std::ostringstream out;
    out << "# Generated bash completion for " << schema.name << ".\n\n";
    out << bash_gen::emit_consume_long(ident, scopes);
    out << bash_gen::emit_consume_short(ident, scopes);
    out << bash_gen::emit_match_child(ident, scopes, path_index);
    out << bash_gen::emit_simulate(ident);
    out << bash_gen::emit_main_body(schema, ident, scopes);
    return out.str();
}
} // namespace argsbarg
