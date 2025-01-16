#ifndef _FCM_SKETCHES_HPP
#define _FCM_SKETCHES_HPP

#include "BOBHash32.h"
#include "common.h"
#include "counter.h"
#include "pds.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <ostream>

class FCM_Sketches : public PDS {
public:
  vector<vector<vector<Counter>>> stages;
  vector<uint32_t> stages_sz;
  vector<uint32_t> stage_overflows;
  uint32_t n_stages;
  uint32_t k;
  uint32_t depth;
  vector<BOBHash32> hash;

  uint32_t hh_threshold;
  std::unordered_set<TUPLE, TupleHash> HH_candidates;
  FCM_Sketches(uint32_t n_roots, uint32_t n_stages, uint32_t k, uint32_t depth,
               uint32_t hh_threshold, uint32_t em_iters, string trace,
               uint8_t tuple_sz, uint32_t n_stage, uint32_t n_struct)
      : PDS(trace, n_stage, n_struct, tuple_sz),
        stages(DEPTH, vector<vector<Counter>>(NUM_STAGES)), stages_sz(n_stages),
        stage_overflows(n_stages), n_stages(n_stages), k(k), depth(depth),
        hash(depth), hh_threshold(hh_threshold), em_iters(em_iters) {

    if (this->depth < 1) {
      std::cout << "[ERROR] Set depth is smaller than zero, needs minimum 1"
                << std::endl;
      exit(1);
    }

    // Defaults and hash
    for (size_t d = 0; d < depth; d++) {
      this->hash[d].initialize(750 + n_struct + d);
    }
    this->columns = n_roots;

    // Calculate max count for counters and length of each stage
    // Maximum 32 bit counter
    uint32_t max_bits = 32;
    uint32_t max_count = std::numeric_limits<uint32_t>::max();
    uint32_t mem = 0;
    for (int i = n_stages - 1; i >= 0; --i) {
      mem += n_roots * max_bits;
      this->stages_sz[i] = n_roots;
      this->stage_overflows[i] = max_count;
      n_roots *= k;
      max_bits /= 2;
      max_count = max_count >> max_bits;
    }

    // Setup logging
    this->csv_header = "Average Relative Error,Average Absolute "
                       "Error,Recall,Precision,F1";
    this->csv_header_em =
        "Epoch,Estimation Time,Total Time,Weighted Mean Relative Error,"
        "Cardinality";
    this->name = "FC/FCM-Sketches";
    this->trace_name = trace;
    this->rows = n_stages;
    this->mem_sz = depth * mem / 8;
    this->store_results = false;
    // std::cout << "Total memory used: " << this->mem_sz << std::endl;
    this->setupLogging();

    // Setup stages accoording to k * n_roots over the stages
    std::cout << "[FCM Sketches] Setting up counters" << std::endl;
    for (size_t d = 0; d < depth; d++) {
      std::cout << "Depth: " << d << std::endl;
      for (size_t i = 0; i < n_stages; i++) {
        std::cout << "\tStage: " << i << " N counters: " << this->stages_sz[i]
                  << "\tMax val " << this->stage_overflows[i] << std::endl;

        this->stages[d][i].resize(this->stages_sz[i]);
        for (size_t j = 0; j < this->stages_sz[i]; j++) {
          this->stages[d][i].at(j) = Counter(this->stage_overflows[i]);
        }
      }
    }
  }

  uint32_t insert(TUPLE tuple);
  uint32_t insert(TUPLE tuple, uint32_t idx);
  uint32_t subtract(TUPLE tuple, uint32_t count);
  uint32_t hashing(TUPLE tuple, uint32_t d);
  uint32_t lookup(TUPLE tuple);
  uint32_t lookup_sketch(TUPLE tuple, uint32_t d);
  uint32_t lookup_degree(TUPLE tuple, uint32_t d);
  void reset();

  void analyze(int epoch);
  double average_absolute_error = 0.0;
  double average_relative_error = 0.0;
  double f1 = 0.0;
  double recall = 0.0;
  double precision = 0.0;
  double wmre = 0.0;
  uint32_t em_iters;
  bool estimate_fsd = true;
  double get_distribution(vector<uint32_t> &true_fsd);

  void store_data();
  void print_sketch();
};
#endif // !_FCM_SKETCHES_HPP
