#ifndef _DATA_PARSER_H_
#define _DATA_PARSER_H_

#include "common.h"
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

class dataParser {
public:
  std::set<FIVE_TUPLE> unique_tuples;
  uint32_t get_traces(char *filename, TRACE &trace) {
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
    std::cout << "Found end" << std::endl;
    uint32_t sz = ftell(this->fin);
    fseek(this->fin, 0L, SEEK_SET);
    std::cout << "Total size of dataset " << this->filename << ": " << sz
              << std::endl;

    trace.resize(sz / sizeof(FIVE_TUPLE));
    std::cout << "Total tuples in trace : " << trace.size() << " tuples"
              << std::endl;
    // Create 5-tuple and trace
    FIVE_TUPLE tin;
    // Load trace with 5-tuple
    if (this->fin == NULL) {
      std::cout << "ERROR: Input file is NULL" << std::endl;
    }
    uint32_t i = 0;
    while (fread(&tin, 1, sizeof(FIVE_TUPLE), this->fin)) {
      trace[i++] = tin;
      unique_tuples.insert(tin);
    }

    std::cout << "Unique tuples in trace : " << unique_tuples.size()
              << " tuples" << std::endl;

    unique_tuples.clear();
    return 0;
  }

private:
  char filename[100];
  FILE *fin;

  uint32_t check_for_traces() {
    struct stat buffer;
    return (stat(this->filename, &buffer) == 0);
  }
};
#endif
