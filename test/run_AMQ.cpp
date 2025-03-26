#include "bloomfilter.h"
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

TEST_CASE("All datasets BloomFilter with SrcTuple", "[Src][BF]") {
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

  for (string &f : filenames) {
    dataParser data_parser;
    TRACE trace = data_parser.get_trace(f.data(), tuple_sz);
    std::cout << "[DataParser] Finished parsing data" << std::endl;
    string file = f.erase(f.find("data/"), sizeof("data/") - 1);
    file = file.erase(file.find(".dat"), sizeof(".dat") - 1);

    BloomFilter bfFilter(4, 2097152, 4, file, tuple_sz);

    for (size_t i = 0; i < trace.size(); i++) {
      bfFilter.insert(trace.at(i));
    }

    bfFilter.print_sketch();

    bfFilter.analyze(0);
    REQUIRE(bfFilter.f1 > 0.0);
  }
}

TEST_CASE("All datasets LazyBloomFilter with SrcTuple", "[Src][LBF]") {
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

  for (string &f : filenames) {
    dataParser data_parser;
    TRACE trace = data_parser.get_trace(f.data(), tuple_sz);
    std::cout << "[DataParser] Finished parsing data" << std::endl;
    string file = f.erase(f.find("data/"), sizeof("data/") - 1);
    file = file.erase(file.find(".dat"), sizeof(".dat") - 1);

    LazyBloomFilter bfFilter(4, 2097152, 4, file, tuple_sz);

    for (size_t i = 0; i < trace.size(); i++) {
      bfFilter.insert(trace.at(i));
    }

    bfFilter.print_sketch();

    bfFilter.analyze(0);
    REQUIRE(bfFilter.f1 > 0.0);
  }
}

TEST_CASE("All datasets Waterfall with SrcTuple", "[Src][Waterfall]") {
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

  for (string &f : filenames) {
    dataParser data_parser;
    TRACE trace = data_parser.get_trace(f.data(), tuple_sz);
    std::cout << "[DataParser] Finished parsing data" << std::endl;
    string file = f.erase(f.find("data/"), sizeof("data/") - 1);
    file = file.erase(file.find(".dat"), sizeof(".dat") - 1);

    Waterfall waterfall(4, 65536, file, tuple_sz);

    for (size_t i = 0; i < trace.size(); i++) {
      waterfall.insert(trace.at(i));
    }
    /*waterfall.print_sketch();*/

    waterfall.analyze(0);
    REQUIRE(waterfall.f1 > 0.0);
  }
}

TEST_CASE("All datasets BloomFilter with SrcDstTuple", "[SrcDst][BF]") {
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

  for (string &f : filenames) {
    dataParser data_parser;
    TRACE trace = data_parser.get_trace(f.data(), tuple_sz);
    std::cout << "[DataParser] Finished parsing data" << std::endl;
    string file = f.erase(f.find("data/"), sizeof("data/") - 1);
    file = file.erase(file.find(".dat"), sizeof(".dat") - 1);

    BloomFilter bfFilter(4, 2097152, 4, file, tuple_sz);

    for (size_t i = 0; i < trace.size(); i++) {
      bfFilter.insert(trace.at(i));
    }

    bfFilter.print_sketch();

    bfFilter.analyze(0);
    REQUIRE(bfFilter.f1 > 0.0);
  }
}

TEST_CASE("All datasets LazyBloomFilter with SrcDstTuple", "[SrcDst][LBF]") {
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

  for (string &f : filenames) {
    dataParser data_parser;
    TRACE trace = data_parser.get_trace(f.data(), tuple_sz);
    std::cout << "[DataParser] Finished parsing data" << std::endl;
    string file = f.erase(f.find("data/"), sizeof("data/") - 1);
    file = file.erase(file.find(".dat"), sizeof(".dat") - 1);

    LazyBloomFilter bfFilter(4, 2097152, 4, file, tuple_sz);

    for (size_t i = 0; i < trace.size(); i++) {
      bfFilter.insert(trace.at(i));
    }

    bfFilter.print_sketch();

    bfFilter.analyze(0);
    REQUIRE(bfFilter.f1 > 0.0);
  }
}
TEST_CASE("All datasets Waterfall with SrcDstTuple", "[SrcDst][Waterfall]") {
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

  for (string &f : filenames) {
    dataParser data_parser;
    TRACE trace = data_parser.get_trace(f.data(), tuple_sz);
    std::cout << "[DataParser] Finished parsing data" << std::endl;
    string file = f.erase(f.find("data/"), sizeof("data/") - 1);
    file = file.erase(file.find(".dat"), sizeof(".dat") - 1);

    Waterfall waterfall(4, 65536, file, tuple_sz);

    for (size_t i = 0; i < trace.size(); i++) {
      waterfall.insert(trace.at(i));
    }
    /*waterfall.print_sketch();*/

    waterfall.analyze(0);
    REQUIRE(waterfall.f1 > 0.0);
  }
}
