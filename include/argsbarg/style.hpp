#pragma once

/// Minimal ANSI SGR wrappers for stderr coloring in errors and help.
///
/// Goal: keep color logic tiny and header-only with no runtime dependency.
/// Why: TTY detection lives in `run` / `err_with_help`; these helpers only wrap strings.
/// How: concatenates escape sequences around UTF-8 safe string views.

#include <string>
#include <string_view>

namespace argsbarg::style {

/// Wraps `body` with `prefix` and `suffix` (used to inject SGR codes).
inline std::string wrap(std::string_view prefix, std::string_view body, std::string_view suffix) {
    return std::string{prefix} + std::string{body} + std::string{suffix};
}

/// Red foreground for error lines.
inline std::string red(std::string_view msg) {
    constexpr std::string_view esc_red = "\033[31m";
    constexpr std::string_view esc_reset = "\033[0m";
    return wrap(esc_red, msg, esc_reset);
}

/// Green foreground (success hints).
inline std::string green(std::string_view msg) {
    constexpr std::string_view esc_green = "\033[32m";
    constexpr std::string_view esc_reset = "\033[0m";
    return wrap(esc_green, msg, esc_reset);
}

/// Dim gray for secondary text.
inline std::string gray(std::string_view msg) {
    constexpr std::string_view esc_gray = "\033[90m";
    constexpr std::string_view esc_reset = "\033[0m";
    return wrap(esc_gray, msg, esc_reset);
}

/// Bold emphasis.
inline std::string bold(std::string_view msg) {
    constexpr std::string_view esc_bold = "\033[1m";
    constexpr std::string_view esc_reset = "\033[0m";
    return wrap(esc_bold, msg, esc_reset);
}

/// Default bright foreground.
inline std::string white(std::string_view msg) {
    constexpr std::string_view esc_white = "\033[37m";
    constexpr std::string_view esc_reset = "\033[0m";
    return wrap(esc_white, msg, esc_reset);
}

/// Cyan + bold for section titles in help.
inline std::string aqua_bold(std::string_view msg) {
    constexpr std::string_view esc_aqua = "\033[96m";
    constexpr std::string_view esc_bold = "\033[1m";
    constexpr std::string_view esc_reset = "\033[0m";
    return wrap(esc_aqua, std::string{esc_bold} + std::string{msg}, esc_reset);
}

/// Bright green for highlighted labels.
inline std::string green_bright(std::string_view msg) {
    constexpr std::string_view esc_green_bright = "\033[92m";
    constexpr std::string_view esc_reset = "\033[0m";
    return wrap(esc_green_bright, msg, esc_reset);
}

} // namespace argsbarg::style
