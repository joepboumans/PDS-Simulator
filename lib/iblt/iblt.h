#ifndef _IBLT_H
#define _IBLT_H

#include "BOBHash32.h"
#include "common.h"
#include "pds.h"
#include <cstdint>
#include <sys/types.h>

template <typename TUPLE> struct IBLT_entry {
  uint32_t count;
  TUPLE tupleXOR;
  uint32_t hashXOR;
};

template <typename TUPLE, typename HASH> class IBLT : public PDS<TUPLE, HASH> {
public:
  BOBHash32 *hash;
  uint32_t n_hash;
  uint32_t length;
  string trace_name;
  vector<IBLT_entry<TUPLE>> table;

  IBLT(uint32_t length, string trace, uint32_t k, uint32_t n_stage,
       uint32_t n_struct)
      : PDS<TUPLE, HASH>{trace, n_stage, n_struct}, table(length) {
    // Assign defaults
    this->n_hash = k;
    this->length = length;
    this->trace_name = trace;
    // 4B Count + 13B tuple XOR + 4B Hash XOR
    this->mem_sz = 21 * length;

    // Setup logging
    this->csv_header =
        "Epoch,Total uniques,Total found,FP,FN,Recall,Precision,F1";
    this->name = "IBLT";
    this->setupLogging();

    // Setup Hashing
    this->hash = new BOBHash32[this->n_hash];
    for (size_t i = 0; i < this->n_hash; i++) {
      this->hash[i].initialize(750 + n_struct * this->n_hash + i);
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

  void store_data();
  void print_sketch();
};

#endif // !_IBLT_H
#define _IBLT_H
