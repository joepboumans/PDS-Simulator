#include "common.h"
#include "data-parser.h"
#include "fcm-sketches.hpp"
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers.hpp>
#include <catch2/matchers/catch_matchers_vector.hpp>
#include <cstddef>

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

  FCM_Sketches fcm(W1, NUM_STAGES, K, DEPTH, 1, 5, "test", tuple_sz);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);

  for (size_t i = 0; i < trace.size(); i++) {
    fcm.insert(trace.at(i));
  }

  fcm.print_sketch();

  fcm.analyze(0);
  vector<double> ns_org = fcm.ns;
  double org_wmre = fcm.wmre;

  fcm.estimator_org = false;
  fcm.analyze(0);
  vector<double> ns_wfcm = fcm.ns;
  double wfcm_wmre = fcm.wmre;

  printf("ORG WMRE: %.8f vs WFCM WMRE: %.8f\n", org_wmre, wfcm_wmre);
  // Compare if WMRE and NS are approx equal. Small difference due to different
  // floating point operations between FCMS and WFCM
  REQUIRE(org_wmre == Catch::Approx(wfcm_wmre).epsilon(1e-10));
  REQUIRE_THAT(ns_org, Catch::Matchers::Approx(ns_wfcm).epsilon(1e-10));
}

TEST_CASE("Degree 1, l2 ", "[em][l1]") {
  TupleSize tuple_sz = SrcTuple;
  FCM_Sketches fcm(4, NUM_STAGES, K, DEPTH, 1, 1, "test", tuple_sz);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);

  TUPLE tuple;
  tuple.sz = tuple_sz;
  // Collision in l2, degree 1
  for (size_t i = 0; i < 300; i++) {
    fcm.insert(tuple, 0);
  }

  fcm.print_sketch();

  fcm.analyze(0);
  vector<double> ns_org = fcm.ns;
  double org_wmre = fcm.wmre;

  fcm.estimator_org = false;
  fcm.analyze(0);
  vector<double> ns_wfcm = fcm.ns;
  double wfcm_wmre = fcm.wmre;

  REQUIRE(org_wmre == wfcm_wmre);
  REQUIRE_THAT(ns_org, Catch::Matchers::Equals(ns_wfcm));
}

TEST_CASE("Degree 1, l3 ", "[em][l1]") {
  TupleSize tuple_sz = SrcTuple;
  FCM_Sketches fcm(4, NUM_STAGES, K, DEPTH, 1, 1, "test", tuple_sz);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);

  TUPLE tuple;
  tuple.sz = tuple_sz;
  // Collision in l3, degree 1
  for (size_t i = 0; i < 70000; i++) {
    fcm.insert(tuple, 0);
  }

  fcm.print_sketch();

  fcm.analyze(0);
  vector<double> ns_org = fcm.ns;
  double org_wmre = fcm.wmre;

  fcm.estimator_org = false;
  fcm.analyze(0);
  vector<double> ns_wfcm = fcm.ns;
  double wfcm_wmre = fcm.wmre;

  REQUIRE(org_wmre == wfcm_wmre);
  REQUIRE_THAT(ns_org, Catch::Matchers::Equals(ns_wfcm));
}

TEST_CASE("Degree 2; L2 Collision", "[em][md][d2][l2]") {

  TupleSize tuple_sz = SrcTuple;
  FCM_Sketches fcm(4, NUM_STAGES, K, DEPTH, 1, 1, "test", tuple_sz);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);

  TUPLE tuple;
  tuple.sz = tuple_sz;
  TUPLE tuple2;
  tuple2.sz = tuple_sz;
  tuple2++;
  // Collision in l2, degree 2
  for (size_t i = 0; i < 300; i++) {
    fcm.insert(tuple, 0);
    fcm.insert(tuple2, 1);
  }

  fcm.print_sketch();

  fcm.analyze(0);
  vector<double> ns_org = fcm.ns;
  double org_wmre = fcm.wmre;

  fcm.estimator_org = false;
  fcm.analyze(0);
  vector<double> ns_wfcm = fcm.ns;
  double wfcm_wmre = fcm.wmre;

  REQUIRE(org_wmre == wfcm_wmre);
  REQUIRE_THAT(ns_org, Catch::Matchers::Equals(ns_wfcm));
}

TEST_CASE("Degree 4; 4x L2 Collision", "[em][md][d4][l2]") {

  TupleSize tuple_sz = SrcTuple;
  FCM_Sketches fcm(4, NUM_STAGES, K, DEPTH, 1, 1, "test", tuple_sz);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);

  vector<TUPLE> tuple(4);
  for (size_t i = 0; i < tuple.size(); i++) {
    tuple[i] += i;
    tuple[i].sz = tuple_sz;
  }
  // Collision in l2, degree 2
  for (size_t i = 0; i < 300; i++) {
    for (size_t i = 0; i < tuple.size(); i++) {
      fcm.insert(tuple[i], i);
    }
  }

  fcm.print_sketch();

  fcm.analyze(0);
  vector<double> ns_org = fcm.ns;
  double org_wmre = fcm.wmre;

  fcm.estimator_org = false;
  fcm.analyze(0);
  vector<double> ns_wfcm = fcm.ns;
  double wfcm_wmre = fcm.wmre;

  REQUIRE(org_wmre == wfcm_wmre);
  REQUIRE_THAT(ns_org, Catch::Matchers::Equals(ns_wfcm));
}

TEST_CASE("Degree 8; 8x L2 Collision", "[em][md][l2]") {

  TupleSize tuple_sz = SrcTuple;
  FCM_Sketches fcm(4, NUM_STAGES, K, DEPTH, 1, 1, "test", tuple_sz);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);

  TUPLE tuple[8];
  for (size_t i = 0; i < 8; i++) {
    tuple[i].sz = tuple_sz;
    tuple[i] = tuple[i] + i;
  }
  // Collision in l2, degree 2
  for (size_t i = 0; i < 300; i++) {
    for (size_t i = 0; i < 8; i++) {
      fcm.insert(tuple[i], i);
    }
  }

  fcm.print_sketch();

  fcm.analyze(0);
  vector<double> ns_org = fcm.ns;
  double org_wmre = fcm.wmre;

  fcm.estimator_org = false;
  fcm.analyze(0);
  vector<double> ns_wfcm = fcm.ns;
  double wfcm_wmre = fcm.wmre;

  REQUIRE(org_wmre == wfcm_wmre);
  REQUIRE_THAT(ns_org, Catch::Matchers::Equals(ns_wfcm));
}

TEST_CASE("Degree 2; L2 collision; L3 overflow", "[em][md][l3]") {

  TupleSize tuple_sz = SrcTuple;
  FCM_Sketches fcm(4, NUM_STAGES, K, DEPTH, 1, 1, "test", tuple_sz);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);

  TUPLE tuple;
  tuple.sz = tuple_sz;
  // Collision in l2, degree 2, overflow to l3
  for (size_t i = 0; i < 70000; i++) {
    fcm.insert(tuple, 0);
  }
  tuple++;
  for (size_t i = 0; i < 300; i++) {
    fcm.insert(tuple, 1);
  }

  fcm.print_sketch();
  fcm.analyze(0);
  vector<double> ns_org = fcm.ns;
  double org_wmre = fcm.wmre;

  fcm.estimator_org = false;
  fcm.analyze(0);
  vector<double> ns_wfcm = fcm.ns;
  double wfcm_wmre = fcm.wmre;

  REQUIRE(org_wmre == wfcm_wmre);
  REQUIRE_THAT(ns_org, Catch::Matchers::Equals(ns_wfcm));
}

TEST_CASE("Degree 2; L3 Collision", "[em][md][l3]") {
  std::cout << std::endl;
  std::cout << "-------------" << std::endl;
  std::cout << "[TEST CASE] Degree 2; L3 Collision" << std::endl;
  std::cout << "-------------" << std::endl;
  std::cout << std::endl;

  TupleSize tuple_sz = SrcTuple;
  FCM_Sketches fcm(4, NUM_STAGES, K, DEPTH, 1, 1, "test", tuple_sz);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);

  TUPLE tuple;
  tuple.sz = tuple_sz;
  TUPLE tuple2;
  tuple2++;
  tuple2.sz = tuple_sz;
  // Collision in l3, degree 2
  for (size_t i = 0; i < 70000; i++) {
    fcm.insert(tuple, 0);
    fcm.insert(tuple2, 8);
  }

  fcm.print_sketch();

  fcm.analyze(0);
  vector<double> ns_org = fcm.ns;
  double org_wmre = fcm.wmre;

  fcm.estimator_org = false;
  fcm.analyze(0);
  vector<double> ns_wfcm = fcm.ns;
  double wfcm_wmre = fcm.wmre;

  REQUIRE(org_wmre == wfcm_wmre);
  REQUIRE_THAT(ns_org, Catch::Matchers::Equals(ns_wfcm));
}
