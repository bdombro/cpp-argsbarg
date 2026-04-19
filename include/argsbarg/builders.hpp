#pragma once

/// Fluent builders (`Opt`, `Arg`, `Leaf`, `Group`) that produce `Option` / `Command` values.
///
/// Goal: mirror a pleasant authoring style while staying macro-free.
/// Why: keeps schema construction readable and chains type-safe (`operator Command()`).
/// How: small wrapper types mutate internal `Option` or `Command`, then convert implicitly.

#include "schema.hpp"

#include <string>

namespace argsbarg {

/// Builder for optional flags and value-taking options (maps to `Option` with `positional ==
/// false`).
class Opt {
  public:
    Opt(std::string name, std::string desc);

    /// Presence flag (default).
    Opt& flag();

    /// String-valued `--name value` option.
    Opt& string();

    /// Numeric `--count 3` option (validated later).
    Opt& number();

    /// Single-letter short form (e.g. `-v`).
    Opt& short_alias(char c);

    [[nodiscard]] operator Option() const;

  private:
    Option opt_{};
};

/// Builder for positional arguments and variadic tails (`positional == true`).
class Arg {
  public:
    Arg(std::string name, std::string desc);

    /// Makes the positional optional (`arg_min = 0`).
    Arg& optional();

    /// List mode: sets minimum words; use `max` to cap (0 = unlimited until `max`).
    Arg& min(int n);

    /// Caps tail length when combined with `min`; `0` means unlimited.
    Arg& max(int n);

    [[nodiscard]] operator Option() const;

  private:
    Option opt_{};
};

/// Builder for executable leaf commands (must end with `.handler(...)`).
class Leaf {
  public:
    Leaf(std::string name, std::string desc);

    /// Sets the code invoked when this leaf is selected.
    Leaf& handler(Handler h);

    /// Adds a flag/value option or positional (`Arg` vs `Opt` via `operator Option()`).
    Leaf& option(Option opt);

    /// Appends a positional definition (forces `positional` true).
    Leaf& arg(Option pos);

    /// Extra help text for this command (supports `{app}` substitution in help).
    Leaf& notes(std::string s);

    [[nodiscard]] operator Command() const;

  private:
    Command cmd_{};
};

/// Builder for routing nodes that hold subcommands and optional group-level options.
class Group {
  public:
    Group(std::string name, std::string desc);

    /// Adds a nested `Leaf` or `Group`.
    Group& child(Command c);

    Group& option(Option opt);

    Group& notes(std::string s);

    [[nodiscard]] operator Command() const;

  private:
    Command cmd_{};
};

inline Opt::Opt(std::string name, std::string desc) {
    opt_.name = std::move(name);
    opt_.description = std::move(desc);
}

inline Opt& Opt::flag() {
    opt_.kind = OptionKind::Presence;
    return *this;
}

inline Opt& Opt::string() {
    opt_.kind = OptionKind::String;
    return *this;
}

inline Opt& Opt::number() {
    opt_.kind = OptionKind::Number;
    return *this;
}

inline Opt& Opt::short_alias(char c) {
    opt_.short_name = c;
    return *this;
}

inline Opt::operator Option() const {
    return opt_;
}

inline Arg::Arg(std::string name, std::string desc) {
    opt_.name = std::move(name);
    opt_.description = std::move(desc);
    opt_.kind = OptionKind::String;
    opt_.positional = true;
    opt_.arg_min = 1;
    opt_.arg_max = 1;
}

inline Arg& Arg::optional() {
    opt_.arg_min = 0;
    opt_.arg_max = 1;
    return *this;
}

inline Arg& Arg::min(int n) {
    opt_.arg_min = n;
    opt_.arg_max = 0;
    return *this;
}

inline Arg& Arg::max(int n) {
    opt_.arg_max = n;
    return *this;
}

inline Arg::operator Option() const {
    return opt_;
}

inline Leaf::Leaf(std::string name, std::string desc) {
    cmd_.name = std::move(name);
    cmd_.description = std::move(desc);
}

inline Leaf& Leaf::handler(Handler h) {
    cmd_.handler = std::move(h);
    return *this;
}

inline Leaf& Leaf::option(Option opt) {
    if (opt.positional) {
        cmd_.positionals.push_back(std::move(opt));
    } else {
        cmd_.options.push_back(std::move(opt));
    }
    return *this;
}

inline Leaf& Leaf::arg(Option pos) {
    pos.positional = true;
    cmd_.positionals.push_back(std::move(pos));
    return *this;
}

inline Leaf& Leaf::notes(std::string s) {
    cmd_.notes = std::move(s);
    return *this;
}

inline Leaf::operator Command() const {
    return cmd_;
}

inline Group::Group(std::string name, std::string desc) {
    cmd_.name = std::move(name);
    cmd_.description = std::move(desc);
}

inline Group& Group::child(Command c) {
    cmd_.children.push_back(std::move(c));
    return *this;
}

inline Group& Group::option(Option opt) {
    if (opt.positional) {
        cmd_.positionals.push_back(std::move(opt));
    } else {
        cmd_.options.push_back(std::move(opt));
    }
    return *this;
}

inline Group& Group::notes(std::string s) {
    cmd_.notes = std::move(s);
    return *this;
}

inline Group::operator Command() const {
    return cmd_;
}

} // namespace argsbarg
