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
  Waterfall waterfall;
  uint32_t n_stages;
  uint32_t em_iters;
  vector<uint32_t> idx_degree;

  uint32_t hh_threshold;
  std::unordered_set<TUPLE, TupleHash> HH_candidates;

public:
  FCM_Sketches fcm_sketches;
  WaterfallFCM(uint32_t n_roots, uint32_t n_stages, uint32_t k,
               uint32_t hh_threshold, uint32_t em_iters, uint32_t n_tables,
               uint32_t length, string trace, uint8_t tuple_sz)
      : PDS(trace, tuple_sz), waterfall(n_tables, length, trace, tuple_sz),
        n_stages(n_stages), em_iters(em_iters), hh_threshold(hh_threshold),
        fcm_sketches(n_roots, n_stages, k, DEPTH, hh_threshold, em_iters, trace,
                     tuple_sz) {

    this->fcm_sketches.estimate_fsd = false;

    // Setup logging
    this->csv_header_em =
        "Epoch,Estimation Time,Total Time,Weighted Mean Relative Error,"
        "Cardinality,Entropy";
    this->csv_header = "Average Relative Error,Average Absolute "
                       "Error,Weighted Mean Relative "
                       "Error,F1 Heavy Hitter,Insertions,F1 Member";
    this->name = "FC_AMQ/WaterfallFCM";
    this->trace_name = trace;
    this->rows = n_stages;
    this->mem_sz = this->fcm_sketches.mem_sz + this->waterfall.mem_sz;
    std::cout << "Total memory used: " << this->mem_sz << std::endl;
    this->setupLogging();
  }

  uint32_t insert(TUPLE tuple);
  uint32_t insert(TUPLE tuple, uint32_t idx);
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
  void write2csv();
  void write2csv_em(uint32_t iter, size_t time, size_t total_time, double card,
                    double entropy);
  vector<uint32_t> peel_sketches(set<TUPLE> &tuples);
  vector<vector<uint32_t>> get_initial_degrees(set<TUPLE> tuples);
  double get_distribution(set<TUPLE> &tuples, vector<uint32_t> &true_fsd);
  void set_estimate_fsd(bool onoff);
  void analyze(int epoch);
};
#endif // !_WATERFALL_FCM_HPP
