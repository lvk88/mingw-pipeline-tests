#include "lib.hpp"

#include <numeric>
#include <span>

int foo(int a, int b)
{
    return a + b;
}

float sum(const std::span<const float>& values)
{
  return std::accumulate(values.begin(), values.end(), 0.0);
}
