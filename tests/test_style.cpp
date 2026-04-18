#include <argsbarg/style.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace argsbarg;

TEST_CASE("style helpers return non-empty strings") {
    REQUIRE(!style::red("e").empty());
    REQUIRE(!style::green("e").empty());
    REQUIRE(!style::gray("e").empty());
    REQUIRE(!style::bold("e").empty());
    REQUIRE(!style::white("e").empty());
    REQUIRE(!style::aqua_bold("e").empty());
    REQUIRE(!style::green_bright("e").empty());
}
