#include "fcm-sketch.hpp"
#include <catch2/catch_test_macros.hpp>
#include <typeinfo>

TEST_CASE("Smoke Test", "[smoke-test]") {
  REQUIRE(true == true);
  REQUIRE(false == false);
}

TEST_CASE("Constructor FCM", "[const-fcm]") {
  FCM_Sketch fcmsketch(3, 3, 8, 1, 1, "test", 0, 0);
  REQUIRE(typeid(fcmsketch) == typeid(FCM_Sketch));
}
TEST_CASE("Analysis test", "[analysis]") {
  FCM_Sketch fcm(4, 3, 8, 1, 1, "test", 0, 0);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);
  FIVE_TUPLE tuple;
  fcm.insert(tuple);
  fcm.insert(tuple);
  fcm.print_sketch();
  fcm.analyze(0);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);
  REQUIRE(fcm.precision == 1.0);
  REQUIRE(fcm.recall == 1.0);
  REQUIRE(fcm.f1 == 1.0);
}

TEST_CASE("Analysis - Flow Size", "[analysis][flow-size]") {
  // Optimized layout for test, due to hash mappings
  FCM_Sketch fcm(100, 3, 16, 1024 * 1024 * 10 + 5000 * 3, 1, "test", 0, 0);
  fcm.estimate_fsd = false;
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

TEST_CASE("Analysis - F1", "[analysis][F1]") {
  FCM_Sketch fcm(32, 3, 8, 512, 1, "test", 0, 0);
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
TEST_CASE("FSD Degree tests", "[small][fsd]") {
  uint32_t roots = 1;
  uint32_t stages = 5;
  uint32_t k = 2;
  uint32_t hh_threshold = 1;
  uint32_t em_iters = 1;
  FCM_Sketch fcm(roots, stages, k, hh_threshold, em_iters, "test", 0, 0);

  SECTION("Single degree 0") {
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "-----\tSingle degree 0\t------" << std::endl;
    std::cout << std::string(50, '-') << std::endl;

    FIVE_TUPLE t;

    for (size_t i = 0; i < 3000; i++) {
      fcm.insert(t, 0);
    }
    fcm.analyze(0);
    fcm.print_sketch();
    REQUIRE(fcm.wmre == 0.0);
  }
  SECTION("Two Degree 0") {
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "------\tTwo degree 0\t------" << std::endl;
    std::cout << std::string(50, '-') << std::endl;

    FIVE_TUPLE t, t2;
    t2 += 2;

    for (size_t i = 0; i < 3000; i++) {
      fcm.insert(t, 0);
      fcm.insert(t2, 8);
    }
    fcm.analyze(0);
    fcm.print_sketch();
    REQUIRE(fcm.wmre == 0.0);
  }
  SECTION("Four Degree 0") {
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "------\tFour degree 0\t------" << std::endl;
    std::cout << std::string(50, '-') << std::endl;

    FIVE_TUPLE t, t2, t3, t4;
    t2 += 2;
    t3 += 3;
    t4 += 4;

    for (size_t i = 0; i < 2; i++) {
      fcm.insert(t, 0);
      fcm.insert(t2, 4);
      fcm.insert(t3, 8);
      fcm.insert(t4, 12);
    }
    for (size_t i = 0; i < 14; i++) {
      fcm.insert(t, 0);
      fcm.insert(t2, 4);
      fcm.insert(t3, 8);
    }
    for (size_t i = 0; i < 254; i++) {
      fcm.insert(t, 0);
      fcm.insert(t2, 4);
    }
    for (size_t i = 0; i < 65534 + 2; i++) {
      fcm.insert(t, 0);
    }
    fcm.analyze(0);
    fcm.print_sketch();
    REQUIRE(fcm.wmre == 0.0);
  }
  SECTION("Small Degree 1") {
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "------\tSmall degree 1\t------" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    fcm.em_iters = 2;

    FIVE_TUPLE t, t2, t3, t4;
    t2 += 2;
    t3 += 3;
    t4 += 4;

    // We need unequal sizes as this is expected by the EM algorithm
    for (size_t i = 0; i < 4; i++) {
      fcm.insert(t, 0);
      fcm.insert(t2, 1);
      fcm.insert(t3, 8);
      fcm.insert(t4, 9);
    }
    fcm.insert(t4, 9);
    fcm.analyze(0);
    fcm.print_sketch();
    REQUIRE(fcm.wmre < 2.0);
  }

  SECTION("Larger Degree 1") {
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "------\tLarger degree 1\t------" << std::endl;
    std::cout << std::string(50, '-') << std::endl;

    FIVE_TUPLE t, t2, t3, t4, t5, t6;
    t2 += 2;
    t3 += 3;
    t4 += 4;
    t5 += 5;
    t6 += 6;

    for (size_t i = 0; i < 4; i++) {
      fcm.insert(t, 0);
      fcm.insert(t2, 15);
      fcm.insert(t3, 12);
      fcm.insert(t4, 13);
      fcm.insert(t5, 2);
      fcm.insert(t6, 4);
    }
    fcm.insert(t4, 13);
    for (size_t i = 0; i < 15 + 255 + 65535; i++) {
      fcm.insert(t, 0);
      fcm.insert(t2, 15);
    }
    fcm.analyze(0);
    fcm.print_sketch();
    REQUIRE(fcm.wmre < 2.0);
  }

  SECTION("Small Degree 2") {
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "------\tSmall degree 2\t------" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    fcm.em_iters = 2;

    FIVE_TUPLE t, t2, t3, t4;
    t2 += 2;
    t3 += 3;
    t4 += 4;

    for (size_t i = 0; i < 3 + 15 + 255; i++) {
      fcm.insert(t, 0);
      fcm.insert(t2, 1);
      fcm.insert(t3, 2);
      fcm.insert(t4, 15);
    }

    fcm.analyze(0);
    fcm.print_sketch();
    REQUIRE(fcm.wmre < 2.0);
  }

  SECTION("Large Degree 2") {
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "------\tLarge degree 2\t------" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    fcm.em_iters = 2;

    FIVE_TUPLE t, t2, t3, t4;
    t2 += 2;
    t3 += 3;
    t4 += 4;

    for (size_t i = 0; i < 2 + 15 + 255; i++) {
      fcm.insert(t, 0);
      fcm.insert(t2, 4);
      fcm.insert(t3, 8);
      fcm.insert(t4, 15);
    }
    for (size_t i = 0; i < 2 + 15 + 255 + 65535; i++) {
      fcm.insert(t, 0);
      fcm.insert(t2, 4);
      fcm.insert(t3, 8);
    }
    fcm.analyze(0);
    fcm.print_sketch();
    REQUIRE(fcm.wmre < 2.0);
  }

  SECTION("Degree 2+") {
    std::cout << std::string(50, '-') << std::endl;
    std::cout << "------\tDegree 2+\t------" << std::endl;
    std::cout << std::string(50, '-') << std::endl;
    fcm.em_iters = 2;

    FIVE_TUPLE t, t2, t3, t4, t5, t6, t7, t8;
    t2 += 2;
    t3 += 3;
    t4 += 4;
    t5 += 5;
    t6 += 6;
    t7 += 7;
    t8 += 8;

    for (size_t i = 0; i < 2 + 15 + 255; i++) {
      fcm.insert(t, 0);
      fcm.insert(t2, 1);
      fcm.insert(t3, 2);
      fcm.insert(t4, 4);
      fcm.insert(t5, 6);
      fcm.insert(t6, 8);
      fcm.insert(t7, 10);
    }
    for (size_t i = 0; i < 2 + 15; i++) {
      fcm.insert(t8, 15);
    }
    fcm.analyze(0);
    fcm.print_sketch();
    REQUIRE(fcm.wmre < 2.0);
  }
}

TEST_CASE("FSD Test", "[analysis][fsd]") {
  FCM_Sketch fcm(1, 5, 2, 1, 1, "test", 0, 0);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);
  FIVE_TUPLE t, t2, t3, t4;
  t3.num_array[12] = 10;
  t2.num_array[11] = 10;
  t2.num_array[2] = 10;
  for (size_t i = 0; i < 65535 + 255 + 15 + 3; i++) {
    fcm.insert(t);
    fcm.insert(t2);
  }
  for (size_t i = 0; i < 3 * (15 + 3); i++) {
    fcm.insert(t3);
    fcm.insert(t4++);
  }
  fcm.analyze(0);
  fcm.print_sketch();
  REQUIRE(fcm.wmre < 2.0);
}
