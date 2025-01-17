#ifndef _WATERFALL_FCM_HPP
#define _WATERFALL_FCM_HPP

#include "common.h"
#include "fcm-sketches.hpp"
#include "pds.h"
#include "waterfall.hpp"
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
  FCM_Sketches fcm_sketches;
  Waterfall waterfall;
  uint32_t n_stages;
  uint32_t em_iters;
  size_t n_unique_tuples;

  uint32_t hh_threshold;
  std::unordered_set<TUPLE, TupleHash> HH_candidates;

public:
  WaterfallFCM(uint32_t n_roots, uint32_t n_stages, uint32_t k,
               uint32_t hh_threshold, uint32_t em_iters, uint32_t n_tables,
               uint32_t length, string trace, uint8_t tuple_sz,
               uint32_t n_stage, uint32_t n_struct)
      : PDS(trace, n_stage, n_struct, tuple_sz),
        fcm_sketches(W3, 3, 8, DEPTH, 100000, 1, trace, tuple_sz, n_stage,
                     n_struct),
        waterfall(n_tables, length, trace, tuple_sz, n_struct, n_stage),
        n_stages(n_stages), em_iters(em_iters), hh_threshold(hh_threshold) {

    this->fcm_sketches.estimate_fsd = false;
    // Setup logging
    this->csv_header = "Epoch,Average Relative Error,Average Absolute "
                       "Error,Weighted Mean Relative "
                       "Error,F1 Heavy Hitter,Estimation "
                       "Time,Iterations,Insertions,F1 Member";
    this->name = "WaterfallFCM";
    this->trace_name = trace;
    this->rows = n_stages;
    this->mem_sz = this->fcm_sketches.mem_sz + this->waterfall.mem_sz;
    std::cout << "Total memory used: " << this->mem_sz << std::endl;
    this->setupLogging();
  }

  uint32_t insert(TUPLE tuple);
  uint32_t hashing(TUPLE tuple, uint32_t k);
  uint32_t lookup(TUPLE tuple);
  void reset();

  bool estimate_fsd = true;
  double average_absolute_error = 0.0;
  double average_relative_error = 0.0;
  double wmre = 0.0;
  double f1_hh = 0.0;
  double f1_member = 0.0;
  uint32_t insertions = 0;

  void store_data();
  void print_sketch();
  vector<uint32_t> peel_sketches(set<TUPLE> &tuples);
  double get_distribution(set<TUPLE> &tuples, vector<uint32_t> &true_fsd);
  void set_estimate_fsd(bool onoff);
  void analyze(int epoch);
};
#endif // !_WATERFALL_FCM_HPP
