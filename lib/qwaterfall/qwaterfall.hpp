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
  uint32_t n_tables;
  uint32_t table_length;
  vector<vector<uint32_t>> tables;

  string trace_name;
  set<FIVE_TUPLE> tuples;

public:
  qWaterfall(uint32_t n_tables, uint32_t table_length, string trace,
             uint32_t n_stage, uint32_t n_struct)
      : PDS(trace, n_stage, n_struct), n_tables(n_tables),
        table_length(table_length),
        tables(n_tables, vector<uint32_t>(table_length)) {

    // Setup Hashing
    this->hash = new BOBHash32[this->n_tables];
    for (size_t i = 0; i < this->n_tables; i++) {
      this->hash[i].initialize(750 + n_struct * this->n_tables + i);
    }

    // Setup logging
    this->csv_header = "Epoch,Insertions,Collisions,Recall,Precision,F1";
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

// Waterfall/Cuckoo tables, but uses quotient for indexing and remained as
// compare value, using FlowTuple
class qWaterfall_FT : public PDS_FlowTuple {
private:
  BOBHash32 *hash;
  uint32_t n_tables;
  uint32_t table_length;
  vector<vector<uint32_t>> tables;

  string trace_name;
  set<FLOW_TUPLE> tuples;

public:
  qWaterfall_FT(uint32_t n_tables, uint32_t table_length, string trace,
                uint32_t n_stage, uint32_t n_struct)
      : PDS_FlowTuple(trace, n_stage, n_struct), n_tables(n_tables),
        table_length(table_length),
        tables(n_tables, vector<uint32_t>(table_length)) {

    // Setup Hashing
    this->hash = new BOBHash32[this->n_tables];
    for (size_t i = 0; i < this->n_tables; i++) {
      this->hash[i].initialize(750 + n_struct * this->n_tables + i);
    }

    // Setup logging
    this->csv_header = "Epoch,Insertions,Collisions,Recall,Precision,F1";
    this->name = "qWaterfall FT";
    this->trace_name = trace;
    this->mem_sz = this->n_tables * this->table_length;
    std::cout << "Total memory used: " << this->mem_sz << std::endl;
    this->setupLogging();
  }

  uint32_t insert(FLOW_TUPLE tuple);
  uint32_t hashing(FLOW_TUPLE tuple, uint32_t k);
  uint32_t rehashing(uint32_t hashed_val, uint32_t k);
  uint32_t lookup(FLOW_TUPLE tuple);
  void reset();

  void analyze(int epoch);
  double recall = 0.0;
  double precision = 0.0;
  double f1 = 0.0;
  uint32_t insertions = 0;
  uint32_t collisions = 0;

  void store_data();
  void print_sketch();
  vector<double> get_distribution(set<FLOW_TUPLE> tuples);
  void set_estimate_fsd(bool onoff);
};
#endif // !_Q_WATERFALL_HPP
