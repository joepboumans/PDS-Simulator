#ifndef _SIMULATOR_CPP
#define _SIMULATOR_CPP
#include "simulator.h"
#include <chrono>
#include <cmath>
#include <iostream>

int Simulator::run(const TRACE &trace, unsigned int duration) {
  unsigned long num_pkts = trace.size();
  unsigned long line_rate = std::ceil((float)num_pkts / duration); // per second
  unsigned long packets_per_epoch = this->epoch_len * line_rate;
  std::cout << "Starting run with " << this->n_pds << " stages over "
            << num_pkts << " packets" << " with a line rate of " << line_rate
            << " p/s over " << duration / this->epoch_len << " epochs"
            << std::endl;

  auto start = std::chrono::high_resolution_clock::now();
  // Split into epochs for simulations
  for (size_t i = 0; i < num_pkts; i += (packets_per_epoch)) {
    int epoch = (int)i / packets_per_epoch;
    std::cout << "\rEpoch: " << epoch << std::flush;
    if (i + packets_per_epoch < num_pkts) {
      this->insert(trace, i, i + packets_per_epoch);
    } else {
      // Skip left over the last, not complete epoch
      break;
    }
    // Store data, analyze data and reset the PDS
    for (auto p : this->pds) {
      // p->store_data(epoch);
      // p->print_sketch();
      p->analyze(epoch);
      p->reset();
    }
    break;
  }
  auto stop = std::chrono::high_resolution_clock::now();
  auto time = duration_cast<std::chrono::milliseconds>(stop - start);
  std::cout << std::endl;
  std::cout << "Finished data set with time: " << time << std::endl;

  return 0;
}

int Simulator::insert(const TRACE &trace, int start, int end) {
  for (size_t i = start; i < end; i++) {
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
