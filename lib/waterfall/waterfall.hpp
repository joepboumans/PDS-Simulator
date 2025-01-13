#ifndef _WATERFALL_H
#define _WATERFALL_H

#include "BOBHash32.h"
#include "common.h"
#include "pds.h"
#include <cstdint>
#include <sys/types.h>

class Waterfall : public PDS {
public:
  BOBHash32 *hash;
  uint32_t n_tables;
  uint32_t length;
  vector<vector<TUPLE>> tables;
  set<TUPLE> tuples;
  uint32_t insertions = 0;
  string trace_name;

  Waterfall(uint32_t n_tables, uint32_t length, string trace, uint8_t tuple_sz,
            uint32_t n_stage, uint32_t n_struct)
      : PDS(trace, n_stage, n_struct, tuple_sz),
        tables(n_tables, vector<TUPLE>(length)) {
    // Assign defaults
    this->n_tables = n_tables;
    this->length = length;
    this->trace_name = trace;
    this->mem_sz = length * n_tables * 13;

    // Setup logging
    this->csv_header = "Epoch,Insertions,Recall,Precision,F1";
    this->name = "Waterfall";
    this->rows = n_tables;
    this->columns = length;
    this->setupLogging();

    // Setup Hashing
    this->hash = new BOBHash32[this->n_tables];
    for (size_t i = 0; i < this->n_tables; i++) {
      this->hash[i].initialize(750 + n_struct * this->n_tables + i);
    }
  }

  uint32_t insert(TUPLE tuple);
  uint32_t lookup(TUPLE tuple);
  uint32_t hashing(TUPLE tuple, uint32_t k);
  void reset();

  void analyze(int epoch);
  double f1 = 0.0;
  double recall = 0.0;
  double precision = 0.0;
  double load_factor = 0.0;

  void store_data();
  void print_sketch();
};

#endif // !_WATERFALL_H
