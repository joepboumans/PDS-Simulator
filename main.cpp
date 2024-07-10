#include <iostream>
#include "src/common.h"
#include "src/data-parser.h"

int main(){
  dataParser data_parser;
  // char filename[100] = "data1.dat";
  char filename[100] = "test.out";
  TRACE trace;
  data_parser.get_traces(filename, trace);

  std::cout << "Trace data:" << std::endl;
  for (int i = 0; i < 10; i++) {
    print_five_tuple(trace.at(i));
  }
  return 0;
}
