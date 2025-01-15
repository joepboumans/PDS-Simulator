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

// Waterfall/Cuckoo tables, but uses quotient for indexing and remained as
// compare value
class qWaterfall_Fcm : public PDS {
private:
  uint32_t n_tables;
  vector<vector<uint32_t>> tables;
  FCM_Sketches fcm_sketches;
  qWaterfall qwaterfall;

  string trace_name;
  set<TUPLE> tuples;
  uint32_t em_iters;

public:
  qWaterfall_Fcm(uint32_t n_tables, uint32_t em_iters, string trace,
                 uint8_t tuple_sz, uint32_t n_stage, uint32_t n_struct)
      : PDS(trace, n_stage, n_struct, tuple_sz), n_tables(n_tables),
        fcm_sketches(W3, 3, 8, DEPTH, 100000, 1, trace, tuple_sz, n_stage,
                     n_struct),
        qwaterfall(n_tables, trace, tuple_sz, n_stage, n_struct),
        em_iters(em_iters) {

    this->fcm_sketches.estimate_fsd = false;
    // Setup logging
    this->csv_header_em =
        "Epoch,Epoch Time,Total Time, Weighted Mean Relative Error, "
        "Cardinality";
    this->csv_header =
        "Average Relative Error,Average Absolute "
        "Error, F1 Heavy Hitter, Iterations, Insertions, Collisions, F1 Member";
    this->name = "qWaterfall_FCM";
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
  double calculate_fsd(set<TUPLE> &tuples, vector<uint32_t> &true_fsd);
  double calculate_fsd_org(set<TUPLE> &tuples, vector<uint32_t> &true_fsd);
  double calculate_fsd_peeling(set<TUPLE> &tuples, vector<uint32_t> &true_fsd);
  vector<uint32_t> peel_sketches(vector<vector<uint32_t>> counts,
                                 vector<vector<vector<TUPLE>>> coll_tuples);
  void set_estimate_fsd(bool onoff);
};

#endif // !_Q_WATERFALL_FCM_HPP
