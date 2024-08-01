#include "src/bloomfilter.h"
#include "src/common.h"
#include "src/data-parser.h"
#include "src/fcmsketch.h"
#include "src/simulator/simulator.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <glob.h>
#include <iostream>
#include <iterator>
#include <vector>

int main() {
  dataParser data_parser;
  // Get data files
  glob_t glob_res;
  memset(&glob_res, 0, sizeof(glob_res));
  glob("data/*.dat", GLOB_TILDE, NULL, &glob_res);
  vector<std::string> filenames;
  for (size_t i = 0; i < glob_res.gl_pathc; ++i) {
    filenames.push_back(std::string(glob_res.gl_pathv[i]));
  }

  // Load in traces
  TRACE traces[std::size(filenames)];
  std::size_t i = 0;
  for (auto f : filenames) {
    std::cout << f << std::endl;
    data_parser.get_traces(f.data(), traces[i++]);
  }
  // FCM_Sketch *fcmsketch = new FCM_Sketch(4, 0);
  // stages.push_back(fcmsketch);

  int n_trace = 0;
  for (const TRACE &trace : traces) {
    vector<PDS *> stages;
    BloomFilter bfilter(1 * 512 * 1024, 0, 4, n_trace);
    stages.push_back(&bfilter);
    BloomFilter bfilter2(1 * 1024 * 1024, 1, 4, n_trace);
    stages.push_back(&bfilter2);
    Simulator sim(stages, stages.size(), 1);
    // Default length of CAIDA traces is 60s
    sim.run(trace, 60);
    n_trace++;
  }
  std::cout << "Finished simulations!" << std::endl;

  // Print results
  // fcmsketch.print_sketch();
  // for (const auto &n : true_data) {
  //   const char *c = n.first.data();
  //   FIVE_TUPLE tuple;
  //   memcpy(&tuple, c, sizeof(FIVE_TUPLE));
  //   print_five_tuple(tuple);
  // }
  return 0;
}
