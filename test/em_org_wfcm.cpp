#include "common.h"
#include "data-parser.h"
#include "waterfall-fcm.hpp"
#include <catch2/catch_all.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/internal/catch_run_context.hpp>
#include <catch2/internal/catch_stdstreams.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

vector<TUPLE> generate_n_tuples(uint32_t n, TupleSize sz) {
  std::srand(Catch::rngSeed());
  static uint32_t c = 1;
  vector<TUPLE> tuple(n);
  for (size_t i = 0; i < n; i++) {
    tuple[i].sz = sz;
    tuple[i] = tuple[i] + i + c;
  }
  c += n;
  return tuple;
}

void compare_fsd(vector<double> x, vector<double> y) {
  if (x.size() != y.size()) {
    std::cout << "[ERROR] not matching sizes!" << std::endl;
    exit(1);
  }
  for (size_t i = 0; i < x.size(); i++) {
    if (x[i] == 0 and y[i] == 0) {
      continue;
    }

    std::cout << i << ": " << x[i] << " = " << y[i];
    if (&x[i] != &x.back()) {
      std::cout << ", ";
    }
    CHECK(x[i] == y[i]);
  }
  std::cout << std::endl;
}

TEST_CASE("Smoke Test", "[smoke-test]") {
  REQUIRE(true == true);
  REQUIRE(false == false);
}

TEST_CASE("Small trace of EM FCM vs EM WFCM", "[trace][small]") {
  glob_t *glob_res = new glob_t;
  if (memset(glob_res, 0, sizeof(glob_t)) == NULL) {
    std::cout << "Glob init failed" << std::endl;
    exit(1);
  }
  glob("data/*.dat", GLOB_TILDE, NULL, glob_res);
  vector<string> filenames(glob_res->gl_pathc);
  for (size_t i = 0; i < glob_res->gl_pathc; ++i) {
    filenames[i] = string(glob_res->gl_pathv[i]);
  }
  globfree(glob_res);

  TupleSize tuple_sz = SrcTuple;
  dataParser data_parser;
  TRACE trace = data_parser.get_trace(filenames[0].data(), tuple_sz);
  std::cout << "[DataParser] Finished parsing data" << std::endl;
  string file =
      filenames[0].erase(filenames[0].find("data/"), sizeof("data/") - 1);
  file = file.erase(file.find(".dat"), sizeof(".dat") - 1);

  WaterfallFCM wfcm(W3, NUM_STAGES, K, 1, 1, 4, 65535, file, tuple_sz);
  REQUIRE(wfcm.average_absolute_error == 0);
  REQUIRE(wfcm.average_relative_error == 0);

  /*for (size_t i = 0; i < trace.size(); i++) {*/
  for (size_t i = 0; i < 10000; i++) {
    wfcm.insert(trace.at(i));
  }

  wfcm.fcm_sketches.print_sketch();

  wfcm.analyze(0);
  vector<double> ns_wfcm = wfcm.fcm_sketches.ns;
  double wfcm_wmre = wfcm.wmre;

  wfcm.fcm_sketches.estimate_fsd = true;
  wfcm.estimate_fsd = false;
  wfcm.analyze(0);

  vector<double> ns_org = wfcm.fcm_sketches.ns;
  double org_wmre = wfcm.wmre;

  printf("ORG WMRE: %.8f vs WFCM WMRE: %.8f\n", org_wmre, wfcm_wmre);
  // Compare if WMRE and NS are approx equal. Small difference due to different
  // floating point operations between FCMS and WFCM
  CHECK(wfcm_wmre <= org_wmre);
  /*compare_fsd(ns_org, ns_wfcm);*/
  /*CHECK_THAT(ns_org, Catch::Matchers::Approx(ns_wfcm).margin(0.0001));*/
}

TEST_CASE("Degree 1 - 3 ; l1", "[em][l1]") {
  TupleSize tuple_sz = SrcTuple;
  WaterfallFCM wfcm(4, NUM_STAGES, K, 1, 1, 4, 65535, "test", tuple_sz);
  REQUIRE(wfcm.average_absolute_error == 0);
  REQUIRE(wfcm.average_relative_error == 0);

  // Insert flows
  vector<TUPLE> tuple = generate_n_tuples(4, tuple_sz);
  for (size_t j = 0; j < 3; j++) {
    wfcm.insert(tuple.at(j), 0);
  }

  wfcm.insert(tuple.at(3), 1);
  wfcm.fcm_sketches.print_sketch();

  wfcm.analyze(0);
  vector<double> ns_wfcm = wfcm.fcm_sketches.ns;
  double wfcm_wmre = wfcm.wmre;

  wfcm.fcm_sketches.estimate_fsd = true;
  wfcm.estimate_fsd = false;
  wfcm.analyze(0);

  vector<double> ns_org = wfcm.fcm_sketches.ns;
  double org_wmre = wfcm.wmre;

  printf("ORG WMRE: %.8f vs WFCM WMRE: %.8f\n", org_wmre, wfcm_wmre);
  CHECK(wfcm_wmre <= org_wmre);
  /*compare_fsd(ns_org, ns_wfcm);*/
}

TEST_CASE("Degree 1 - 3 ; l2", "[em][l1][l2]") {
  TupleSize tuple_sz = SrcTuple;
  WaterfallFCM wfcm(4, NUM_STAGES, K, 1, 1, 4, 65535, "test", tuple_sz);
  REQUIRE(wfcm.average_absolute_error == 0);
  REQUIRE(wfcm.average_relative_error == 0);

  // Insert flows for sketch degree 2
  vector<TUPLE> tuple = generate_n_tuples(4, tuple_sz);
  for (size_t j = 0; j < 2; j++) {
    for (size_t i = 0; i < 4; i++) {
      wfcm.insert(tuple.at(j), 0);
    }
  }
  for (size_t i = 0; i < 260; i++) {
    wfcm.insert(tuple.at(2), 0);
  }

  wfcm.insert(tuple.at(3), 1);
  wfcm.fcm_sketches.print_sketch();

  wfcm.analyze(0);
  vector<double> ns_wfcm = wfcm.fcm_sketches.ns;
  double wfcm_wmre = wfcm.wmre;

  wfcm.fcm_sketches.estimate_fsd = true;
  wfcm.estimate_fsd = false;
  wfcm.analyze(0);

  vector<double> ns_org = wfcm.fcm_sketches.ns;
  double org_wmre = wfcm.wmre;

  printf("ORG WMRE: %.8f vs WFCM WMRE: %.8f\n", org_wmre, wfcm_wmre);
  CHECK(wfcm_wmre <= org_wmre);
  /*compare_fsd(ns_org, ns_wfcm);*/
}

TEST_CASE("Degree 2 ; l2", "[em][l2]") {
  TupleSize tuple_sz = SrcTuple;
  WaterfallFCM wfcm(4, NUM_STAGES, K, 1, 1, 4, 65535, "test", tuple_sz);
  REQUIRE(wfcm.average_absolute_error == 0);
  REQUIRE(wfcm.average_relative_error == 0);

  vector<TUPLE> tuple = generate_n_tuples(2, tuple_sz);
  std::cout << tuple[0] << " and " << tuple[1] << std::endl;

  // Insert flows
  for (size_t i = 0; i < 260; i++) {
    for (size_t j = 0; j < 2; j++) {
      wfcm.insert(tuple.at(j), j);
    }
  }
  wfcm.fcm_sketches.print_sketch();

  wfcm.analyze(0);
  vector<double> ns_wfcm = wfcm.fcm_sketches.ns;
  double wfcm_wmre = wfcm.wmre;

  wfcm.fcm_sketches.estimate_fsd = true;
  wfcm.estimate_fsd = false;
  wfcm.analyze(0);

  vector<double> ns_org = wfcm.fcm_sketches.ns;
  double org_wmre = wfcm.wmre;

  printf("ORG WMRE: %.8f vs WFCM WMRE: %.8f\n", org_wmre, wfcm_wmre);
  CHECK(wfcm_wmre <= org_wmre);

  REQUIRE(org_wmre == wfcm_wmre);
  compare_fsd(ns_org, ns_wfcm);
  REQUIRE_THAT(ns_org, Catch::Matchers::Equals(ns_wfcm));
}

TEST_CASE("Degree 2 - 4 ; l2", "[em][l2]") {
  TupleSize tuple_sz = SrcTuple;
  WaterfallFCM wfcm(4, NUM_STAGES, K, 1, 1, 4, 65535, "test", tuple_sz);
  REQUIRE(wfcm.average_absolute_error == 0);
  REQUIRE(wfcm.average_relative_error == 0);

  // Insert flows for sketch degree 2
  vector<TUPLE> tuple = generate_n_tuples(8, tuple_sz);
  for (size_t i = 0; i < 260; i++) {
    for (size_t j = 0; j < 2; j++) {
      wfcm.insert(tuple.at(j), j);
    }
  }
  // At extra information from Waterfall
  for (size_t j = 0; j < 6; j++) {
    wfcm.insert(tuple.at(j + 2), j % 2);
  }

  // Add some tuples for Bayes estimation
  vector<TUPLE> single_tuples = generate_n_tuples(4, tuple_sz);
  for (size_t j = 0; j < 4; j++) {
    wfcm.insert(single_tuples.at(j), j + 2);
  }

  vector<TUPLE> overflow_tuples = generate_n_tuples(1, tuple_sz);
  for (size_t i = 0; i < 260; i++) {
    wfcm.insert(overflow_tuples[0], 16);
  }
  wfcm.fcm_sketches.print_sketch();

  wfcm.analyze(0);
  vector<double> ns_wfcm = wfcm.fcm_sketches.ns;
  double wfcm_wmre = wfcm.wmre;

  wfcm.fcm_sketches.estimate_fsd = true;
  wfcm.estimate_fsd = false;
  wfcm.analyze(0);

  vector<double> ns_org = wfcm.fcm_sketches.ns;
  double org_wmre = wfcm.wmre;

  printf("ORG WMRE: %.8f vs WFCM WMRE: %.8f\n", org_wmre, wfcm_wmre);
  CHECK(wfcm_wmre <= org_wmre);

  REQUIRE(org_wmre == wfcm_wmre);
  compare_fsd(ns_org, ns_wfcm);
  /*REQUIRE_THAT(ns_org, Catch::Matchers::Equals(ns_wfcm));*/
}

TEST_CASE("Multi Degree ; l1", "[em][md][l1]") {
  TupleSize tuple_sz = SrcTuple;
  WaterfallFCM wfcm(4, NUM_STAGES, K, 1, 1, 4, 65535, "test", tuple_sz);
  size_t l1_sz = wfcm.fcm_sketches.stages_sz[0];
  REQUIRE(wfcm.average_absolute_error == 0);
  REQUIRE(wfcm.average_relative_error == 0);

  auto l1_flows = GENERATE(64, 128, 256, 2048);
  vector<TUPLE> tuple = generate_n_tuples(l1_flows, tuple_sz);

  std::random_device rd{};
  std::mt19937 gen{rd()};
  std::normal_distribution d{0.9, 16.0};
  auto random_uint = [&d, &gen] {
    auto val = d(gen);
    if (val <= 0.0) {
      return (uint32_t)1;
    }
    return (uint32_t)std::lround(val);
  };
  // Insert L1 flows
  for (size_t j = 0; j < l1_flows; j++) {
    uint32_t idx = rand() % l1_sz;
    for (size_t i = 0; i < random_uint(); i++) {
      wfcm.insert(tuple.at(j), idx);
    }
  }
  wfcm.fcm_sketches.print_sketch();

  wfcm.analyze(0);
  vector<double> ns_wfcm = wfcm.fcm_sketches.ns;
  double wfcm_wmre = wfcm.wmre;

  wfcm.fcm_sketches.estimate_fsd = true;
  wfcm.estimate_fsd = false;
  wfcm.analyze(0);

  vector<double> ns_org = wfcm.fcm_sketches.ns;
  double org_wmre = wfcm.wmre;

  CHECK(org_wmre < 2.0);
  CHECK(wfcm_wmre < 2.0);
  CHECK(wfcm_wmre <= org_wmre);

  compare_fsd(ns_org, ns_wfcm);
}

TEST_CASE("Multi Degree ; All", "[em][md][all]") {
  TupleSize tuple_sz = SrcTuple;
  WaterfallFCM wfcm(4, NUM_STAGES, K, 1, 1, 4, 65535, "test", tuple_sz);
  size_t l1_sz = wfcm.fcm_sketches.stages_sz[0];
  REQUIRE(wfcm.average_absolute_error == 0);
  REQUIRE(wfcm.average_relative_error == 0);

  auto l1_flows = GENERATE(512, 1024);
  auto l2_flows = GENERATE(2, 4);
  auto l3_flows = GENERATE(64);
  vector<TUPLE> tuple =
      generate_n_tuples(l1_flows + l2_flows + l3_flows, tuple_sz);

  std::random_device rd{};
  std::mt19937 gen{rd()};
  std::normal_distribution d{0.9, 16.0};
  auto random_uint = [&d, &gen] {
    auto val = d(gen);
    if (val <= 0.0) {
      return (uint32_t)1;
    }
    return (uint32_t)std::lround(val);
  };
  // Insert L1 flows
  for (size_t j = 0; j < l1_flows; j++) {
    uint32_t idx = rand() % l1_sz;
    for (size_t i = 0; i < random_uint(); i++) {
      wfcm.insert(tuple.at(j), idx);
    }
  }
  // Insert L3 flows
  for (size_t j = 0; j < l3_flows; j++) {
    uint32_t idx = rand() % l1_sz;
    for (size_t i = 0; i < 70000; i++) {
      wfcm.insert(tuple.at(j + l1_flows), idx);
    }
  }

  // Insert L2 flows
  for (size_t j = 0; j < l2_flows; j++) {
    uint32_t idx = rand() % l1_sz;
    for (size_t i = 0; i < 254 + random_uint(); i++) {
      wfcm.insert(tuple.at(j + l3_flows + l1_flows), idx);
    }
  }

  wfcm.fcm_sketches.print_sketch();

  wfcm.analyze(0);
  vector<double> ns_wfcm = wfcm.fcm_sketches.ns;
  double wfcm_wmre = wfcm.wmre;

  wfcm.fcm_sketches.estimate_fsd = true;
  wfcm.estimate_fsd = false;
  wfcm.analyze(0);

  vector<double> ns_org = wfcm.fcm_sketches.ns;
  double org_wmre = wfcm.wmre;

  CHECK(org_wmre < 2.0);
  CHECK(wfcm_wmre < 2.0);
  CHECK(org_wmre < 1.0);
  CHECK(wfcm_wmre < 1.0);

  REQUIRE(org_wmre == wfcm_wmre);
  compare_fsd(ns_org, ns_wfcm);
  /*REQUIRE_THAT(ns_org, Catch::Matchers::Equals(ns_wfcm));*/
}
