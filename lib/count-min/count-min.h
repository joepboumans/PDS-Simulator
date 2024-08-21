#ifndef _COUNT_MIN_H
#define _COUNT_MIN_H

#include "../../src/BOBHash32.h"
#include "../../src/common.h"
#include "../../src/counter.h"
#include "../../src/pds.h"
#include <cstdint>
#include <limits>
#include <set>
#include <sys/types.h>

class CountMin : public PDS {
public:
  vector<vector<Counter>> counters;
  BOBHash32 *hash;
  uint32_t n_hash;
  string trace_name;

  uint32_t hh_threshold;
  std::set<string> HH_candidates;

  CountMin(uint32_t rows, uint32_t columns, uint32_t hh_threshold, string trace,
           uint32_t n_stage, uint32_t n_struct)
      : PDS{trace, n_stage, n_struct},
        counters(rows,
                 vector<Counter>(
                     columns, Counter(std::numeric_limits<uint32_t>::max()))) {
    // Assign defaults
    this->columns = columns;
    this->n_hash = rows;
    this->hh_threshold = hh_threshold;

    // Setup logging
    this->csv_header = "epoch,Average Relative Error,Average Absolute "
                       "Error,Weighted Mean Relative Error,Recall,Precision,F1";
    this->name = "CountMin";
    this->trace_name = trace;
    this->columns = columns;
    this->rows = rows;
    this->mem_sz = rows * columns * sizeof(uint32_t);
    this->setupLogging();

    // Setup Hashing
    this->hash = new BOBHash32[this->n_hash];
    for (size_t i = 0; i < this->n_hash; i++) {
      this->hash[i].initialize(750 + n_struct * this->n_hash + i);
    }
  }

  uint32_t insert(FIVE_TUPLE tuple);
  uint32_t lookup(FIVE_TUPLE tuple);
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

#endif // !_COUNT_MIN_H
#define _COUNT_MIN_H
