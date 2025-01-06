#ifndef _Q_WATERFALL_HPP
#define _Q_WATERFALL_HPP

#include "common.h"
#include "pds.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <ostream>
#include <sys/types.h>
#include <vector>

// Waterfall/Cuckoo tables, but uses quotient for indexing and remained as
// compare value
template <typename TUPLE, typename HASH>
class qWaterfall : public PDS<TUPLE, HASH> {
private:
  BOBHash32 *hash;
  uint32_t n_tables;
  vector<vector<uint16_t>> tables;

  uint32_t table_length;
  string trace_name;

public:
  set<TUPLE> tuples;
  qWaterfall(uint32_t n_tables, string trace, uint32_t n_stage,
             uint32_t n_struct)
      : PDS<TUPLE, HASH>(trace, n_stage, n_struct), n_tables(n_tables) {

    // Setup Tables
    this->table_length = std::numeric_limits<uint16_t>::max();
    this->tables.resize(n_tables);
    for (auto &table : this->tables) {
      table.resize(this->table_length);
    }
    // Setup Hashing
    this->hash = new BOBHash32[this->n_tables * 2];
    for (size_t i = 0; i < this->n_tables * 2; i++) {
      this->hash[i].initialize(750 + n_struct * this->n_tables * 2 + i);
    }

    // Setup logging
    this->csv_header = "Epoch,Insertions,Collisions,Recall,Precision,F1";
    this->name = "qWaterfall";
    this->trace_name = trace;
    this->rows = this->table_length;
    this->columns = this->n_tables;
    this->mem_sz = this->n_tables * this->table_length;
    std::cout << "Total memory used: " << this->mem_sz << std::endl;
    this->setupLogging();
  }

  uint32_t insert(TUPLE tuple);
  uint32_t hashing(TUPLE tuple, uint32_t k);
  uint32_t rehashing(uint32_t hashed_val, uint32_t k);
  uint32_t lookup(TUPLE tuple);
  void reset();

  void analyze(int epoch);
  double recall = 0.0;
  double precision = 0.0;
  double f1 = 0.0;
  uint32_t insertions = 0;
  uint32_t collisions = 0;
  double load_factor = 0.0;

  void store_data();
  void print_sketch();
};

#endif // !_Q_WATERFALL_HPP
