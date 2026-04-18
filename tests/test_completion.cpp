#include <argsbarg/argsbarg.hpp>
#include <argsbarg/completion.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace argsbarg;

TEST_CASE("bash completion script contains complete directive") {
    Schema s{.name = "demo", .description = ""};
    s.commands.push_back(Leaf{"hi", ""}.handler([](Context&) {}));
    const auto m = merge_builtins(s);
    schema_validate(m);
    const auto script = completion_bash_script(m);
    REQUIRE(script.find("complete -F") != std::string::npos);
    REQUIRE(script.find("demo") != std::string::npos);
}

TEST_CASE("zsh completion script contains compdef") {
    Schema s{.name = "demo", .description = ""};
    s.commands.push_back(Leaf{"hi", ""}.handler([](Context&) {}));
    const auto m = merge_builtins(s);
    schema_validate(m);
    const auto script = completion_zsh_script(m);
    REQUIRE(script.find("#compdef") != std::string::npos);
    REQUIRE(script.find("compdef") != std::string::npos);
}
