#include "lib/simulator/simulator.h"
#include "lib/count-min/count-min.h"
#include "src/bloomfilter.h"
#include "src/data-parser.h"
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

TEST_CASE("Run simulator on BF", "[BF][sim]") {
  dataParser data_parser;
  string data_set =
      "/workspace/master-thesis/data/equinix-chicago-20160121-130000.dat";
  TRACE trace;
  data_parser.get_traces(data_set.data(), trace);
  vector<PDS *> stage;
  BloomFilter bfilter(256 * 1024, 4, data_set, 0, 0);
  bfilter.setName();
  bfilter.setupLogging();
  stage.push_back(&bfilter);
  Simulator simulator(stage, stage.size(), 1);
  simulator.run(trace, 60);
}

TEST_CASE("Run simulator on CM-Sketch", "[CM][sim]") {
  dataParser data_parser;
  string data_set =
      "/workspace/master-thesis/data/equinix-chicago-20160121-130000.dat";
  TRACE trace;
  data_parser.get_traces(data_set.data(), trace);
  vector<PDS *> stage;
  CountMin cm(32, 1024, trace.size() * 0.0005 / 60, data_set, 0, 0);
  stage.push_back(&cm);
  Simulator simulator(stage, stage.size(), 1);
  simulator.run(trace, 60);
}
TEST_CASE("Run simulator on BF->CM-Sketch", "[BF][CM][sim]") {
  dataParser data_parser;
  string data_set = "data/equinix-chicago-20160121-130000.dat";
  TRACE trace;
  data_parser.get_traces(data_set.data(), trace);

  vector<PDS *> stages;

  BloomFilter bfilter(256 * 1024, 4, "sim", 0, 0);
  bfilter.setName();
  bfilter.setupLogging();
  stages.push_back(&bfilter);

  CountMin cm(64, 1024, trace.size() * 0.0005 / 60, "sim", 0, 0);
  stages.push_back(&cm);

  Simulator simulator(stages, stages.size(), 1);
  simulator.run(trace, 60);
}
