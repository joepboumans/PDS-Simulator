#include "common.h"
#include "data-parser.h"
#include "fcm-sketches.hpp"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Smoke Test", "[smoke-test]") {
  REQUIRE(true == true);
  REQUIRE(false == false);
}

TEST_CASE("Analysis test", "[analysis]") {
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

  FCM_Sketches fcm(4, NUM_STAGES, K, DEPTH, 1, 1, "test", TupleSize::FiveTuple);
  REQUIRE(fcm.average_absolute_error == 0);
  REQUIRE(fcm.average_relative_error == 0);

  for (size_t i = 0; i < 10; i++) {
    fcm.insert(trace.at(i));
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
