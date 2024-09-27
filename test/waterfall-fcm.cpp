#include "waterfall-fcm.hpp"
#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <sys/types.h>
#include <typeinfo>

TEST_CASE("Smoke Test", "[smoke-test]") {
  REQUIRE(true == true);
  REQUIRE(false == false);
}

TEST_CASE("Constructor WaterfallFCM", "[const]") {
  WaterfallFCM waterfall(3, 3, 8, 1, 1, 1, 8, "test", 0, 0);
  REQUIRE(typeid(waterfall) == typeid(WaterfallFCM));
}
TEST_CASE("Analysis test", "[small]") {
  WaterfallFCM waterfall(4, 3, 8, 1, 1, 3, 8, "test", 0, 0);
  REQUIRE(waterfall.average_absolute_error == 0);
  REQUIRE(waterfall.average_relative_error == 0);
  FIVE_TUPLE tuple;
  tuple++;
  waterfall.insert(tuple);
  waterfall.insert(tuple);
  waterfall.analyze(0);
  REQUIRE(waterfall.average_absolute_error == 0);
  REQUIRE(waterfall.average_relative_error == 0);
  REQUIRE(waterfall.wmre == 0.0);
  REQUIRE(waterfall.f1_hh == 1.0);
  REQUIRE(waterfall.f1_member == 1.0);
}

TEST_CASE("Analysis - Flow Size", "[flow-size]") {
  // Optimized layout for test, due to hash mappings
  WaterfallFCM waterfall(100, 3, 16, 1024 * 1024 * 10 + 5000 * 3, 1, 3, 8,
                         "test", 0, 0);
  waterfall.set_estimate_fsd(false);
  REQUIRE(waterfall.average_absolute_error == 0);
  REQUIRE(waterfall.average_relative_error == 0);
  FIVE_TUPLE tuple;
  for (size_t i = 0; i < 1024 * 1024; i++) {
    waterfall.insert(tuple);
    waterfall.insert(tuple + 1);
    waterfall.insert(tuple + 2);
    waterfall.insert(tuple + 3);
    waterfall.insert(tuple + 4);
    waterfall.insert(tuple + 5);
    waterfall.insert(tuple + 6);
    waterfall.insert(tuple + 7);
    waterfall.insert(tuple + 8);
    waterfall.insert(tuple + 9);
  }
  FIVE_TUPLE tuple3;
  for (size_t i = 0; i < 5000; i++) {
    waterfall.insert(tuple3);
    tuple3++;
  }
  FIVE_TUPLE tuple4;
  for (size_t i = 0; i < 5000; i++) {
    waterfall.insert(tuple4);
    tuple4++;
  }
  FIVE_TUPLE tuple5;
  for (size_t i = 0; i < 5000; i++) {
    waterfall.insert(tuple5);
    tuple5++;
  }
  waterfall.analyze(0);
  REQUIRE(waterfall.average_absolute_error <= 1);
  REQUIRE(waterfall.average_relative_error <= 1);
}

TEST_CASE("Analysis - F1", "[F1]") {
  WaterfallFCM waterfall(32, 3, 8, 512, 1, 3, 8, "test", 0, 0);
  waterfall.set_estimate_fsd(false);
  FIVE_TUPLE tuple;
  // For CHT tuple cannot be 0.0.0.0|00:0.0.0.0|00 0
  tuple++;

  SECTION("Single HH") {
    for (size_t i = 0; i < 1024; i++) {
      waterfall.insert(tuple);
    }
    waterfall.analyze(0);
    REQUIRE(waterfall.f1_hh == 1.0);
    REQUIRE(waterfall.f1_member == 1.0);
  }

  SECTION("Five HH") {
    for (size_t i = 0; i < 1024; i++) {
      waterfall.insert(tuple);
      waterfall.insert(tuple + 1);
      waterfall.insert(tuple + 2);
      waterfall.insert(tuple + 3);
      waterfall.insert(tuple + 4);
    }
    waterfall.analyze(0);
    REQUIRE(waterfall.f1_hh <= 1.0);
    REQUIRE(waterfall.f1_member == 1.0);
  }

  SECTION("Five HH - 500 Small Flows") {
    for (size_t i = 0; i < 1024; i++) {
      waterfall.insert(tuple);
      waterfall.insert(tuple + 1);
      waterfall.insert(tuple + 2);
      waterfall.insert(tuple + 3);
      waterfall.insert(tuple + 4);
    }
    tuple.num_array[12]++;
    for (size_t i = 0; i < 500; i++) {
      waterfall.insert(tuple);
      tuple++;
    }
    waterfall.analyze(0);
    REQUIRE(waterfall.f1_hh <= 1.0);
    REQUIRE(waterfall.f1_member == 1.0);
  }

  SECTION("100 HH - 10k Small Flows") {
    for (size_t i = 0; i < 1024; i++) {
      for (size_t j = 0; j < 100; j++) {
        waterfall.insert(tuple + j);
      }
    }
    tuple.num_array[12]++;
    for (size_t i = 0; i < 10000; i++) {
      waterfall.insert(tuple);
      tuple++;
    }
    waterfall.analyze(0);
    REQUIRE(waterfall.f1_hh <= 1.0);
    REQUIRE(waterfall.f1_member == 1.0);
  }
}
TEST_CASE("FSD Test", "[small][fsd]") {
  uint32_t roots = 1;
  uint32_t stages = 5;
  uint32_t k = 2;
  uint32_t hh_threshold = 1;
  uint32_t em_iters = 1;
  WaterfallFCM waterfall(roots, stages, k, hh_threshold, em_iters, 3, 8, "test",
                         0, 0);
  REQUIRE(waterfall.average_absolute_error == 0);
  REQUIRE(waterfall.average_relative_error == 0);
  FIVE_TUPLE t, t2, t3, t4;
  t += 1;
  t2 += 2;
  t3.num_array[12] = 10;
  for (size_t i = 0; i < 3000; i++) {
    waterfall.insert(t);
    waterfall.insert(t2);
    waterfall.insert(t3);
  }
  waterfall.print_sketch();
  waterfall.analyze(0);
}

TEST_CASE("FSD Test", "[fsd]") {
  uint32_t roots = 1;
  uint32_t stages = 5;
  uint32_t k = 2;
  uint32_t hh_threshold = 1;
  uint32_t em_iters = 2;
  WaterfallFCM waterfall(roots, stages, k, hh_threshold, em_iters, 3, 8, "test",
                         0, 0);
  REQUIRE(waterfall.average_absolute_error == 0);
  REQUIRE(waterfall.average_relative_error == 0);
  FIVE_TUPLE t, t2, t3, t4;
  t += 1;
  t4 += 1;
  t3.num_array[12] = 10;
  t2.num_array[11] = 10;
  t2.num_array[2] = 10;
  // for (size_t i = 0; i < 65535 + 255; i++) {
  //   waterfall.insert(t);
  //   waterfall.insert(t2);
  // }
  for (size_t i = 0; i < 10; i++) {
    waterfall.insert(t4++);
  }
  waterfall.print_sketch();
  waterfall.analyze(0);
}
