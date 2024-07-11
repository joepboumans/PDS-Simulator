#ifndef _DATA_PARSER_H_
#define _DATA_PARSER_H_

#include "common.h"
#include <bitset>
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

class dataParser {
public:
  uint32_t get_traces(char *filename, TRACE &trace) {
    // Load filename
    sprintf(this->filename, "%s", filename);

    // Verify if the trace is present or needs to be downloaded
    if (!this->check_for_traces()) {
      int result = this->download_data_traces();
      if (result != 0) {
        return result;
      }
    }

    std::cout << "Found file, opening..." << std::endl;
    this->fin = fopen(this->filename, "rb");
    assert(this->fin);

    std::cout << "Opened, finding size..." << std::endl;
    fseek(this->fin, 0L, SEEK_END);
    std::cout << "Found end" << std::endl;
    uint32_t sz = ftell(this->fin);
    fseek(this->fin, 0L, SEEK_SET);
    std::cout << "Size of trace " << this->filename << ": " << sz << std::endl;

    // Create 5-tuple and trace
    FIVE_TUPLE tin;
    // Load trace with 5-tuple
    if (this->fin == NULL) {
      std::cout << "ERROR: Input file is NULL" << std::endl;
    }
    while (fread(&tin, 1, sizeof(FIVE_TUPLE), this->fin)) {
      trace.push_back(tin);
    }

    return 0;
  }

private:
  char filename[100];
  char path_to_file[200];
  FILE *fin;

  uint32_t download_data_traces() {
    char command[200];
    sprintf(command, "%s %s%s",
            "wget https://www.dropbox.com/s/bajuqha884cxgwl/data1.dat -P",
            get_current_dir_name(), "/data/");
    std::cout << "\033[1;31m A sample data (at data/) does not exist, so start "
                 "to download (~260 MB)...\033[0m"
              << std::endl;

    int wget_result = std::system(command);
    std::cout << "Download result " << wget_result << std::endl;
    if (!this->check_for_traces()) {
      std::cout << "\033[1;31m Download is failed. Please see the "
                   "src/data-parser.h\033[0m"
                << std::endl;
    }

    return wget_result;
  }

  uint32_t check_for_traces() {
    struct stat buffer;
    return (stat(this->filename, &buffer) == 0);
  }
};
#endif
