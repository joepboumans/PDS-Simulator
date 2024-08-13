#include "../src/simulator/simulator.h"
#include "../src/bloomfilter.h"
#include "../src/count-min/count-min.h"
#include "../src/data-parser.h"
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Smoke Test", "[smoke-test]") {
  REQUIRE(true == true);
  REQUIRE(false == false);
  std::cout << "Smoke done!" << std::endl;
}

TEST_CASE("Constructor simulator", "[const][sim]") {
  TRACE trace;
  vector<PDS *> stage;
  Simulator simulator(stage, stage.size(), 1);
  REQUIRE(typeid(simulator) == typeid(Simulator));
}

TEST_CASE("Run simulator on BF", "[sim]") {
  dataParser data_parser;
  string data_set = "data/equinix-chicago.20160218-133000.UTC.dat";
  TRACE trace;
  data_parser.get_traces(data_set.data(), trace);
  vector<PDS *> stage;
  BloomFilter bfilter(256 * 1024, 4, data_set, 0, 0);
  stage.push_back(&bfilter);
  Simulator simulator(stage, stage.size(), 1);
  simulator.run(trace, 60);
}
