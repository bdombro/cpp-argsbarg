#pragma once

#include "schema.hpp"

#include <string>

namespace argsbarg {

/// Fluent builder for optional flags and value-taking options (maps to @ref Option with
/// `positional == false`).
class Opt {
  public:
    Opt(std::string name, std::string desc);
    Opt& flag(); // default; OptionKind::Presence
    Opt& string();
    Opt& number();
    Opt& short_alias(char c);
    [[nodiscard]] operator Option() const;

  private:
    Option opt_{};
};

/// Fluent builder for positional arguments and tails (maps to @ref Option with
/// `positional == true`).
class Arg {
  public:
    Arg(std::string name, std::string desc);
    Arg& optional();    // arg_min = 0 (default is required: arg_min = 1)
    Arg& min(int n);    // list mode: sets arg_min and arg_max = 0 (unlimited) until @ref max
    Arg& max(int n);    // cap the tail length; 0 = unlimited
    [[nodiscard]] operator Option() const;

  private:
    Option opt_{};
};

/// Fluent builder for leaf commands (has a handler).
class Leaf {
  public:
    Leaf(std::string name, std::string desc);
    Leaf& handler(Handler h);
    Leaf& option(Option opt); // accepts Opt or Arg via `operator Option()`
    Leaf& arg(Option pos);    // always appends to `positionals`
    Leaf& notes(std::string s);
    [[nodiscard]] operator Command() const;

  private:
    Command cmd_{};
};

/// Fluent builder for routing group commands (subcommands, optional group-level options).
class Group {
  public:
    Group(std::string name, std::string desc);
    Group& child(Command c); // accepts Leaf or Group via `operator Command()`
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
