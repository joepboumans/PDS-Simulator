#include "src/bloomfilter.h"
#include "src/common.h"
#include "src/count-min.h"
#include "src/data-parser.h"
#include "src/fcmsketch.h"
#include "src/simulator/simulator.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <glob.h>
#include <iostream>
#include <iterator>
#include <unordered_map>
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
  unordered_map<string, TRACE> data_traces;
  std::size_t i = 0;
  for (auto f : filenames) {
    std::cout << f << std::endl;
    data_parser.get_traces(f.data(), traces[i]);
    string dir = "data/";
    string dat = ".dat";
    string::size_type k = f.find(dir);
    if (k != string::npos) {
      f.erase(k, dir.length());
    }
    string::size_type j = f.find(dat);
    if (j != string::npos) {
      f.erase(j, dat.length());
    }
    data_traces[f] = traces[i++];
  }

  std::cout << "------" << std::endl;
  std::cout << "Finished parsing data, starting simulations..." << std::endl;
  std::cout << "------" << std::endl;
  // FCM_Sketch *fcmsketch = new FCM_Sketch(4, 0);
  // stages.push_back(fcmsketch);

  int n_trace = 0;
  for (const auto &[name_set, trace] : data_traces) {
    vector<PDS *> stages;

    // CountMin cm(4, 4, 3, name_set, 0, 0);
    // stages.push_back(&cm);

    LazyBloomFilter bfilter(1 * 256 * 1024, 4, name_set, 0, 0);
    bfilter.setName();
    bfilter.setupLogging();
    stages.push_back(&bfilter);
    //
    BloomFilter bfilter2(1 * 256 * 1024, 4, name_set, 1, 1);
    bfilter2.setName();
    bfilter2.setupLogging();
    stages.push_back(&bfilter2);

    Simulator sim(stages, stages.size(), 1);
    // Default length of CAIDA traces is 60s
    sim.run(trace, 60);
    n_trace++;
  }

  std::cout << "------" << std::endl;
  std::cout << "Finished simulations!" << std::endl;
  std::cout << "------" << std::endl;

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
