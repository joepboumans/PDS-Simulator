#ifndef _SIMULATOR_CPP
#define _SIMULATOR_CPP
#include "simulator.h"
#include <chrono>
#include <cmath>
#include <iostream>

int Simulator::run(const TRACE &trace, unsigned int duration) {
  unsigned long num_pkts = trace.size();
  unsigned long line_rate = std::floor(num_pkts / this->epoch_len) - 1; // per
  unsigned long packets_per_epoch = line_rate;
  std::cout << "Starting run with " << this->n_pds << " stage(s) over "
            << num_pkts << " packets" << " with a line rate of " << line_rate
            << " p/s over " << duration << " epoch(s)" << std::endl;

  auto start = std::chrono::high_resolution_clock::now();
  // Split into epochs for simulations
  for (size_t i = 0; i < duration; i++) {
    // std::cout << "\rEpoch: " << i << std::flush;
    std::cout << "\rEpoch: " << i << std::endl;
    this->insert(trace, i * packets_per_epoch, (i + 1) * packets_per_epoch);
    // Store data, analyze data and reset the PDS
    for (auto p : this->pds) {
      p->analyze(i);
      p->reset();
    }
  }
  auto stop = std::chrono::high_resolution_clock::now();
  auto time = duration_cast<std::chrono::milliseconds>(stop - start);
  std::cout << std::endl;
  std::cout << "Finished data set with time: " << time << std::endl;

  return 0;
}

int Simulator::insert(const TRACE &trace, int start, int end) {
  for (size_t i = start; i < end; i++) {
    if (i >= trace.size()) {
      break;
    }
    FIVE_TUPLE tuple = trace.at(i);
    for (auto p : this->pds) {
      int res = p->insert(tuple);
      // Do not continue to the next PDS if return anything other than 0
      if (!res) {
        break;
      }
    }
  }
  return 0;
}

#endif // !_SIMULATOR_CPP
