#include "catch2/catch_approx.hpp"
#include "lib.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("foo", "")
{
    auto result = foo(40, 2);
    REQUIRE(result == 42);
}

TEST_CASE("sum", "")
{
  std::array values{1.0f, 2.0f, 42.0f};
  auto result = sum(std::span{values.begin(), values.size()});

  REQUIRE(result == Catch::Approx(45.0f));
}
