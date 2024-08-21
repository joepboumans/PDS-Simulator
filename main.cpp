#include "lib/count-min/count-min.h"
#include "lib/cuckoo-hash/cuckoo-hash.h"
#include "lib/fcm-sketch/fcm-sketch.h"
#include "lib/iblt/iblt.h"
#include "lib/simulator/simulator.h"
#include "src/bloomfilter.h"
#include "src/common.h"
#include "src/data-parser.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <glob.h>
#include <iostream>
#include <iterator>
#include <ostream>
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
    break;
  }

  std::cout << "------" << std::endl;
  std::cout << "Finished parsing data, starting simulations..." << std::endl;
  std::cout << "------" << std::endl;

  for (const auto &[name_set, trace] : data_traces) {
    vector<PDS *> stages;

    // CuckooHash cuckoo(10, 1024, name_set, 0, 0);
    // stages.push_back(&cuckoo);
    FCM_Sketch fcm(3, 3, 8, name_set, 0, 0);
    stages.push_back(&fcm);
    Simulator sim(stages, stages.size(), 1);
    // Default length of CAIDA traces is 60s
    sim.run(trace, 60);
  }

  std::cout << "------" << std::endl;
  std::cout << "Finished simulations!" << std::endl;
  std::cout << "------" << std::endl;

  return 0;
}
