#include "common.h"
#include "fcm-sketches.hpp"
#include <catch2/catch_test_macros.hpp>
#include <typeinfo>

TEST_CASE("Smoke Test", "[smoke-test]") {
  REQUIRE(true == true);
  REQUIRE(false == false);
}

TEST_CASE("Analysis test", "[analysis]") {
  FCM_Sketches fcm(4, NUM_STAGES, K, DEPTH, 1, 1, "test", TupleSize::FiveTuple);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);
  TUPLE tuple;
  TUPLE tuple2;
  tuple2++;
  for (size_t i = 0; i < 256; i++) {
    fcm.insert(tuple);
  }
  for (size_t i = 0; i < 256; i++) {
    fcm.insert(tuple2);
  }
  fcm.print_sketch();
  fcm.analyze(0);
  fcm.estimator_org = false;
  fcm.analyze(0);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);
  REQUIRE(fcm.precision == 1.0);
  REQUIRE(fcm.recall == 1.0);
  REQUIRE(fcm.f1 == 1.0);
}
