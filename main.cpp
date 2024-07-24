#include "src/bloomfilter.h"
#include "src/common.h"
#include "src/data-parser.h"
#include "src/fcmsketch.h"
#include <cstddef>
#include <cstring>
#include <glob.h>
#include <iostream>
#include <iterator>

int main() {
  dataParser data_parser;
  // Get data files
  glob_t glob_res;
  memset(&glob_res, 0, sizeof(glob_res));
  glob("data/*.dat", GLOB_TILDE, NULL, &glob_res);
  vector<std::string> filenames;
  for (size_t i = 0; i < glob_res.gl_pathc; ++i) {
    filenames.push_back(std::string(glob_res.gl_pathv[i]));
  }

  // Load in traces
  TRACE traces[std::size(filenames)];
  std::size_t i = 0;
  for (auto f : filenames) {
    std::cout << f << std::endl;
    data_parser.get_traces(f.data(), traces[i++]);
  }

  FCM_Sketch fcmsketch(4, 0);
  BloomFilter bfilter(1 * 1024 * 1024, 0, 4);
  unordered_map<string, uint32_t> true_data;
  // Loop over traces
  for (auto trace : traces) {
    int num_pkt = (int)trace.size();
    std::cout << "Trace loading with size: " << num_pkt << std::endl;
    for (int i = 0; i < 10; i++) {
      // Record true data
      FIVE_TUPLE tuple = trace.at(i);
      char c_ftuple[sizeof(FIVE_TUPLE)];
      memcpy(c_ftuple, &tuple, sizeof(FIVE_TUPLE));
      string s_ftuple;
      s_ftuple.assign(c_ftuple, sizeof(FIVE_TUPLE));
      print_five_tuple(tuple);
      true_data[s_ftuple]++;
      // Use Sketch insert
      fcmsketch.insert(trace.at(i));
      bfilter.insert(trace.at(i));
      if (!bfilter.lookup(trace.at(i))) {
        std::cout << "ERROR IN INSERTION!" << std::endl;
      }
    }
    break;
  }
  std::cout << "Finished insertions, printing results: " << std::endl;
  // Print results
  // fcmsketch.print_sketch();
  for (const auto &n : true_data) {
    const char *c = n.first.data();
    FIVE_TUPLE tuple;
    memcpy(&tuple, c, sizeof(FIVE_TUPLE));
    print_five_tuple(tuple);
  }
  // bfilter.print_sketch();
  return 0;
}
