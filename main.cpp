// #include "count-min.h"
#include "EM_FSD.hpp"
#include "common.h"
/*#include "count-min.cpp"*/
#include "cuckoo-hash.hpp"
#include "data-parser.h"
/*#include "fcm-sketch.hpp"*/
/*#include "fcm-sketches.hpp"*/
#include "iblt.h"
#include "pds.h"
#include "qwaterfall.hpp"
#include "simulator.h"
#include "waterfall-fcm.hpp"
/*#include "src/bloomfilter.h"*/
/*#include "waterfall-fcm.hpp"*/
#include "qwaterfall-fcm.hpp"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <glob.h>
#include <iostream>
#include <limits>
#include <malloc.h>
#include <ostream>
#include <unordered_map>

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

  uint32_t sim_length = 10;
  uint32_t iter = 1;

  for (string &f : filenames) {
    std::cout << "[DataParser] Start parsing " << f << "..." << std::endl;
    dataParser<FLOW_TUPLE, TRACE_FLOW> data_parser;
    TRACE_FLOW trace = data_parser.get_trace(f.data());
    std::cout << "[DataParser] Finished parsing data" << std::endl;

    // PDS
    /*WaterfallFCM<FLOW_TUPLE, flowTupleHash> waterfall_fcm(8192, 3, 8, 100000,
     * 1,*/
    /*                                                      4, 65355, f, 0,
     * 0);*/

    /*qWaterfall_Fcm<FLOW_TUPLE, flowTupleHash> qwaterfall_fcm(4, 65355, 65355,
     * 1,*/
    /*                                                         f, 0, 0);*/
    /**/
    /*unsigned long num_pkts = trace.size();*/
    /*unsigned long packets_per_epoch =*/
    /*    std::floor(num_pkts / sim_length) - 1; // per iteration*/
    /*std::cout << "[Sim] Starting run with of " << f << " over " << num_pkts*/
    /*          << " packets" << " with a line rate of " << packets_per_epoch*/
    /*          << " p/s over " << iter << " epoch(s)" << std::endl;*/
    /**/
    /*// Split into epochs for simulations*/
    /*auto start = std::chrono::high_resolution_clock::now();*/
    /*for (size_t i = 0; i < iter; i++) {*/
    /*  std::cout << "\rEpoch: " << i << std::endl;*/
    /*  for (size_t j = i * packets_per_epoch; j < (i + 1) *
     * packets_per_epoch;*/
    /*       j++) {*/
    /*    // Exit if running outside of trace*/
    /*    if (j >= num_pkts) {*/
    /*      break;*/
    /*    }*/
    /**/
    /*    // Add PDS here*/
    /*waterfall_fcm.insert(trace.at(j));*/
    /*    qwaterfall_fcm.insert(trace.at(j));*/
    /*  }*/
    /*  // Store data, analyze data and reset the PDS*/
    /*  qwaterfall_fcm.analyze(i);*/
    /*  qwaterfall_fcm.reset();*/
    /*}*/
    /*auto stop = std::chrono::high_resolution_clock::now();*/
    /*auto time = duration_cast<std::chrono::milliseconds>(stop - start);*/
    /*std::cout << std::endl;*/
    /*std::cout << "[Sim] Finished data set with time: " << time << std::endl;*/
    /**/
    vector<PDS<FLOW_TUPLE, flowTupleHash> *> stages;
    /*qWaterfall<FLOW_TUPLE, flowTupleHash> qwaterfall(*/
    /*    4, std::numeric_limits<uint16_t>::max(),*/
    /*    std::numeric_limits<uint32_t>::max(), f, 0, 0);*/
    /*stages.push_back(&qwaterfall);*/

    qWaterfall_Fcm<FLOW_TUPLE, flowTupleHash> qwaterfall_fcm(4, 3, f, 0, 0);
    stages.push_back(&qwaterfall_fcm);

    /*qWaterfall<FLOW_TUPLE, flowTupleHash> qwaterfall(*/
    /*    4, std::numeric_limits<uint16_t>::max(),*/
    /*    std::numeric_limits<uint16_t>::max(), f, 0, 0);*/
    /*stages.push_back(&qwaterfall);*/
    /**/
    /*qWaterfall<FLOW_TUPLE, flowTupleHash> qwaterfall_16(*/
    /*    4, 8 * std::numeric_limits<uint16_t>::max(),*/
    /*    8 * std::numeric_limits<uint16_t>::max(),
     * data_parser.n_unique_tuples,*/
    /*    f, 0, 0);*/
    /*stages.push_back(&qwaterfall_16);*/

    /*CuckooHash<FLOW_TUPLE, flowTupleHash> cuckoo(*/
    /*    4, std::numeric_limits<uint16_t>::max(), data_parser.n_unique_tuples,
     * f,*/
    /*    0, 0);*/
    /*stages.push_back(&cuckoo);*/

    std::cout << "[PDS] Added " << stages.size() << " stages" << std::endl;
    Simulator<PDS<FLOW_TUPLE, flowTupleHash>, TRACE_FLOW> sim(
        stages, stages.size(), sim_length);
    std::cout << "[Simulator] Created simulator for FLOW_TUPLE" << std::endl;

    std::cout << "[Simulator] Starting simulations..." << std::endl;
    sim.run(trace, iter);
    std::cout << "[Simulator] ...done!" << std::endl;
    std::cout << std::endl << std::endl;

    break;
    // Run simulations on 5-tuple
    /*std::cout << "[DataParser] Start parsing " << f << "..." << std::endl;*/
    /*dataParser<FIVE_TUPLE, TRACE> data_parser_5t;*/
    /*TRACE trace_5t = data_parser_5t.get_trace(f.data());*/
    /*std::cout << "[DataParser] Finished parsing data" << std::endl;*/
    /**/
    /*vector<PDS<FIVE_TUPLE, fiveTupleHash> *> stages_5t;*/

    /*qWaterfall<FIVE_TUPLE, fiveTupleHash> qwaterfall_5t(*/
    /*    6, std::numeric_limits<uint16_t>::max(),
     * data_parser_5t.n_unique_tuples,*/
    /*    f, 0, 0);*/
    /*stages_5t.push_back(&qwaterfall_5t);*/

    /*CuckooHash<FIVE_TUPLE, fiveTupleHash> cuckoo(*/
    /*    4, std::numeric_limits<uint16_t>::max(),
     * data_parser_5t.n_unique_tuples,*/
    /*    f, 0, 0);*/
    /*stages_5t.push_back(&cuckoo);*/

    /*std::cout << "[PDS] Added " << stages_5t.size() << " stages" <<
     * std::endl;*/
    /*Simulator<PDS<FIVE_TUPLE, fiveTupleHash>, TRACE> sim_5t(*/
    /*    stages_5t, stages_5t.size(), sim_length);*/
    /*std::cout << "[Simulator] Created simulator for FIVE_TUPLE" <<
     * std::endl;*/
    /**/
    /*std::cout << "[Simulator] Starting simulations..." << std::endl;*/
    /*sim_5t.run(trace_5t, iter);*/
    /*std::cout << "[Simulator] ...done!" << std::endl;*/
  }

  std::cout << "------" << std::endl;
  std::cout << "Finished all simulations!" << std::endl;
  std::cout << "------" << std::endl;

  return 0;
}
