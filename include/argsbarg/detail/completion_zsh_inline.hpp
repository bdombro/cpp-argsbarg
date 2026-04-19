#pragma once

/// Zsh completion script generation and optional install under `~/.zsh/completions/`.
///
/// Goal: emit `#compdef` scripts compatible with `compinit` and `_describe` / `_files`.
/// Why: zsh completion has different word indexing and array syntax than bash.
/// How: builds parallel helper functions and `compdef` registration; install path uses
/// `<filesystem>`.

#include "argsbarg/context.hpp"
#include "argsbarg/detail/completion_shared.hpp"
#include "argsbarg/schema.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace argsbarg {

/// String fragments for zsh completion (internal to this header).
namespace zsh_gen {

constexpr const char* k_help_long = "--help";
constexpr const char* k_help_short = "-h";

using detail::collect_scopes;
using detail::esc_shell_single_quoted;
using detail::ident_token;
using detail::ScopeRec;

/// Emits `typeset` arrays holding command and option descriptions per scope id.
inline std::string emit_scope_arrays(std::string_view ident, const std::vector<ScopeRec>& scopes) {
    std::ostringstream lines;
    for (std::size_t i = 0; i < scopes.size(); ++i) {
        const auto& sc = scopes[i];
        auto sorted_kids = sc.kids;
        std::sort(sorted_kids.begin(), sorted_kids.end(),
                  [](const Command& a, const Command& b) { return a.name < b.name; });
        lines << "typeset -g -a A_" << ident << "_" << i << "_cmds\n";
        lines << "A_" << ident << "_" << i << "_cmds=(";
        for (std::size_t k = 0; k < sorted_kids.size(); ++k) {
            if (k) {
                lines << ' ';
            }
            lines << '\'' << esc_shell_single_quoted(sorted_kids[k].name) << ':'
                  << esc_shell_single_quoted(sorted_kids[k].description) << '\'';
        }
        lines << ")\n";

        auto sorted_opts = sc.opts;
        std::sort(sorted_opts.begin(), sorted_opts.end(),
                  [](const Option& a, const Option& b) { return a.name < b.name; });
        lines << "typeset -g -a A_" << ident << "_" << i << "_opts\n";
        lines << "A_" << ident << "_" << i << "_opts=(";
        lines << '\'' << esc_shell_single_quoted(k_help_long) << ':'
              << esc_shell_single_quoted("Show help for this command.") << "' '"
              << esc_shell_single_quoted(k_help_short) << ':'
              << esc_shell_single_quoted("Show help for this command.") << '\'';
        for (const auto& o : sorted_opts) {
            if (o.positional) {
                continue;
            }
            std::string lab;
            switch (o.kind) {
            case OptionKind::Presence:
                lab = "--" + o.name;
                break;
            case OptionKind::Number:
                lab = "--" + o.name + "=<number>";
                break;
            case OptionKind::String:
                lab = "--" + o.name + "=<string>";
                break;
            }
            lines << " '" << esc_shell_single_quoted(lab) << ':'
                  << esc_shell_single_quoted(o.description) << '\'';
            if (o.short_name != '\0') {
                lines << " '" << esc_shell_single_quoted(std::string{'-', o.short_name}) << ':'
                      << esc_shell_single_quoted(o.description) << '\'';
            }
        }
        lines << ")\n";
        lines << "typeset -g A_" << ident << "_" << i << "_leaf=" << (sc.kids.empty() ? "1" : "0")
              << "\n";
        lines << "typeset -g A_" << ident << "_" << i << "_pos=" << (sc.wants_files ? "1" : "0")
              << "\n";
    }
    return lines.str();
}

/// Zsh variant of long-option consumption classifier (mirrors bash logic with zsh string ops).
inline std::string emit_consume_long_zsh(std::string_view ident,
                                         const std::vector<ScopeRec>& scopes) {
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

/// Zsh variant of short-option bundle classifier.
inline std::string emit_consume_short_zsh(std::string_view ident,
                                          const std::vector<ScopeRec>& scopes) {
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
        o << "        ch=${rest[1,1]}\n";
        o << "        rest=${rest[2,-1]}\n";
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

/// Maps a subcommand token to the next scope id for zsh (throws if path missing from index).
inline std::string emit_match_child_zsh(std::string_view ident, const std::vector<ScopeRec>& scopes,
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
            const int cid = path_index.at(child_path);
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

/// Replays `$words` up to `CURRENT` and stores the final scope id in `REPLY_SID`.
inline std::string emit_simulate_zsh(std::string_view ident) {
    std::ostringstream o;
    o << "_" << ident << "_nac_simulate() {\n";
    o << "  local i=2 sid=0 w steps next\n";
    o << "  while (( i < CURRENT )); do\n";
    o << "    w=$words[i]\n";
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
    o << "  REPLY_SID=$sid\n";
    o << "}\n";
    return o.str();
}

/// Main `_<app>` completion widget and `compdef` line for zsh.
inline std::string emit_main_body_zsh(const Schema& schema, std::string_view ident) {
    std::string main_name = schema.name;
    for (auto& c : main_name) {
        if (c == '-') {
            c = '_';
        }
    }
    std::ostringstream o;
    o << "_" << main_name << "() {\n";
    o << "  local curcontext=\"$curcontext\" ret=1\n";
    o << "  _" << ident << "_nac_simulate\n";
    o << "  local sid=$REPLY_SID\n";
    o << "  if [[ $PREFIX == -* ]]; then\n";
    o << "    local -a optsarr\n";
    o << "    local oname=\"A_" << ident << "_${sid}_opts\"\n";
    o << "    optsarr=(${(P@)oname})\n";
    o << "    _describe -t options 'option' optsarr && ret=0\n";
    o << "  else\n";
    o << "    local lname=\"A_" << ident << "_${sid}_leaf\"\n";
    o << "    if [[ ${(P)lname} -eq 0 ]]; then\n";
    o << "      local -a cmdsarr\n";
    o << "      local cname=\"A_" << ident << "_${sid}_cmds\"\n";
    o << "      cmdsarr=(${(P@)cname})\n";
    o << "      _describe -t commands 'command' cmdsarr && ret=0\n";
    o << "    else\n";
    o << "      local pname=\"A_" << ident << "_${sid}_pos\"\n";
    o << "      if [[ ${(P)pname} -eq 1 ]]; then\n";
    o << "        _files && ret=0\n";
    o << "      fi\n";
    o << "    fi\n";
    o << "  fi\n";
    o << "  return ret\n";
    o << "}\n\n";
    o << "compdef _" << main_name << " " << schema.name << "\n";
    return o.str();
}

} // namespace zsh_gen

/// Full zsh completion script (`#compdef` + helpers + `compdef`) for `schema.name`.
inline std::string completion_zsh_script(const Schema& schema) {
    const auto ident = detail::ident_token(schema.name);
    const auto scopes = detail::collect_scopes(schema);
    std::unordered_map<std::string, int> path_index;
    for (std::size_t i = 0; i < scopes.size(); ++i) {
        path_index[scopes[i].path] = static_cast<int>(i);
    }
    std::ostringstream out;
    out << "#compdef " << schema.name << "\n\n";
    out << zsh_gen::emit_scope_arrays(ident, scopes);
    out << zsh_gen::emit_consume_long_zsh(ident, scopes);
    out << zsh_gen::emit_consume_short_zsh(ident, scopes);
    out << zsh_gen::emit_match_child_zsh(ident, scopes, path_index);
    out << zsh_gen::emit_simulate_zsh(ident);
    out << zsh_gen::emit_main_body_zsh(schema, ident);
    return out.str();
}

/// Writes the zsh script to stdout (`--print`) or to `~/.zsh/completions/` with setup hints.
inline void completion_zsh_install_or_print(const Schema& merged, const Context& ctx) {
    const auto script = completion_zsh_script(merged);
    if (ctx.flag("print")) {
        std::cout << script;
        return;
    }
    const char* home = std::getenv("HOME");
    if (home == nullptr) {
        std::cerr << "HOME is not set; use --print and redirect manually.\n";
        std::cout << script;
        return;
    }
    std::filesystem::path dir = std::filesystem::path{home} / ".zsh" / "completions";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        std::cerr << "Could not create " << dir.string() << ": " << ec.message() << "\n";
        std::cerr << "Add to ~/.zshrc before compinit:\n";
        std::cerr << "  fpath=(" << dir.string() << " $fpath)\n";
        std::cout << script;
        return;
    }
    std::string fname = "_" + merged.name;
    for (auto& c : fname) {
        if (c == '-') {
            c = '_';
        }
    }
    const auto outpath = dir / fname;
    std::ofstream f(outpath);
    if (!f) {
        std::cerr << "Could not write " << outpath.string() << "\n";
        std::cout << script;
        return;
    }
    f << script;
    std::cerr << "Wrote " << outpath.string() << "\n";
    std::cerr << "Ensure ~/.zshrc contains before compinit:\n";
    std::cerr << "  fpath=(" << dir.string() << " $fpath)\n";
    std::cerr << "  autoload -Uz compinit && compinit\n";
}
} // namespace argsbarg
