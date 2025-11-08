#include "lib.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("foo", "")
{
    auto result = foo(40, 2);
    REQUIRE(result == 42);
}