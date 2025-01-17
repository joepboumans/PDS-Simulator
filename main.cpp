// #include "count-min.h"
#include "EM_FSD.hpp"
#include "common.h"
#include "data-parser.h"
#include "qwaterfall-fcm.hpp"
#include "simulator.h"
#include "waterfall-fcm.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <glob.h>
#include <iostream>
#include <malloc.h>
#include <ostream>

int main() {
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

  uint32_t sim_length = 1;
  uint32_t iter = 1;

  uint32_t curr_file = 0;
  TupleSize tuple_sz = SrcTuple;

  for (string &f : filenames) {
    std::cout << "[DataParser] Start parsing " << f << "..." << std::endl;

    dataParser data_parser;
    TRACE trace = data_parser.get_trace(f.data(), tuple_sz);
    std::cout << "[DataParser] Finished parsing data" << std::endl;
    string file = f.erase(f.find("data/"), sizeof("data/") - 1);
    file = file.erase(file.find(".dat"), sizeof(".dat") - 1);

    vector<PDS *> stages;
    qWaterfall_Fcm qwaterfall_fcm(4, 5, file, tuple_sz);
    stages.push_back(&qwaterfall_fcm);
    /*WaterfallFCM waterfall_fcm(W3, NUM_STAGES, K, 100000, 5, 4, 65535, file,*/
    /*                           tuple_sz, 0, 0);*/
    /*stages.push_back(&waterfall_fcm);*/

    /*FCM_Sketches fcm_sketches(W3, NUM_STAGES, K, DEPTH, 100000, 5, file,*/
    /*                          tuple_sz, 0, 0);*/
    /*stages.push_back(&fcm_sketches);*/

    std::cout << "[PDS] Added " << stages.size() << " stages" << std::endl;
    Simulator sim(stages, stages.size(), sim_length);
    std::cout << "[Simulator] Starting simulations..." << std::endl;
    sim.run(trace, iter);
    std::cout << "[Simulator] ...done!" << std::endl;
    std::cout << std::endl << std::endl;

    curr_file++;
  }

  std::cout << "------" << std::endl;
  std::cout << "Finished all simulations!" << std::endl;
  std::cout << "------" << std::endl;

  return 0;
}
