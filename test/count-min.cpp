#include "../src/count-min/count-min.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Smoke Test", "[smoke-test]") {
  REQUIRE(true == true);
  REQUIRE(false == false);
}

TEST_CASE("Constructor FCM", "[const-CM]") {
  CountMin count_min(4, 4, "test", 0, 0);
  REQUIRE(typeid(count_min) == typeid(CountMin));
}
