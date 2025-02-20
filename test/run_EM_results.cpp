#include "catch_amalgamated.hpp"
#include "common.h"
#include "data-parser.h"
#include "waterfall-fcm.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <vector>

TEST_CASE("All datasets WFCM with SrcTuple", "[Src][WFCM]") {
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
  uint32_t em_iters = 15;

  for (string &f : filenames) {
    dataParser data_parser;
    TRACE trace = data_parser.get_trace(f.data(), tuple_sz);
    std::cout << "[DataParser] Finished parsing data" << std::endl;
    string file = f.erase(f.find("data/"), sizeof("data/") - 1);
    file = file.erase(file.find(".dat"), sizeof(".dat") - 1);

    WaterfallFCM wfcm(W3, NUM_STAGES, K, 100000, em_iters, 4, 65535, file,
                      tuple_sz);
    REQUIRE(wfcm.average_absolute_error == 0);
    REQUIRE(wfcm.average_relative_error == 0);

    for (size_t i = 0; i < trace.size(); i++) {
      wfcm.insert(trace.at(i));
    }

    wfcm.fcm_sketches.print_sketch();

    wfcm.analyze(0);
    REQUIRE(wfcm.wmre < 2.0);
  }
}

TEST_CASE("All datasets FCM with SrcTuple", "[Src][FCM]") {
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
  uint32_t em_iters = 15;

  for (string &f : filenames) {
    dataParser data_parser;
    TRACE trace = data_parser.get_trace(f.data(), tuple_sz);
    std::cout << "[DataParser] Finished parsing data" << std::endl;
    string file = f.erase(f.find("data/"), sizeof("data/") - 1);
    file = file.erase(file.find(".dat"), sizeof(".dat") - 1);

    FCM_Sketches fcm(W3, NUM_STAGES, K, DEPTH, 100000, em_iters, file,
                     tuple_sz);
    REQUIRE(fcm.average_absolute_error == 0);
    REQUIRE(fcm.average_relative_error == 0);

    for (size_t i = 0; i < trace.size(); i++) {
      fcm.insert(trace.at(i));
    }

    fcm.print_sketch();

    fcm.analyze(0);
    REQUIRE(fcm.wmre < 2.0);
  }
}

TEST_CASE("All datasets WFCM with FlowTuple", "[Flow][WFCM]") {
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

  TupleSize tuple_sz = FlowTuple;
  uint32_t em_iters = 15;
  for (string &f : filenames) {
    dataParser data_parser;
    TRACE trace = data_parser.get_trace(f.data(), tuple_sz);
    std::cout << "[DataParser] Finished parsing data" << std::endl;
    string file = f.erase(f.find("data/"), sizeof("data/") - 1);
    file = file.erase(file.find(".dat"), sizeof(".dat") - 1);

    WaterfallFCM wfcm(W3, NUM_STAGES, K, 100000, em_iters, 4, 65535, file,
                      tuple_sz);
    REQUIRE(wfcm.average_absolute_error == 0);
    REQUIRE(wfcm.average_relative_error == 0);

    for (size_t i = 0; i < trace.size(); i++) {
      wfcm.insert(trace.at(i));
    }

    wfcm.fcm_sketches.print_sketch();

    wfcm.analyze(0);
    REQUIRE(wfcm.wmre < 2.0);
  }
}

TEST_CASE("All datasets FCM with FlowTuple", "[Flow][FCM]") {
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

  TupleSize tuple_sz = FlowTuple;
  uint32_t em_iters = 15;
  for (string &f : filenames) {
    dataParser data_parser;
    TRACE trace = data_parser.get_trace(f.data(), tuple_sz);
    std::cout << "[DataParser] Finished parsing data" << std::endl;
    string file = f.erase(f.find("data/"), sizeof("data/") - 1);
    file = file.erase(file.find(".dat"), sizeof(".dat") - 1);

    FCM_Sketches fcm(W3, NUM_STAGES, K, DEPTH, 100000, em_iters, f, tuple_sz);
    REQUIRE(fcm.average_absolute_error == 0);
    REQUIRE(fcm.average_relative_error == 0);

    for (size_t i = 0; i < trace.size(); i++) {
      fcm.insert(trace.at(i));
    }

    fcm.print_sketch();

    fcm.analyze(0);
    REQUIRE(fcm.wmre < 2.0);
  }
}

TEST_CASE("All datasets WFCM with FiveTuple", "[Five][WFCM]") {
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

  TupleSize tuple_sz = FiveTuple;
  uint32_t em_iters = 15;
  for (string &f : filenames) {
    dataParser data_parser;
    TRACE trace = data_parser.get_trace(f.data(), tuple_sz);
    std::cout << "[DataParser] Finished parsing data" << std::endl;
    string file = f.erase(f.find("data/"), sizeof("data/") - 1);
    file = file.erase(file.find(".dat"), sizeof(".dat") - 1);

    WaterfallFCM wfcm(W3, NUM_STAGES, K, 100000, em_iters, 4, 65535, file,
                      tuple_sz);
    REQUIRE(wfcm.average_absolute_error == 0);
    REQUIRE(wfcm.average_relative_error == 0);

    for (size_t i = 0; i < trace.size(); i++) {
      wfcm.insert(trace.at(i));
    }

    wfcm.fcm_sketches.print_sketch();

    wfcm.analyze(0);
    REQUIRE(wfcm.wmre < 2.0);
  }
}

TEST_CASE("All datasets FCM with FiveTuple", "[Five][FCM]") {
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

  TupleSize tuple_sz = FiveTuple;
  uint32_t em_iters = 15;
  for (string &f : filenames) {
    dataParser data_parser;
    TRACE trace = data_parser.get_trace(f.data(), tuple_sz);
    std::cout << "[DataParser] Finished parsing data" << std::endl;
    string file = f.erase(f.find("data/"), sizeof("data/") - 1);
    file = file.erase(file.find(".dat"), sizeof(".dat") - 1);

    FCM_Sketches fcm(W3, NUM_STAGES, K, DEPTH, 100000, em_iters, f, tuple_sz);
    REQUIRE(fcm.average_absolute_error == 0);
    REQUIRE(fcm.average_relative_error == 0);

    for (size_t i = 0; i < trace.size(); i++) {
      fcm.insert(trace.at(i));
    }

    fcm.print_sketch();

    fcm.analyze(0);
    REQUIRE(fcm.wmre < 2.0);
  }
}
