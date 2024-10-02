#ifndef _WATERFALL_FCM_HPP
#define _WATERFALL_FCM_HPP

#include "common.h"
#include "cuckoo-hash.hpp"
#include "fcm-sketch.hpp"
#include "pds.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <ostream>
#include <vector>

class WaterfallFCM : public PDS {
private:
  FCM_Sketch fcm;
  CuckooHash cuckoo;
  uint32_t n_stages;
  uint32_t em_iters;

  uint32_t hh_threshold;
  std::unordered_set<FIVE_TUPLE, tupleHash> HH_candidates;

public:
  WaterfallFCM(uint32_t n_roots, uint32_t n_stages, uint32_t k,
               uint32_t hh_threshold, uint32_t em_iters, uint32_t n_tables,
               uint32_t length, string trace, uint32_t n_stage,
               uint32_t n_struct)
      : PDS(trace, n_stage, n_struct), fcm(n_roots, n_stages, k, hh_threshold,
                                           em_iters, trace, n_stage, n_struct),
        cuckoo(n_tables, length, trace, n_struct, n_stage), n_stages(n_stages),
        em_iters(em_iters), hh_threshold(hh_threshold) {

    this->fcm.estimate_fsd = false;
    // Setup logging
    this->csv_header = "Epoch,Average Relative Error,Average Absolute "
                       "Error,Weighted Mean Relative"
                       "Error,F1 Heavy Hitter,Estimation "
                       "Time,Iterations,Insertions,F1 Member";
    this->name = "WaterfallFCM";
    this->trace_name = trace;
    this->rows = n_stages;
    this->mem_sz = this->fcm.mem_sz + this->cuckoo.mem_sz;
    std::cout << "Total memory used: " << this->mem_sz << std::endl;
    this->setupLogging();
  }

  uint32_t insert(FIVE_TUPLE tuple);
  uint32_t hashing(FIVE_TUPLE tuple, uint32_t k);
  uint32_t lookup(FIVE_TUPLE tuple);
  void reset();

  void analyze(int epoch);
  bool estimate_fsd = true;
  double average_absolute_error = 0.0;
  double average_relative_error = 0.0;
  double wmre = 0.0;
  double f1_hh = 0.0;
  double f1_member = 0.0;
  uint32_t insertions = 0;

  void store_data();
  void print_sketch();
  vector<double> get_distribution(set<FIVE_TUPLE> tuples);
  void set_estimate_fsd(bool onoff);
};

#endif // !_WATERFALL_FCM_HPP
