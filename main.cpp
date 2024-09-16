// #include "lib/count-min/count-min.h"
#include "lib/cuckoo-hash/cuckoo-hash.h"
#include "lib/fcm-sketch/fcm-sketch.hpp"
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
#include <malloc.h>
#include <ostream>
#include <unordered_map>
#include <vector>

int main() {
  // mallopt(M_PERTURB, 0x7D);
  dataParser data_parser;
  // Get data files
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

  // Load in traces
  vector<TRACE> traces(filenames.size());
  unordered_map<string, TRACE> data_traces;
  size_t i = 0;
  string dir = "data/";
  string dat = ".dat";
  for (string &f : filenames) {
    std::cout << f << std::endl;
    data_parser.get_traces(f.data(), traces[i]);
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

  for (const auto &[name_set, trace] : data_traces) {
    vector<PDS *> stages;
    // CuckooHash cuckoo(10, 1024, name_set, 0, 0);
    // stages.push_back(&cuckoo);
    FCM_Sketch fcm(128, 4, 8, trace.size() * 0.0005 / 60, name_set, 0, 0);
    stages.push_back(&fcm);
    Simulator sim(stages, stages.size(), 1);
    // Default length of CAIDA traces is 60s
    sim.run(trace, 1);
  }

  std::cout << "------" << std::endl;
  std::cout << "Finished simulations!" << std::endl;
  std::cout << "------" << std::endl;

  return 0;
}
