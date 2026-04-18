#include <argsbarg/argsbarg.hpp>
#include <catch2/catch_test_macros.hpp>
#include <string>

using namespace argsbarg;

TEST_CASE("help_render root mentions app name") {
    Schema s{.name = "mytool", .description = "Does things."};
    s.commands.push_back(Leaf{"go", "Run"}.handler([](Context&) {}));
    const auto m = merge_builtins(s);
    schema_validate(m);
    const auto h = help_render(m, {});
    REQUIRE(h.find("mytool") != std::string::npos);
    REQUIRE(h.find("Does things.") != std::string::npos);
}

TEST_CASE("help_render unknown path message") {
    Schema s{.name = "app", .description = ""};
    s.commands.push_back(Leaf{"x", ""}.handler([](Context&) {}));
    const auto m = merge_builtins(s);
    schema_validate(m);
    const auto h = help_render(m, {"nope"});
    REQUIRE(h.find("Unknown") != std::string::npos);
}

TEST_CASE("help_render for routing node path") {
    Schema s{.name = "app", .description = ""};
    s.commands.push_back(Group{"g", "Group help."}.child(Leaf{"x", ""}.handler([](Context&) {})));
    const auto m = merge_builtins(s);
    schema_validate(m);
    const auto h = help_render(m, {"g"});
    REQUIRE(h.find("g") != std::string::npos);
    REQUIRE(h.find("Group help.") != std::string::npos);
}

TEST_CASE("help_render_stderr produces output for leaf") {
    Schema s{.name = "mytool", .description = ""};
    s.commands.push_back(Leaf{"go", "Run it"}
                             .handler([](Context&) {})
                             .option(Opt{"fast", "Speed"}.short_alias('f')));
    const auto m = merge_builtins(s);
    schema_validate(m);
    const auto h = help_render_stderr(m, {"go"});
    REQUIRE(h.find("go") != std::string::npos);
}

TEST_CASE("help_render leaf includes usage") {
    Schema s{.name = "mytool", .description = ""};
    s.commands.push_back(Leaf{"go", "Run it"}
                             .handler([](Context&) {})
                             .option(Opt{"fast", "Speed"}.short_alias('f')));
    const auto m = merge_builtins(s);
    schema_validate(m);
    const auto h = help_render(m, {"go"});
    REQUIRE(h.find("go") != std::string::npos);
    REQUIRE(h.find("--fast") != std::string::npos);
}
