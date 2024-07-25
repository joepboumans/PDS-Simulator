#include "src/bloomfilter.h"
#include "src/common.h"
#include "src/data-parser.h"
#include "src/fcmsketch.h"
#include "src/simulator/simulator.h"
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <glob.h>
#include <iostream>
#include <iterator>
#include <vector>

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

  vector<PDS> stages;

  // BloomFilter bfilter(1 * 1024 * 1024, 0, 4);
  // stages.push_back(bfilter);
  FIVE_TUPLE tuple = traces[0].at(0);
  // bfilter.insert(tuple);
  FCM_Sketch fcmsketch(4, 0);
  stages.push_back(fcmsketch);

  Simulator sim(stages, stages.size(), 1);
  for (const TRACE &trace : traces) {
    sim.run(trace);
    break;
  }

  // unordered_map<string, uint32_t> true_data;
  // // Loop over traces
  // for (auto trace : traces) {
  //   int num_pkt = (int)trace.size() / 60;
  //   std::cout << "Trace loading with size: " << num_pkt << std::endl;
  //   for (int i = 0; i < num_pkt; i++) {
  //     // Record true data
  //     FIVE_TUPLE tuple = trace.at(i);
  //     string s_ftuple = (string)tuple;
  //     true_data[s_ftuple]++;
  //     // Use Sketch insert
  //     // fcmsketch.insert(trace.at(i));
  //     bfilter.insert(trace.at(i));
  //     // if (!bfilter.lookup(trace.at(i))) {
  //     //   std::cout << "ERROR IN INSERTION!" << std::endl;
  //     // }
  //   }
  //   break;
  // }
  // std::cout << "Finished insertions, printing results: " << std::endl;
  // Print results
  // fcmsketch.print_sketch();
  // for (const auto &n : true_data) {
  //   const char *c = n.first.data();
  //   FIVE_TUPLE tuple;
  //   memcpy(&tuple, c, sizeof(FIVE_TUPLE));
  //   print_five_tuple(tuple);
  // }
  // int false_pos = 0;
  // int total_pkt = true_data.size();
  // std::cout << "Total found " << bfilter.tuples.size() << std::endl;
  // for (const auto &[s_tuple, count] : true_data) {
  //   FIVE_TUPLE tup(s_tuple);
  //   // std::cout << tup << std::endl;
  //   if (auto search = bfilter.tuples.find((string)tup);
  //       search != bfilter.tuples.end()) {
  //     // Recorded correctly
  //   } else {
  //     false_pos++;
  //   }
  // }
  // bfilter.print_sketch();
  // std::cout << "False Positives: " << false_pos << std::endl;
  // std::cout << "Total num_pkt " << total_pkt << std::endl;
  // int true_pos = total_pkt - false_pos;
  // double recall = (double)true_pos / (true_pos + false_pos);
  // double precision = 1; // IN BF can never result in false negatives
  // double f1_score = 2 * ((recall * precision) / (precision + recall));
  // char msg[1000];
  // sprintf(msg, "Recall: %.3f\tPrecision: %.3f\nF1-Score: %.3f", recall,
  //         precision, f1_score);
  // std::cout << msg << std::endl;
  //
  // return 0;
}
