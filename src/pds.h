#ifndef _PDS_H
#define _PDS_H

#include "common.h"
#include <cstdint>
#include <fstream>
#include <unordered_map>

#define BUF_SZ 1024 * 1024

class PDS {
public:
  std::unordered_map<string, uint32_t> true_data;
  string name = "NOT SET";
  string trace_name = "NOT SET";
  uint32_t mem_sz;
  uint32_t n_stage;
  uint32_t n_struct;

  // Logging members
  char filename_dat[400];
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
  virtual uint32_t insert(FIVE_TUPLE tuple) {
    std::cout << "Not implemented!" << std::endl;
    return -1;
  };
  virtual uint32_t remove(FIVE_TUPLE tuple) {
    std::cout << "Not implemented!" << std::endl;
    return -1;
  }
  virtual uint32_t lookup(FIVE_TUPLE tuple) {
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
  virtual void store_data(int epoch) {
    std::cout << "not implemented!" << std::endl;
    return;
  }
  virtual void print_sketch() { std::cout << "Not implemented!" << std::endl; }

  void setName(string in) { this->name = in; }
  virtual void setName() { this->name = "NOT SET"; }
  void setupLogging() {
    sprintf(this->filename_dat, "results/%s_%s_%i_%i_%i.dat",
            this->trace_name.c_str(), this->name.c_str(), this->n_stage,
            this->n_struct, this->mem_sz);
    sprintf(this->filename_csv, "results/%s_%s_%i_%i_%i.csv",
            this->trace_name.c_str(), this->name.c_str(), this->n_stage,
            this->n_struct, this->mem_sz);
    // Remove previous data file
    std::remove(filename_dat);
    std::remove(filename_csv);
    // Open files
    std::cout << "Removed files, opening new files..." << std::endl;
    std::cout << this->filename_dat << std::endl;

    // Setup data file with buffer
    this->fdata.open(this->filename_dat, std::ios::out | std::ios_base::app);
    this->fdata.rdbuf()->pubsetbuf(this->data_buf, BUF_SZ);
    std::cout << "Open file " << this->filename_dat << std::endl;
    // Setup csv file with buffer
    this->fcsv.open(this->filename_csv, std::ios::out);
    this->fcsv.rdbuf()->pubsetbuf(this->csv_buf, BUF_SZ);
    std::cout << "Open file " << this->filename_csv << std::endl;

    std::cout << "...opened files" << std::endl;

    this->fcsv << this->csv_header << std::endl;
  }
};

#endif // !_PDS_H
