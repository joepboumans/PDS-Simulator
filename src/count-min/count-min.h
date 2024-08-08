#ifndef _COUNT_MIN_H
#define _COUNT_MIN_H

#include "../BOBHash32.h"
#include "../common.h"
#include "../counter.h"
#include "../pds.h"
#include <cstdint>
#include <limits>
#include <sys/types.h>
#include <vector>

class CountMin : public PDS {
public:
  BOBHash32 *hash;
  vector<vector<Counter>> counters;
  uint32_t row;
  uint32_t columns;
  uint32_t n_hash;
  string trace_name;
  CountMin(uint32_t row, uint32_t columns, string trace, uint32_t n_stage,
           uint32_t n_struct)
      : PDS{trace, n_stage, n_struct},
        counters(row,
                 vector<Counter>(
                     columns, Counter(std::numeric_limits<uint32_t>::max()))) {
    // Assign defaults
    this->columns = columns;
    this->n_hash = row;
    this->trace_name = trace;
    this->mem_sz = row * columns * sizeof(uint32_t);

    // Setup logging
    this->csv_header = "epoch,Average Relative Error,Average Absolute "
                       "Error,Weighted Mean Relative Error,Recall,Precision,F1";
    this->name = "CountMin";
    this->setupLogging();

    // Setup Hashing
    this->hash = new BOBHash32[this->n_hash];
    for (size_t i = 0; i < this->n_hash; i++) {
      this->hash[i].initialize(n_struct * this->n_hash + i);
    }
  }
  uint32_t insert(FIVE_TUPLE tuple);
  uint32_t lookup(FIVE_TUPLE tuple);
  uint32_t hashing(FIVE_TUPLE tuple, uint32_t k);
  void reset();
  void analyze(int epoch);
  void store_data();
  void print_sketch();
};

#endif // !_COUNT_MIN_H
#define _COUNT_MIN_H
