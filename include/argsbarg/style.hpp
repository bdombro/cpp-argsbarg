#pragma once

#include <string>
#include <string_view>

namespace argsbarg::style {

inline std::string wrap(std::string_view prefix, std::string_view body, std::string_view suffix) {
    return std::string{prefix} + std::string{body} + std::string{suffix};
}

inline std::string red(std::string_view msg) {
    constexpr std::string_view esc_red = "\033[31m";
    constexpr std::string_view esc_reset = "\033[0m";
    return wrap(esc_red, msg, esc_reset);
}
inline std::string green(std::string_view msg) {
    constexpr std::string_view esc_green = "\033[32m";
    constexpr std::string_view esc_reset = "\033[0m";
    return wrap(esc_green, msg, esc_reset);
}
inline std::string gray(std::string_view msg) {
    constexpr std::string_view esc_gray = "\033[90m";
    constexpr std::string_view esc_reset = "\033[0m";
    return wrap(esc_gray, msg, esc_reset);
}
inline std::string bold(std::string_view msg) {
    constexpr std::string_view esc_bold = "\033[1m";
    constexpr std::string_view esc_reset = "\033[0m";
    return wrap(esc_bold, msg, esc_reset);
}
inline std::string white(std::string_view msg) {
    constexpr std::string_view esc_white = "\033[37m";
    constexpr std::string_view esc_reset = "\033[0m";
    return wrap(esc_white, msg, esc_reset);
}
inline std::string aqua_bold(std::string_view msg) {
    constexpr std::string_view esc_aqua = "\033[96m";
    constexpr std::string_view esc_bold = "\033[1m";
    constexpr std::string_view esc_reset = "\033[0m";
    return wrap(esc_aqua, std::string{esc_bold} + std::string{msg}, esc_reset);
}
inline std::string green_bright(std::string_view msg) {
    constexpr std::string_view esc_green_bright = "\033[92m";
    constexpr std::string_view esc_reset = "\033[0m";
    return wrap(esc_green_bright, msg, esc_reset);
}

} // namespace argsbarg::style
