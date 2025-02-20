#ifndef _DATA_PARSER_H_
#define _DATA_PARSER_H_

#include "common.h"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

class dataParser {
public:
  unordered_map<string, TRACE> get_traces(vector<string> filenames,
                                          uint8_t tuple_sz) {
    vector<TRACE> traces(filenames.size());
    unordered_map<string, TRACE> data_traces;

    string dir = "data/";
    string dat = ".dat";
    size_t i = 0;

    for (string &f : filenames) {
      std::cout << f << std::endl;
      traces[i] = this->get_trace(f.data(), tuple_sz);

      string::size_type k = f.find(dir);
      if (k != string::npos) {
        f.erase(k, dir.length());
      }
      string::size_type j = f.find(dat);
      if (j != string::npos) {
        f.erase(j, dat.length());
      }
      data_traces[f] = traces[i++];
    }
    return data_traces;
  }

  TRACE get_trace(char *filename, uint8_t tuple_sz) {
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
    // Load filename
    sprintf(this->filename, "%s", filename);

    // Verify if the trace is present or needs to be downloaded
    if (!this->check_for_traces()) {
      std::cout << "Cannot find dataset " << this->filename << std::endl;
      exit(1);
    }

    std::cout << "Found file, opening..." << std::endl;
    this->fin = fopen(this->filename, "rb");
    assert(this->fin);

    std::cout << "Opened, finding size..." << std::endl;
    fseek(this->fin, 0L, SEEK_END);
    uint32_t sz = ftell(this->fin);
    fseek(this->fin, 0L, SEEK_SET);
    std::cout << "Total size of dataset " << this->filename << ": " << sz
              << std::endl;

    TRACE trace(sz / MAX_TUPLE_SZ);
    // Create 5-tuple and trace, dataset is always a 5 tuple set
    TUPLE tin;
    // Load trace with 5-tuple
    if (this->fin == NULL) {
      std::cout << "ERROR: Input file is NULL" << std::endl;
    }
    uint32_t i = 0;
    std::set<TUPLE> unique_tuples;
    while (fread(&tin.num_array, 1, MAX_TUPLE_SZ, this->fin)) {
      trace[i] = tin;
      trace[i].sz = tuple_sz;
      unique_tuples.insert(trace[i]);
      i++;
    }

    std::cout << "Total tuples " << trace.size()
              << "\tUnique tuples in trace : " << unique_tuples.size()
              << " tuples" << std::endl;

    this->write_ns2dat(trace);
    return trace;
  }

private:
  char filename[100];
  string tuple_name;
  FILE *fin;

  uint32_t check_for_traces() {
    struct stat buffer;
    return (stat(this->filename, &buffer) == 0);
  }

  void write_ns2dat(TRACE trace) {
    string name_dir = "results/" + this->tuple_name + "/TrueData";
    if (!std::filesystem::is_directory(name_dir)) {
      std::filesystem::create_directories(name_dir);
    }

    string filename = this->filename;
    std::cout << filename << std::endl;
    string file = filename.erase(filename.find("data/"), sizeof("data/") - 1);
    std::cout << file << std::endl;
    char ns_filename[300];
    sprintf(ns_filename, "%s/ns_%s", name_dir.c_str(), file.c_str());
    std::remove(ns_filename);
    std::ofstream fcsv_ns;
    fcsv_ns.open(ns_filename, std::ios::out | std::ios::binary);

    std::unordered_map<TUPLE, uint32_t, TupleHash> true_data;
    for (auto &tuple : trace) {
      true_data[tuple]++;
    }

    using pair_type = decltype(true_data)::value_type;
    auto max_count =
        std::max_element(true_data.begin(), true_data.end(),
                         [](const pair_type &p1, const pair_type &p2) {
                           return p1.second < p2.second;
                         });

    vector<double> true_fsd(max_count->second + 1);
    std::cout << "[Data Parser] Max true count is " << max_count->second
              << std::endl;

    for (const auto &[tuple, count] : true_data) {
      true_fsd[count]++;
    }

    uint32_t num_entries = 0;
    for (uint32_t i = 0; i < true_fsd.size(); i++) {
      if (true_fsd[i] != 0) {
        num_entries++;
      }
    }

    std::cout << "[Data Parser] Writing " << num_entries << " entries to "
              << ns_filename << std::endl;
    // Write NS FSD size and then the FSD as uint64_t
    fcsv_ns.write((char *)&num_entries, sizeof(num_entries));
    for (uint32_t i = 0; i < true_fsd.size(); i++) {
      if (true_fsd[i] != 0) {
        fcsv_ns.write((char *)&i, sizeof(i));
        fcsv_ns.write((char *)&true_fsd[i], sizeof(true_fsd[i]));
      }
    }
  }
};
#endif
