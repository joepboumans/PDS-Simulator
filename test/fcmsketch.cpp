#include "../src/fcmsketch.h"
#include <catch2/catch_test_macros.hpp>
#include <typeinfo>

TEST_CASE("Smoke Test", "[smoke-test]") {
  REQUIRE(true == true);
  REQUIRE(false == false);
}

TEST_CASE("Constructor FCM", "[const-fcm]") {
  FCM_Sketch fcmsketch(3, "test", 0, 0);
  REQUIRE(typeid(fcmsketch) == typeid(FCM_Sketch));
}
