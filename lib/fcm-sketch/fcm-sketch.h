#ifndef _FCM_SKETCH_H
#define _FCM_SKETCH_H

#include "../../src/BOBHash32.h"
#include "../../src/common.h"
#include "../../src/counter.h"
#include "../../src/pds.h"
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <ostream>

class FCM_Sketch : public PDS {
private:
  vector<vector<Counter>> stages;
  vector<uint32_t> stages_sz;
  uint32_t n_stages;
  uint32_t k;
  BOBHash32 hash;

  uint32_t hh_threshold;
  std::set<string> HH_candidates;

public:
  FCM_Sketch(uint32_t n_roots, uint32_t n_stages, uint32_t k,
             uint32_t hh_threshold, string trace, uint32_t n_stage,
             uint32_t n_struct)
      : PDS(trace, n_stage, n_struct), stages(n_stages), stages_sz(n_stages) {

    // Defaultss and hash
    this->hash.initialize(750 + n_struct);
    this->k = k;
    this->n_stages = n_stages;
    this->hh_threshold = hh_threshold;

    // Calculate max count for counters and length of each stage
    // Maximum 32 bit counter
    uint32_t max_bits = 32;
    uint32_t max_count = std::numeric_limits<uint32_t>::max();
    uint32_t max_counter[n_stages];
    uint32_t mem = 0;
    for (int i = n_stages - 1; i >= 0; --i) {
      mem += n_roots * max_bits;
      this->stages_sz[i] = n_roots;
      max_counter[i] = max_count;
      n_roots *= k;
      max_count = max_count >> max_bits;
      max_bits /= 2;
    }

    // Setup stages accoording to k * n_roots over the stages
    for (size_t i = 0; i < n_stages; i++) {
      for (size_t j = 0; j < this->stages_sz[i]; j++) {
        this->stages[i].push_back(Counter(max_counter[i]));
      }
      std::cout << this->stages[i][0].count << std::endl;
    }

    // Setup logging
    this->csv_header = "epoch,Average Relative Error,Average Absolute "
                       "Error,Weighted Mean Relative Error,Recall,Precision,F1";
    this->name = "FCM-Sketch";
    this->trace_name = trace;
    this->columns = n_roots;
    this->rows = n_stages;
    this->mem_sz = mem / 8;
    // std::cout << "Total memory used: " << this->mem_sz << std::endl;
    this->setupLogging();
  }

  uint32_t insert(FIVE_TUPLE tuple);
  uint32_t hashing(FIVE_TUPLE tuple, uint32_t k);
  void reset();

  void analyze(int epoch);
  double average_absolute_error = 0.0;
  double average_relative_error = 0.0;
  double f1 = 0.0;
  double recall = 0.0;
  double precision = 0.0;
  double wmre = 0.0;

  void store_data();
  void print_sketch();
};

#endif // !_FCM_SKETCH_H
