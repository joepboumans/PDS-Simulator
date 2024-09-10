#include "lib/fcm-sketch/fcm-sketch.hpp"
#include <catch2/catch_test_macros.hpp>
#include <typeinfo>

TEST_CASE("Smoke Test", "[smoke-test]") {
  REQUIRE(true == true);
  REQUIRE(false == false);
}

TEST_CASE("Constructor FCM", "[const-fcm]") {
  FCM_Sketch fcmsketch(3, 3, 8, 1, "test", 0, 0);
  REQUIRE(typeid(fcmsketch) == typeid(FCM_Sketch));
}
TEST_CASE("Analysis test", "[CM][analysis]") {
  FCM_Sketch fcm(4, 3, 8, 1, "test", 0, 0);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);
  FIVE_TUPLE tuple;
  fcm.insert(tuple);
  fcm.insert(tuple);
  fcm.analyze(0);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);
  REQUIRE(fcm.precision == 1.0);
  REQUIRE(fcm.recall == 1.0);
  REQUIRE(fcm.f1 == 1.0);
}

TEST_CASE("Analysis - Flow Size", "[CM][analysis][flow-size]") {
  // Optimized layout for test, due to hash mappings
  FCM_Sketch fcm(100, 3, 16, 1024 * 1024 * 10 + 5000 * 3, "test", 0, 0);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);
  FIVE_TUPLE tuple;
  for (size_t i = 0; i < 1024 * 1024; i++) {
    fcm.insert(tuple);
    fcm.insert(tuple + 1);
    fcm.insert(tuple + 2);
    fcm.insert(tuple + 3);
    fcm.insert(tuple + 4);
    fcm.insert(tuple + 5);
    fcm.insert(tuple + 6);
    fcm.insert(tuple + 7);
    fcm.insert(tuple + 8);
    fcm.insert(tuple + 9);
  }
  FIVE_TUPLE tuple3;
  for (size_t i = 0; i < 5000; i++) {
    fcm.insert(tuple3);
    tuple3++;
  }
  FIVE_TUPLE tuple4;
  for (size_t i = 0; i < 5000; i++) {
    fcm.insert(tuple4);
    tuple4++;
  }
  FIVE_TUPLE tuple5;
  for (size_t i = 0; i < 5000; i++) {
    fcm.insert(tuple5);
    tuple5++;
  }
  fcm.analyze(0);
  REQUIRE(fcm.average_absolute_error <= 1);
  REQUIRE(fcm.average_relative_error <= 1);
}

TEST_CASE("Analysis - F1", "[CM][analysis][F1]") {
  FCM_Sketch fcm(32, 3, 8, 512, "test", 0, 0);
  FIVE_TUPLE tuple;

  SECTION("Single HH") {
    for (size_t i = 0; i < 1024; i++) {
      fcm.insert(tuple);
    }
    fcm.analyze(0);
    REQUIRE(fcm.f1 == 1.0);
    REQUIRE(fcm.precision == 1.0);
    REQUIRE(fcm.recall == 1.0);
  }

  SECTION("Five HH") {
    for (size_t i = 0; i < 1024; i++) {
      fcm.insert(tuple);
      fcm.insert(tuple + 1);
      fcm.insert(tuple + 2);
      fcm.insert(tuple + 3);
      fcm.insert(tuple + 4);
    }
    fcm.analyze(0);
    REQUIRE(fcm.f1 == 1.0);
    REQUIRE(fcm.precision == 1.0);
    REQUIRE(fcm.recall == 1.0);
  }

  SECTION("Five HH - 500 Small Flows") {
    for (size_t i = 0; i < 1024; i++) {
      fcm.insert(tuple);
      fcm.insert(tuple + 1);
      fcm.insert(tuple + 2);
      fcm.insert(tuple + 3);
      fcm.insert(tuple + 4);
    }
    tuple.num_array[12]++;
    for (size_t i = 0; i < 500; i++) {
      fcm.insert(tuple);
      tuple++;
    }
    fcm.analyze(0);
    REQUIRE(fcm.f1 < 1.0);
    REQUIRE(fcm.precision <= 1.0);
    REQUIRE(fcm.recall <= 1.0);
  }

  SECTION("100 HH - 10k Small Flows") {
    for (size_t i = 0; i < 1024; i++) {
      for (size_t j = 0; j < 100; j++) {
        fcm.insert(tuple + j);
      }
    }
    tuple.num_array[12]++;
    for (size_t i = 0; i < 10000; i++) {
      fcm.insert(tuple);
      tuple++;
    }
    fcm.analyze(0);
    REQUIRE(fcm.f1 < 1.0);
    REQUIRE(fcm.precision <= 1.0);
    REQUIRE(fcm.recall <= 1.0);
  }
}

TEST_CASE("FSD Test", "[CM][analysis][fsd]") {
  FCM_Sketch fcm(1, 5, 2, 1, "test", 0, 0);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);
  FIVE_TUPLE t, t2, t3, t4;
  t3.num_array[12] = 10;
  t2.num_array[11] = 10;
  t2.num_array[2] = 10;
  for (size_t i = 0; i < 65535 + 255; i++) {
    fcm.insert(t);
    fcm.insert(t2);
    // fcm.insert(t3);
  }
  for (size_t i = 0; i < 100; i++) {
    fcm.insert(t4++);
  }
  fcm.print_sketch();
  fcm.analyze(0);
}
