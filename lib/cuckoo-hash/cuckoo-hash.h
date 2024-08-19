#ifndef _CUCKOO_HASH_H
#define _CUCKOO_HASH_H

#include "../../src/BOBHash32.h"
#include "../../src/common.h"
#include "../../src/pds.h"
#include <cstdint>
#include <sys/types.h>

class CuckooHash : public PDS {
public:
  BOBHash32 *hash;
  uint32_t length;
  uint32_t n_tables;
  vector<vector<FIVE_TUPLE>> tables;
  string trace_name;
  set<string> tuples;

  CuckooHash(uint32_t n_tables, uint32_t length, string trace, uint32_t n_stage,
             uint32_t n_struct)
      : PDS{trace, n_stage, n_struct},
        tables(n_tables, vector<FIVE_TUPLE>(length)) {
    // Assign defaults
    this->n_tables = n_tables;
    this->length = length;
    this->trace_name = trace;
    this->mem_sz = length * n_tables * sizeof(uint32_t);

    // Setup logging
    this->csv_header = "epoch,Insertions,Recall,Precision,F1";
    this->name = "CuckooHash";
    this->setupLogging();

    // Setup Hashing
    this->hash = new BOBHash32[this->n_tables];
    for (size_t i = 0; i < this->n_tables; i++) {
      this->hash[i].initialize(750 + n_struct * this->n_tables + i);
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
  uint32_t insertions = 0;

  void store_data();
  void print_sketch();
};

#endif // !_CUCKOO_HASH_H
