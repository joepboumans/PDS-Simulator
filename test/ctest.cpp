#include <catch2/catch_test_macros.hpp>
#include <iostream>

int hello_world() {
  std::cout << "Hello Test Wolrd" << std::endl;
  return 0;
}

TEST_CASE("Hello Test World", "[hello-test-world]") {
  REQUIRE(hello_world() == 0);
}
