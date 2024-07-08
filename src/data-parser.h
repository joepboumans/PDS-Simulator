#ifndef _DATA_PARSER_H_
#define _DATA_PARSER_H_


#include <iostream>
#include <fstream>
#include <unistd.h>

uint32_t download_data_traces() {
  char command[200];
  sprintf(command, "%s %s%s", "wget https://www.dropbox.com/s/bajuqha884cxgwl/data1.dat -P", get_current_dir_name(), "/data/");
  std::cout << "\033[1;31m A sample data (at data/) does not exist, so start to download (~260 MB)...\033[0m" << std::endl;

  char filename[100] = "data1.dat";
  FILE *fin;

  int wget_result = std::system(command);
  std::cout << "Download result " << wget_result << std::endl;
  if (!(fin = fopen(filename, "rb")))
      std::cout << "\033[1;31m Download is failed. Please see the src/data-parser.h\033[0m" << std::endl;

  return wget_result;
}

uint32_t get_traces() {
  return 0;
}
#endif
