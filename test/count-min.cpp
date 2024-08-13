#include "../lib/count-min/count-min.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Smoke Test", "[smoke-test]") {
  REQUIRE(true == true);
  REQUIRE(false == false);
  std::cout << "Smoke done!" << std::endl;
}

TEST_CASE("Constructor CountMin Sketch", "[const-CM]") {
  CountMin count_min(4, 4, "test", 0, 0);
  REQUIRE(typeid(count_min) == typeid(CountMin));
}

TEST_CASE("Analysis test", "[Analysis]") {
  CountMin count_min(4, 4, "test", 0, 0);
  REQUIRE(count_min.average_absolute_error == 0);
  REQUIRE(count_min.average_relative_error == 0);
  FIVE_TUPLE tuple;
  count_min.insert(tuple);
  count_min.analyze(0);
  REQUIRE(count_min.average_absolute_error == 0);
  REQUIRE(count_min.average_relative_error == 0);
}

TEST_CASE("Mixed insert - analysis", "[Mixed-Analysis]") {
  CountMin count_min(32, 1024, "test", 0, 0);
  REQUIRE(count_min.average_absolute_error == 0);
  REQUIRE(count_min.average_relative_error == 0);
  FIVE_TUPLE tuple;
  for (size_t i = 0; i < 1024 * 1024; i++) {
    count_min.insert(tuple);
    count_min.insert(tuple + 1);
    count_min.insert(tuple + 2);
    count_min.insert(tuple + 3);
    count_min.insert(tuple + 4);
    count_min.insert(tuple + 5);
    count_min.insert(tuple + 6);
    count_min.insert(tuple + 7);
    count_min.insert(tuple + 8);
    count_min.insert(tuple + 9);
  }
  FIVE_TUPLE tuple3;
  for (size_t i = 0; i < 5000; i++) {
    count_min.insert(tuple3);
    tuple3++;
  }
  FIVE_TUPLE tuple4;
  for (size_t i = 0; i < 5000; i++) {
    count_min.insert(tuple4);
    tuple4++;
  }
  FIVE_TUPLE tuple5;
  for (size_t i = 0; i < 5000; i++) {
    count_min.insert(tuple5);
    tuple5++;
  }
  count_min.analyze(0);
  // count_min.print_sketch();
  REQUIRE(count_min.average_absolute_error <= 1000);
  REQUIRE(count_min.average_relative_error <= 2);
}
