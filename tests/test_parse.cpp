#include <argsbarg/argsbarg.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace argsbarg;

static Schema tiny() {
    Schema s{.name = "app", .description = "d"};
    s.commands.push_back(
        Leaf{"hello", "h"}.handler([](Context&) {}).option(Opt{"verbose", ""}.short_alias('v')));
    return merge_builtins(s);
}

TEST_CASE("parse explicit help at nested routing depth") {
    Schema s{.name = "app", .description = "d"};
    s.commands.push_back(Group{"g", "grp"}.child(Leaf{"x", ""}.handler([](Context&) {})));
    const auto m = merge_builtins(s);
    schema_validate(m);
    auto pr = parse(m, {"g", "-h"});
    REQUIRE(pr.kind == ParseKind::Help);
    REQUIRE(pr.help_explicit);
    REQUIRE(pr.help_path == std::vector<std::string>{"g"});
}

TEST_CASE("parse explicit help at root") {
    const auto s = tiny();
    schema_validate(s);
    auto pr = parse(s, {"-h"});
    REQUIRE(pr.kind == ParseKind::Help);
    REQUIRE(pr.help_explicit);
}

TEST_CASE("parse short option bundle for presence flags") {
    Schema s{.name = "app", .description = "d"};
    s.commands.push_back(Leaf{"x", ""}
                             .handler([](Context&) {})
                             .option(Opt{"a", ""}.short_alias('a'))
                             .option(Opt{"b", ""}.short_alias('b')));
    const auto m = merge_builtins(s);
    schema_validate(m);
    auto pr = post_parse_validate(m, parse(m, {"x", "-ab"}));
    REQUIRE(pr.kind == ParseKind::Ok);
    REQUIRE(pr.opts.contains("a"));
    REQUIRE(pr.opts.contains("b"));
}

TEST_CASE("parse long option equals form") {
    Schema s{.name = "app", .description = "d"};
    s.commands.push_back(
        Leaf{"x", ""}.handler([](Context&) {}).option(Opt{"name", ""}.string().short_alias('n')));
    const auto m = merge_builtins(s);
    schema_validate(m);
    auto pr = post_parse_validate(m, parse(m, {"x", "--name=pat"}));
    REQUIRE(pr.kind == ParseKind::Ok);
    REQUIRE(pr.opts.at("name") == "pat");
}

TEST_CASE("parse routes to known command") {
    const auto s = tiny();
    schema_validate(s);
    auto pr = post_parse_validate(s, parse(s, {"hello"}));
    REQUIRE(pr.kind == ParseKind::Ok);
    REQUIRE(pr.path == std::vector<std::string>{"hello"});
}

TEST_CASE("parse MissingOrUnknown routes unknown first token to default") {
    Schema s{.name = "app",
             .description = "d",
             .fallback_command = "hello",
             .fallback_mode = FallbackMode::MissingOrUnknown};
    s.commands.push_back(Leaf{"hello", "h"}
                             .handler([](Context&) {})
                             .option(Opt{"name", ""}.string().short_alias('n')));
    const auto m = merge_builtins(s);
    schema_validate(m);
    auto pr = post_parse_validate(m, parse(m, {"--name", "bob"}));
    REQUIRE(pr.kind == ParseKind::Ok);
    REQUIRE(pr.path.front() == "hello");
    REQUIRE(pr.opts.at("name") == "bob");
}

TEST_CASE("parse MissingOnly errors on unknown command") {
    Schema s{.name = "app",
             .description = "d",
             .fallback_command = "hello",
             .fallback_mode = FallbackMode::MissingOnly};
    s.commands.push_back(Leaf{"hello", "h"}.handler([](Context&) {}));
    const auto m = merge_builtins(s);
    schema_validate(m);
    auto pr = parse(m, {"nope"});
    REQUIRE(pr.kind == ParseKind::Error);
}

TEST_CASE("parse UnknownOnly empty argv yields implicit help") {
    Schema s{.name = "app",
             .description = "d",
             .fallback_command = "hello",
             .fallback_mode = FallbackMode::UnknownOnly};
    s.commands.push_back(Leaf{"hello", "h"}.handler([](Context&) {}));
    const auto m = merge_builtins(s);
    schema_validate(m);
    auto pr = parse(m, {});
    REQUIRE(pr.kind == ParseKind::Help);
    REQUIRE(!pr.help_explicit);
}

TEST_CASE("parse rejects unknown subcommand on routing node") {
    Schema s{.name = "app", .description = "d"};
    s.commands.push_back(Group{"g", "grp"}.child(Leaf{"x", ""}.handler([](Context&) {})));
    const auto m = merge_builtins(s);
    schema_validate(m);
    auto pr = parse(m, {"g", "nope"});
    REQUIRE(pr.kind == ParseKind::Error);
}

TEST_CASE("parse rejects option after positional token") {
    Schema s{.name = "app", .description = "d"};
    s.commands.push_back(Leaf{"x", ""}.handler([](Context&) {}).arg(Arg{"file", ""}));
    const auto m = merge_builtins(s);
    schema_validate(m);
    auto pr = parse(m, {"x", "a", "--not-an-option"});
    REQUIRE(pr.kind == ParseKind::Error);
}

TEST_CASE("parse implicit help when routing node has no subcommand") {
    Schema s{.name = "app", .description = "d"};
    s.commands.push_back(Group{"g", "grp"}.child(Leaf{"x", ""}.handler([](Context&) {})));
    const auto m = merge_builtins(s);
    schema_validate(m);
    auto pr = parse(m, {"g"});
    REQUIRE(pr.kind == ParseKind::Help);
    REQUIRE(!pr.help_explicit);
}

TEST_CASE("parse errors on missing value for short string option") {
    Schema s{.name = "app", .description = "d"};
    s.commands.push_back(
        Leaf{"x", ""}.handler([](Context&) {}).option(Opt{"name", ""}.string().short_alias('n')));
    const auto m = merge_builtins(s);
    schema_validate(m);
    auto pr = parse(m, {"x", "-n"});
    REQUIRE(pr.kind == ParseKind::Error);
}

TEST_CASE("parse errors when arg_list below minimum") {
    Schema s{.name = "app", .description = "d"};
    s.commands.push_back(Leaf{"x", ""}.handler([](Context&) {}).arg(Arg{"files", ""}.min(2)));
    const auto m = merge_builtins(s);
    schema_validate(m);
    auto pr = parse(m, {"x", "only"});
    REQUIRE(pr.kind == ParseKind::Error);
}

TEST_CASE("parse errors on extra arguments at leaf") {
    Schema s{.name = "app", .description = "d"};
    s.commands.push_back(Leaf{"x", ""}.handler([](Context&) {}).arg(Arg{"one", ""}));
    const auto m = merge_builtins(s);
    schema_validate(m);
    auto pr = parse(m, {"x", "a", "b"});
    REQUIRE(pr.kind == ParseKind::Error);
}

TEST_CASE("post_parse_validate rejects invalid number") {
    Schema s{.name = "app", .description = "d"};
    s.commands.push_back(Leaf{"x", ""}.handler([](Context&) {}).option(Opt{"n", ""}.number()));
    const auto m = merge_builtins(s);
    schema_validate(m);
    auto pr = parse(m, {"x", "--n", "12abc"});
    pr = post_parse_validate(m, pr);
    REQUIRE(pr.kind == ParseKind::Error);
}
