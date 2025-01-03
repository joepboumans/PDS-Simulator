#ifndef _Q_WATERFALL_FCM_HPP
#define _Q_WATERFALL_FCM_HPP

#include "common.h"
#include "fcm-sketches.hpp"
#include "pds.h"
#include "qwaterfall.hpp"
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
class qWaterfall_Fcm : public PDS<TUPLE, HASH> {
private:
  uint32_t n_tables;
  vector<vector<uint32_t>> tables;
  FCM_Sketches<TUPLE, HASH> fcm_sketches;
  qWaterfall<TUPLE, HASH> qwaterfall;

  string trace_name;
  set<TUPLE> tuples;
  uint32_t em_iters;

public:
  qWaterfall_Fcm(uint32_t n_tables, uint32_t em_iters, string trace,
                 uint32_t n_stage, uint32_t n_struct)
      : PDS<TUPLE, HASH>(trace, n_stage, n_struct), n_tables(n_tables),
        em_iters(em_iters),
        fcm_sketches(W3, 3, 8, DEPTH, 10000, 1, trace, n_stage, n_struct),
        qwaterfall(n_tables, trace, n_stage, n_struct) {

    this->fcm_sketches.estimate_fsd = false;
    // Setup logging
    this->csv_header = "Epoch,Average Relative Error,Average Absolute "
                       "Error,Weighted Mean Relative "
                       "Error,F1 Heavy Hitter,Estimation "
                       "Time,Iterations,Insertions,Collisions,F1 Member";
    this->name = "qWaterfall_Fcm";
    this->trace_name = trace;
    this->rows = std::numeric_limits<uint16_t>::max();
    this->columns = n_tables;
    this->mem_sz = this->fcm_sketches.mem_sz + this->qwaterfall.mem_sz;
    std::cout << "Total memory used: " << this->mem_sz << std::endl;
    this->setupLogging();
  }

  uint32_t insert(TUPLE tuple);
  uint32_t hashing(TUPLE tuple, uint32_t k);
  uint32_t rehashing(uint32_t idx, uint32_t val, uint32_t k);
  uint32_t lookup(TUPLE tuple);
  void reset();

  void analyze(int epoch);
  bool estimate_fsd = true;
  double f1_member = 0.0;
  uint32_t insertions = 0;
  uint32_t collisions = 0;
  double load_factor = 0.0;
  double wmre = 0.0;
  double average_absolute_error = 0.0;
  double average_relative_error = 0.0;
  double f1_hh = 0.0;

  void store_data();
  void print_sketch();
  vector<double> get_distribution(set<TUPLE> tuples);
  void set_estimate_fsd(bool onoff);
};

#endif // !_Q_WATERFALL_FCM_HPP
