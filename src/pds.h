#ifndef _PDS_H
#define _PDS_H

#include "common.h"
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <unordered_map>

class PDS {
public:
  std::unordered_map<TUPLE, uint32_t, TupleHash> true_data;
  string name;
  string trace_name;
  string tuple_name;
  uint32_t mem_sz;
  uint32_t rows;
  uint32_t columns;
  uint32_t n_stage;
  uint32_t n_struct;

  // Logging members
  bool store_results = true;
  char filename_csv[400];
  char filename_csv_em[400];
  char filename_csv_ns[400];
  std::ofstream fcsv;
  std::ofstream fcsv_em;
  std::ofstream fcsv_ns;
  string csv_header;
  string csv_header_em;
  string csv_header_ns;

  PDS(string trace, uint32_t stage, uint32_t n, uint8_t tuple_sz) {
    this->trace_name = trace;
    this->n_stage = stage;
    this->n_struct = n;
    switch (tuple_sz) {
    case TupleSize::FiveTuple:
      this->tuple_name = "FiveTuple";
      break;
    case TupleSize::FlowTuple:
      this->tuple_name = "FlowTuple";
      break;
    case TupleSize::SrcTuple:
      this->tuple_name = "SrcTuple";
      break;
    default:
      std::cout << "[ERROR] NO VALID TUPLE SZ" << std::endl;
      exit(1);
    }
  }

  PDS(string trace, uint32_t stage, uint32_t n, uint32_t sz, uint8_t tuple_sz) {
    this->trace_name = trace;
    this->n_stage = stage;
    this->n_struct = n;
    this->mem_sz = sz;
    switch (tuple_sz) {
    case TupleSize::FiveTuple:
      this->tuple_name = "FiveTuple";
      break;
    case TupleSize::FlowTuple:
      this->tuple_name = "FlowTuple";
      break;
    case TupleSize::SrcTuple:
      this->tuple_name = "SrcTuple";
      break;
    default:
      std::cout << "[ERROR] NO VALID TUPLE SZ" << std::endl;
      exit(1);
    }
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
  virtual void setName() { this->name = ""; }

  void setupLogging() {
    // Do not setup file writers if setting up logging
    if (!this->store_results) {
      return;
    }

    if (this->tuple_name.empty() or this->name.empty() or
        this->trace_name.empty()) {
      std::cerr << "[ERROR] Name or tuple name is empty of PDS: " << std::endl;
      std::cerr << "Tuple name : " << this->tuple_name << std::endl;
      std::cerr << "Name : " << this->name << std::endl;
      exit(1);
    }

    string name_dir = "results/" + this->tuple_name + "/" + this->name;
    if (!std::filesystem::is_directory(name_dir)) {
      std::filesystem::create_directories(name_dir);
    }

    if (this->rows == 0) {
      sprintf(this->filename_csv, "%s/%s_%i.csv", name_dir.c_str(),
              this->trace_name.c_str(), this->mem_sz);
    } else {
      sprintf(this->filename_csv, "%s/%s_%i_%i_%i.csv", name_dir.c_str(),
              this->trace_name.c_str(), this->rows, this->columns,
              this->mem_sz);
    }

    // Remove previous csv file and setup csv file
    std::remove(this->filename_csv);
    this->fcsv.open(this->filename_csv, std::ios::out);
    this->fcsv << this->csv_header << std::endl;

    if (this->csv_header_em.empty()) {
      std::cout << "Empty CSV header, not printing any EM stats for "
                << this->name << std::endl;
      return;
    }

    std::cout << "Found header for EM, printing stat for " << this->name
              << std::endl;
    sprintf(this->filename_csv_em, "%s/em_%s_%i_%i_%i.csv", name_dir.c_str(),
            this->trace_name.c_str(), this->rows, this->columns, this->mem_sz);

    std::remove(this->filename_csv_em);
    this->fcsv_em.open(this->filename_csv_em, std::ios::out);
    this->fcsv_em << this->csv_header_em << std::endl;

    sprintf(this->filename_csv_ns, "%s/ns_%s_%i_%i_%i.dat", name_dir.c_str(),
            this->trace_name.c_str(), this->rows, this->columns, this->mem_sz);

    std::remove(this->filename_csv_ns);
    this->fcsv_ns.open(this->filename_csv_ns, std::ios::out | std::ios::binary);
  }
};

#endif // !_PDS_H
