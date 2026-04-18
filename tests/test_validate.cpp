#include <argsbarg/argsbarg.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace argsbarg;

TEST_CASE("schema_validate rejects duplicate top-level commands") {
    Schema s{.name = "a",
             .description = "",
             .commands = {Leaf{"x", ""}.handler([](Context&) {}),
                          Leaf{"x", ""}.handler([](Context&) {})}};
    REQUIRE_THROWS_AS(schema_validate(merge_builtins(s)), SchemaError);
}

TEST_CASE("schema_validate rejects leaf without handler via group") {
    Schema s{.name = "a",
             .description = "",
             .commands = {Command{.name = "bad", .description = "", .children = {}}}};
    REQUIRE_THROWS_AS(schema_validate(merge_builtins(s)), SchemaError);
}

TEST_CASE("schema_validate rejects handler on routing node") {
    Schema s{.name = "a",
             .description = "",
             .commands = {Command{.name = "g",
                                  .description = "",
                                  .children = {Leaf{"x", ""}.handler([](Context&) {})},
                                  .handler = [](Context&) {}}}};
    REQUIRE_THROWS_AS(schema_validate(merge_builtins(s)), SchemaError);
}

TEST_CASE("schema_validate rejects reserved completion name") {
    Schema s{.name = "a",
             .description = "",
             .commands = {Leaf{"completion", ""}.handler([](Context&) {})}};
    REQUIRE_THROWS_AS(merge_builtins(s), SchemaError);
}

TEST_CASE("schema_validate rejects fallback mode without command") {
    Schema s{.name = "a", .description = "", .fallback_mode = FallbackMode::MissingOrUnknown};
    REQUIRE_THROWS_AS(schema_validate(merge_builtins(s)), SchemaError);
}
