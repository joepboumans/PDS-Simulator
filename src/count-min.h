#ifndef _COUNT_MIN_H
#define _COUNT_MIN_H

// #include "BOBHash32.h"
#include "common.h"
#include "counter.h"
#include "pds.h"
#include <cstdint>
#include <limits>
#include <sys/types.h>
#include <vector>

class CountMin : public PDS {
public:
  vector<vector<Counter>> counters;
  uint32_t row;
  uint32_t columns;
  uint32_t n_hash;
  string trace_name;
  CountMin(uint32_t row, uint32_t columns, uint32_t k, string trace,
           uint32_t n_stage, uint32_t n_struct)
      : PDS{trace, n_stage, n_struct},
        counters(row,
                 vector<Counter>(
                     columns, Counter(std::numeric_limits<uint32_t>::max()))) {
    // Assign defaults
    this->row = row;
    this->columns = columns;
    this->n_hash = k;
    this->trace_name = trace;
  }
};

#endif // !_COUNT_MIN_H
#define _COUNT_MIN_H
