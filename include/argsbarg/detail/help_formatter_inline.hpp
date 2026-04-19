#pragma once

/// Box-drawn help text: terminal width, UTF-8 column counting, and optional ANSI color.
///
/// Goal: implement `help_render` / `help_render_stderr` in one header for consumers of `help.hpp`.
/// Why: keeps help layout identical to the reference Nim implementation without a link dependency.
/// How: measures `TIOCGWINSZ`, skips ANSI sequences for width, emits bordered sections and tables.

#include "argsbarg/schema.hpp"
#include "argsbarg/style.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

namespace argsbarg {

/// Helpers and layout primitives for contextual help (not intended as a public API).
namespace help_fmt_detail {

// UTF-8 box drawing (aligned with nim-argsbarg help.nim)
inline constexpr std::string_view k_box_tl = "\xe2\x95\xad"; // ╭
inline constexpr std::string_view k_box_tr = "\xe2\x95\xae"; // ╮
inline constexpr std::string_view k_box_v = "\xe2\x94\x82";  // │
inline constexpr std::string_view k_box_bl = "\xe2\x95\xb0"; // ╰
inline constexpr std::string_view k_box_br = "\xe2\x95\xaf"; // ╯
inline constexpr std::string_view k_box_h = "\xe2\x94\x80";  // ─

/// Returns the index after the UTF-8 codepoint starting at `i` (replacement char advance on bad
/// leading bytes).
inline std::size_t utf8_codepoint_advance(std::string_view s, std::size_t i) {
    if (i >= s.size()) {
        return i;
    }
    const unsigned char c = static_cast<unsigned char>(s[i]);
    if (c < 0x80) {
        return i + 1;
    }
    if ((c >> 5) == 0x6) {
        return std::min(s.size(), i + 2);
    }
    if ((c >> 4) == 0xe) {
        return std::min(s.size(), i + 3);
    }
    if ((c >> 3) == 0x1e) {
        return std::min(s.size(), i + 4);
    }
    return i + 1;
}

/// Terminal display width: one column per codepoint, ignoring ANSI SGR sequences.
inline int visible_width(std::string_view s) {
    int w = 0;
    for (std::size_t i = 0; i < s.size();) {
        if (s[i] == '\033' && i + 1 < s.size() && s[i + 1] == '[') {
            i += 2;
            while (i < s.size() && s[i] != 'm') {
                ++i;
            }
            if (i < s.size()) {
                ++i;
            }
            continue;
        }
        ++w;
        i = utf8_codepoint_advance(s, i);
    }
    return w;
}

/// Repeats the horizontal box glyph `n` times (may be zero).
inline std::string repeat_box_h(int n) {
    std::string r;
    r.reserve(k_box_h.size() * static_cast<std::size_t>(std::max(0, n)));
    for (int k = 0; k < n; ++k) {
        r += k_box_h;
    }
    return r;
}

/// Repeats ASCII character `c` `n` times (clamped at non-negative length).
inline std::string repeat_char(char c, int n) {
    return std::string(std::max(0, n), c);
}

/// Returns `n` ASCII spaces (non-negative).
inline std::string spaces(int n) {
    return repeat_char(' ', std::max(0, n));
}

/// Right-pads `s` with spaces so its visible width is at least `width`.
inline std::string pad_visible(std::string_view s, int width) {
    return std::string{s} + spaces(std::max(0, width - visible_width(s)));
}

/// Preferred help line width from tty `fd`, or a conservative default when not a tty.
inline int help_width_fd(int fd) {
    winsize w{};
    if (ioctl(fd, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        return std::max(40, static_cast<int>(w.ws_col));
    }
    return 80;
}

/// True when `fd` refers to an interactive terminal (drives color in help).
inline bool tty_fd(int fd) {
    return isatty(fd) != 0;
}

/// Greedy word-wrap of `text` into lines no wider than `width` (measured in columns).
inline std::vector<std::string> wrap_text(std::string_view text, int width) {
    const int available = std::max(1, width);
    std::vector<std::string> out;
    std::string cur;
    std::istringstream iss{std::string{text}};
    std::string word;
    while (iss >> word) {
        if (cur.empty()) {
            cur = word;
            continue;
        }
        if (static_cast<int>(cur.size() + 1 + word.size()) <= available) {
            cur += ' ';
            cur += word;
        } else {
            out.push_back(cur);
            cur = word;
        }
    }
    if (!cur.empty()) {
        out.push_back(std::move(cur));
    }
    if (out.empty()) {
        out.push_back("");
    }
    return out;
}

/// Suffix for usage labels (`<number>`, `<string>`, or empty for presence).
inline std::string opt_kind_label(OptionKind k) {
    switch (k) {
    case OptionKind::Presence:
        return "";
    case OptionKind::Number:
        return " <number>";
    case OptionKind::String:
        return " <string>";
    }
    return "";
}

/// Renders one option or positional for tables and usage (optional ANSI highlights).
inline std::string option_label(const Option& o, bool color) {
    if (o.positional) {
        if (o.arg_max == 1) {
            return (o.arg_min == 0 ? std::string("[") + o.name + "]"
                                   : std::string("<") + o.name + ">");
        }
        return (o.arg_min == 0 ? std::string("[") + o.name + "...]"
                               : std::string("<") + o.name + "...>");
    }
    std::string r = "--" + o.name + opt_kind_label(o.kind);
    if (o.short_name != '\0') {
        r += ", -";
        r += o.short_name;
    }
    if (!color) {
        return r;
    }
    const auto comma = r.find(", ");
    if (comma != std::string::npos) {
        return style::aqua_bold(r.substr(0, comma + 1)) + " " +
               style::green_bright(r.substr(comma + 2));
    }
    return style::aqua_bold(r);
}

/// One row in a bordered help table (label column + description).
struct HelpRow {
    std::string label;
    std::string description;
};

/// Renders a titled paragraph box with UTF-8 corners and optional gray/bold styling.
inline std::vector<std::string>
render_text_box(std::string_view title, const std::vector<std::string>& lines, int hw, bool color) {
    if (lines.empty()) {
        return {};
    }
    std::string title_lead =
        (color ? style::gray(std::string{k_box_h} + " ") +
                     style::bold(style::gray(std::string{title})) + style::gray(" ")
               : std::string{k_box_h} + " " + std::string{title} + " ");
    int content_width = visible_width(title_lead) + 1;
    for (const auto& line : lines) {
        content_width = std::max(content_width, visible_width(line));
    }
    content_width = std::max(hw - 2, content_width);
    content_width = std::min(content_width, hw - 4);
    const int border_width = content_width + 2;
    const int header_fill = std::max(1, border_width - visible_width(title_lead));
    std::vector<std::string> out;
    out.push_back((color ? style::gray(std::string{k_box_tl}) : std::string{k_box_tl}) +
                  title_lead +
                  (color ? style::gray(repeat_box_h(header_fill) + std::string{k_box_tr})
                         : repeat_box_h(header_fill) + std::string{k_box_tr}));
    for (const auto& line : lines) {
        out.push_back((color ? style::gray(std::string{k_box_v}) : std::string{k_box_v}) + " " +
                      pad_visible(line, content_width) + " " +
                      (color ? style::gray(std::string{k_box_v}) : std::string{k_box_v}));
    }
    out.push_back(color
                      ? style::gray(std::string{k_box_bl} + repeat_box_h(border_width) +
                                    std::string{k_box_br})
                      : std::string{k_box_bl} + repeat_box_h(border_width) + std::string{k_box_br});
    return out;
}

/// Renders a two-column table (label + wrapped description) inside a bordered box.
inline std::vector<std::string>
render_table_box(std::string_view title, const std::vector<HelpRow>& rows, int hw, bool color) {
    if (rows.empty()) {
        return {};
    }
    int label_width = 0;
    for (const auto& row : rows) {
        label_width = std::max(label_width, visible_width(row.label));
    }
    const int minimum_content_width = std::max(
        static_cast<int>(visible_width(std::string{k_box_h} + " " + std::string{title} + " ")) + 1,
        label_width + 2 + 18);
    int content_width = std::max(hw - 2, minimum_content_width);
    const int desc_width = std::max(1, content_width - label_width - 2);
    std::vector<std::string> body_lines;
    for (const auto& row : rows) {
        const auto wrapped = wrap_text(row.description, desc_width);
        const std::string first = row.label + spaces(label_width - visible_width(row.label)) +
                                  "  " + (color ? style::white(wrapped.front()) : wrapped.front());
        body_lines.push_back(first);
        for (std::size_t idx = 1; idx < wrapped.size(); ++idx) {
            body_lines.push_back((color ? style::gray(spaces(label_width)) : spaces(label_width)) +
                                 "  " + (color ? style::white(wrapped[idx]) : wrapped[idx]));
        }
    }
    std::string title_lead =
        (color ? style::gray(std::string{k_box_h} + " ") +
                     style::bold(style::gray(std::string{title})) + style::gray(" ")
               : std::string{k_box_h} + " " + std::string{title} + " ");
    content_width = std::max(content_width, visible_width(title_lead) + 1);
    for (const auto& line : body_lines) {
        content_width = std::max(content_width, visible_width(line));
    }
    content_width = std::min(content_width, hw - 4);
    const int border_width = content_width + 2;
    const int header_fill = std::max(1, border_width - visible_width(title_lead));
    std::vector<std::string> out;
    out.push_back((color ? style::gray(std::string{k_box_tl}) : std::string{k_box_tl}) +
                  title_lead +
                  (color ? style::gray(repeat_box_h(header_fill) + std::string{k_box_tr})
                         : repeat_box_h(header_fill) + std::string{k_box_tr}));
    for (const auto& line : body_lines) {
        out.push_back((color ? style::gray(std::string{k_box_v}) : std::string{k_box_v}) + " " +
                      pad_visible(line, content_width) + " " +
                      (color ? style::gray(std::string{k_box_v}) : std::string{k_box_v}));
    }
    out.push_back(color
                      ? style::gray(std::string{k_box_bl} + repeat_box_h(border_width) +
                                    std::string{k_box_br})
                      : std::string{k_box_bl} + repeat_box_h(border_width) + std::string{k_box_br});
    return out;
}

/// Builds one or two usage synopsis lines for root or nested help paths.
inline std::vector<std::string> usage_lines(const std::string& app_name,
                                            const std::vector<std::string>& help_path,
                                            bool has_commands, bool has_args, bool color) {
    std::string full_path = app_name;
    for (const auto& seg : help_path) {
        full_path += " ";
        full_path += seg;
    }
    const std::string usage_opts = color ? style::aqua_bold("[OPTIONS]") : "[OPTIONS]";
    const std::string usage_cmd = color ? style::aqua_bold("COMMAND") : "COMMAND";
    const std::string usage_args = color ? style::aqua_bold("[ARGS]...") : "[ARGS]...";
    std::vector<std::string> out;
    if (help_path.empty()) {
        if (has_commands) {
            out.push_back(full_path + " " + usage_opts + " " + usage_cmd + " " + usage_args);
        } else {
            out.push_back(full_path + " " + usage_opts);
        }
        return out;
    }
    out.push_back(full_path + " " + usage_opts + (has_args ? (" " + usage_args) : ""));
    if (has_commands) {
        out.push_back(full_path + " " + usage_cmd + " " + usage_args);
    }
    return out;
}

/// Table rows for non-positional options at a scope, including built-in `--help` / `-h`.
inline std::vector<HelpRow> rows_for_options(const std::vector<Option>& defs, bool color) {
    std::vector<HelpRow> rows;
    rows.push_back({color ? style::aqua_bold("--help, ") + style::green_bright("-h") : "--help, -h",
                    "Show help for this command."});
    for (const auto& o : defs) {
        if (o.positional) {
            continue;
        }
        rows.push_back({option_label(o, color), o.description});
    }
    return rows;
}

/// Table rows for positional / variadic argument definitions at a scope.
inline std::vector<HelpRow> rows_for_positionals(const std::vector<Option>& defs, bool color) {
    std::vector<HelpRow> rows;
    for (const auto& o : defs) {
        if (o.positional) {
            rows.push_back({option_label(o, color), o.description});
        }
    }
    return rows;
}

/// Sorted subcommand names and descriptions for a routing node.
inline std::vector<HelpRow> rows_for_subcommands(const std::vector<Command>& cmds) {
    auto sorted = cmds;
    std::sort(sorted.begin(), sorted.end(),
              [](const Command& a, const Command& b) { return a.name < b.name; });
    std::vector<HelpRow> rows;
    for (const auto& c : sorted) {
        rows.push_back({c.name, c.description});
    }
    return rows;
}

/// Joins pre-rendered lines with `sep` (typically newline).
inline std::string join_lines(const std::vector<std::string>& lines, const std::string& sep) {
    std::ostringstream oss;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        if (i) {
            oss << sep;
        }
        oss << lines[i];
    }
    return oss.str();
}

/// Full help document for `help_path`, using `tty_fd_num` for width and color decisions.
inline std::string help_render_for_fd(const Schema& schema,
                                      const std::vector<std::string>& help_path, int tty_fd_num) {
    const int hw = help_width_fd(tty_fd_num);
    const bool color = tty_fd(tty_fd_num);

    if (help_path.empty()) {
        std::vector<std::string> lines;
        lines.push_back("");
        if (!schema.description.empty()) {
            lines.push_back(color ? style::white(schema.description) : schema.description);
            lines.push_back("");
        }
        lines.push_back(
            join_lines(render_text_box("Usage",
                                       usage_lines(schema.name, help_path, !schema.commands.empty(),
                                                   false, color),
                                       hw, color),
                       "\n"));
        const auto opt_box =
            render_table_box("Options", rows_for_options(schema.options, color), hw, color);
        if (!opt_box.empty()) {
            lines.push_back("");
            lines.push_back(join_lines(opt_box, "\n"));
        }
        if (!schema.commands.empty()) {
            lines.push_back("");
            lines.push_back(join_lines(
                render_table_box("Commands", rows_for_subcommands(schema.commands), hw, color),
                "\n"));
        }
        return join_lines(lines, "\n") + "\n\n";
    }

    const std::vector<Command>* layer = &schema.commands;
    const Command* node = nullptr;
    for (const auto& seg : help_path) {
        const auto* ch = find_child(*layer, seg);
        if (ch == nullptr) {
            return (color ? style::red("Unknown help path.") : std::string{"Unknown help path."}) +
                   "\n";
        }
        node = ch;
        layer = &ch->children;
    }

    std::vector<std::string> lines;
    lines.push_back("");
    if (!node->description.empty()) {
        lines.push_back(color ? style::white(node->description) : node->description);
        lines.push_back("");
    }
    lines.push_back(
        join_lines(render_text_box("Usage",
                                   usage_lines(schema.name, help_path, !node->children.empty(),
                                               !node->positionals.empty(), color),
                                   hw, color),
                   "\n"));

    const auto opt_box =
        render_table_box("Options", rows_for_options(node->options, color), hw, color);
    if (!opt_box.empty()) {
        lines.push_back("");
        lines.push_back(join_lines(opt_box, "\n"));
    }
    const auto pos_box =
        render_table_box("Arguments", rows_for_positionals(node->positionals, color), hw, color);
    if (!pos_box.empty()) {
        lines.push_back("");
        lines.push_back(join_lines(pos_box, "\n"));
    }
    const auto sub_box =
        render_table_box("Subcommands", rows_for_subcommands(node->children), hw, color);
    if (!sub_box.empty()) {
        lines.push_back("");
        lines.push_back(join_lines(sub_box, "\n"));
    }
    if (!node->notes.empty()) {
        std::string resolved = node->notes;
        for (;;) {
            const auto pos = resolved.find("{app}");
            if (pos == std::string::npos) {
                break;
            }
            resolved.replace(pos, 5, schema.name);
        }
        lines.push_back("");
        lines.push_back(
            join_lines(render_text_box("Notes", wrap_text(resolved, hw - 4), hw, color), "\n"));
    }
    return join_lines(lines, "\n") + "\n\n";
}

} // namespace help_fmt_detail

/// Renders help to a string as if for stdout (width/color follow stdout tty).
inline std::string help_render(const Schema& schema, const std::vector<std::string>& help_path) {
    return help_fmt_detail::help_render_for_fd(schema, help_path, STDOUT_FILENO);
}

/// Renders help using stderr tty width/color (for error footers next to stderr messages).
inline std::string help_render_stderr(const Schema& schema,
                                      const std::vector<std::string>& help_path) {
    return help_fmt_detail::help_render_for_fd(schema, help_path, STDERR_FILENO);
}
} // namespace argsbarg
