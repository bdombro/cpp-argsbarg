#include <argsbarg/argsbarg.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace argsbarg;

TEST_CASE("Context number_opt parses valid number") {
    Schema s{.name = "x", .description = ""};
    s.commands.push_back(Leaf{"l", ""}.handler([](Context&) {}));
    const auto m = merge_builtins(s);
    Context ctx("x", {"l"}, {{"n", "3.5"}}, {}, m);
    REQUIRE(ctx.number_opt("n").has_value());
    REQUIRE(ctx.number_opt("n").value() == Catch::Approx(3.5));
}

TEST_CASE("Context number_opt rejects invalid") {
    Schema s{.name = "x", .description = ""};
    s.commands.push_back(Leaf{"l", ""}.handler([](Context&) {}));
    const auto m = merge_builtins(s);
    Context ctx("x", {"l"}, {{"n", "3.5x"}}, {}, m);
    REQUIRE(!ctx.number_opt("n").has_value());
}
