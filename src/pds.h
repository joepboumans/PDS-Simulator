#ifndef _PDS_H
#define _PDS_H

#include "common.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <unordered_map>

#define BUF_SZ 1024 * 1024

class PDS {
public:
  std::unordered_map<TUPLE, uint32_t, TupleHash> true_data;
  string name = "NOT SET";
  string trace_name = "NOT SET";
  uint32_t mem_sz;
  uint32_t rows;
  uint32_t columns;
  uint32_t n_stage;
  uint32_t n_struct;

  // Logging members
  char filename_csv[400];
  std::ofstream fdata;
  std::ofstream fcsv;
  char data_buf[BUF_SZ];
  char csv_buf[BUF_SZ];
  string csv_header = "CSV HEADER IS NOT SET";

  PDS(string trace, uint32_t stage, uint32_t n) {
    this->trace_name = trace;
    this->n_stage = stage;
    this->n_struct = n;
  }
  PDS(string trace, uint32_t stage, uint32_t n, uint32_t sz) {
    this->trace_name = trace;
    this->n_stage = stage;
    this->n_struct = n;
    this->mem_sz = sz;
  }
  ~PDS() = default;
  virtual uint32_t insert(TUPLE tuple) {
    std::cout << "Not implemented!" << std::endl;
    return -1;
  };
  virtual uint32_t remove(TUPLE tuple) {
    std::cout << "Not implemented!" << std::endl;
    return -1;
  }
  virtual uint32_t lookup(TUPLE tuple) {
    std::cout << "Not implemented!" << std::endl;
    return -1;
  }
  virtual void reset() {
    std::cout << "Not implemented!" << std::endl;
    return;
  }
  virtual void analyze(int epoch) {
    std::cout << "Not implemented!" << std::endl;
    return;
  }
  virtual void print_sketch() { std::cout << "Not implemented!" << std::endl; }

  void setName(string in) { this->name = in; }
  virtual void setName() { this->name = "NOT SET"; }

  void setupLogging() {
    string name_dir = "results/flow_tuple" + this->name;
    if (!std::filesystem::is_directory(name_dir)) {
      std::filesystem::create_directory(name_dir);
    }
    if (this->rows == 0) {
      sprintf(this->filename_csv, "results/%s/%s_%i_%i_%i.csv",
              this->name.c_str(), this->trace_name.c_str(), this->n_stage,
              this->n_struct, this->mem_sz);
    } else {
      sprintf(this->filename_csv, "results/%s/%s_%i_%i_%i_%i_%i.csv",
              this->name.c_str(), this->trace_name.c_str(), this->n_stage,
              this->n_struct, this->rows, this->columns, this->mem_sz);
    }
    // Remove previous csv file
    std::remove(filename_csv);

    // Setup csv file with buffer
    this->fcsv.open(this->filename_csv, std::ios::out);
    this->fcsv.rdbuf()->pubsetbuf(this->csv_buf, BUF_SZ);

    this->fcsv << this->csv_header << std::endl;
  }
};

#endif // !_PDS_H
