#ifndef _DATA_PARSER_H_
#define _DATA_PARSER_H_

#include "common.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

template <typename TUPLE, typename T> class dataParser {
public:
  size_t n_unique_tuples = 0;
  unordered_map<string, T> get_traces(vector<string> filenames) {
    vector<T> traces(filenames.size());
    unordered_map<string, T> data_traces;

    string dir = "data/";
    string dat = ".dat";
    size_t i = 0;

    for (string &f : filenames) {
      std::cout << f << std::endl;
      traces[i] = this->get_trace(f.data());

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

  T get_trace(char *filename) {
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

    T trace(sz / sizeof(TUPLE));
    // Create 5-tuple and trace, dataset is always a 5 tuple set
    FIVE_TUPLE tin;
    // Load trace with 5-tuple
    if (this->fin == NULL) {
      std::cout << "ERROR: Input file is NULL" << std::endl;
    }
    uint32_t i = 0;
    while (fread(&tin, 1, sizeof(FIVE_TUPLE), this->fin)) {
      trace[i++] = TUPLE(tin);
      unique_tuples.insert(tin);
    }

    n_unique_tuples = unique_tuples.size();
    std::cout << "Total tuples " << trace.size()
              << "\tUnique tuples in trace : " << unique_tuples.size()
              << " tuples" << std::endl;
    unique_tuples.clear();

    return trace;
  }

private:
  std::set<TUPLE> unique_tuples;
  char filename[100];
  FILE *fin;

  uint32_t check_for_traces() {
    struct stat buffer;
    return (stat(this->filename, &buffer) == 0);
  }
};
#endif
