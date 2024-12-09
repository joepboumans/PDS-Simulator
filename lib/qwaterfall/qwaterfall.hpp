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
class qWaterfall : public PDS {
private:
  BOBHash32 *hash;
  vector<vector<uint32_t>> tables;
  uint32_t n_tables;
  uint32_t table_length;

  string trace_name;
  set<FIVE_TUPLE> tuples;

public:
  qWaterfall(uint32_t n_tables, uint32_t table_length, string trace,
             uint32_t n_stage, uint32_t n_struct)
      : PDS(trace, n_stage, n_struct), n_tables(n_tables),
        table_length(table_length) {

    // Setup logging
    this->csv_header = "Epoch, Recall, Precision, F1, Insetions, Collisions";
    this->name = "qWaterfall";
    this->trace_name = trace;
    this->mem_sz = this->n_tables * this->table_length;
    std::cout << "Total memory used: " << this->mem_sz << std::endl;
    this->setupLogging();
  }

  uint32_t insert(FIVE_TUPLE tuple);
  uint32_t hashing(FIVE_TUPLE tuple, uint32_t k);
  uint32_t rehashing(uint32_t hashed_val, uint32_t k);
  uint32_t lookup(FIVE_TUPLE tuple);
  void reset();

  void analyze(int epoch);
  double recall = 0.0;
  double precision = 0.0;
  double f1 = 0.0;
  uint32_t insertions = 0;
  uint32_t collisions = 0;

  void store_data();
  void print_sketch();
  vector<double> get_distribution(set<FIVE_TUPLE> tuples);
  void set_estimate_fsd(bool onoff);
};

#endif // !_Q_WATERFALL_HPP
