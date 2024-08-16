#ifndef _IBLT_H
#define _IBLT_H

#include "../../src/BOBHash32.h"
#include "../../src/common.h"
#include "../../src/counter.h"
#include "../../src/pds.h"
#include <cstdint>
#include <limits>
#include <set>
#include <sys/types.h>

class IBLT : public PDS {
public:
  BOBHash32 *hash;
  uint32_t n_hash;
  string trace_name;

  IBLT(uint32_t row, uint32_t columns, uint32_t hh_threshold, string trace,
       uint32_t n_stage, uint32_t n_struct)
      : PDS{trace, n_stage, n_struct} {
    // Assign defaults
    this->trace_name = trace;
    this->mem_sz = row * columns * sizeof(uint32_t);

    // Setup logging
    this->csv_header = "epoch,Average Relative Error,Average Absolute "
                       "Error,Weighted Mean Relative Error,Recall,Precision,F1";
    this->name = "IBLT";
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
  double f1 = 0.0;
  double recall = 0.0;
  double precision = 0.0;

  void store_data();
  void print_sketch();
};

#endif // !_IBLT_H
#define _IBLT_H
