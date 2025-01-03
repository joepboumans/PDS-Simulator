#ifndef _SIMULATOR_CPP
#define _SIMULATOR_CPP
#include "simulator.h"
#include "common.h"
#include "pds.h"
#include <chrono>
#include <cmath>
#include <iostream>

template class Simulator<PDS<FIVE_TUPLE, fiveTupleHash>, TRACE>;
template class Simulator<PDS<FLOW_TUPLE, flowTupleHash>, TRACE_FLOW>;

template <typename P, typename T>
int Simulator<P, T>::run(const T &trace, unsigned int iters) {
  // Scale duration to a minimum for smaller datasets
  unsigned long num_pkts = trace.size();
  if (num_pkts < this->duration) {
    this->duration = num_pkts;
  }

  // Calculate the speed/line rate of the simulation
  unsigned long packets_per_epoch = std::floor(num_pkts / this->duration);
  std::cout << "Starting run with " << this->n_pds << " stage(s) over "
            << num_pkts << " packets" << " with " << packets_per_epoch
            << " packets/epoch over " << iters << " epoch(s)" << std::endl;

  // Run over each epoch and insert, analyze and reset
  auto start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < iters; i++) {
    std::cout << std::endl << "--- Epoch: " << i << " ---" << std::endl;

    this->insert(trace, i * packets_per_epoch, (i + 1) * packets_per_epoch);
    for (auto p : this->pds) {
      p->analyze(i);
      p->reset();
    }
  }

  auto stop = std::chrono::high_resolution_clock::now();
  auto time = duration_cast<std::chrono::milliseconds>(stop - start);
  std::cout << std::endl;
  std::cout << "Finished data set with time: " << time << " ms" << std::endl;

  return 0;
}

template <typename P, typename T>
int Simulator<P, T>::insert(const T &trace, int start, int end) {
  for (size_t i = start; i < end; i++) {
    // Prevent going out of bounds
    if (i >= trace.size()) {
      break;
    }

    // Insert and do not continue to the next PDS if return anything other
    // than 0
    for (auto p : this->pds) {
      int res = p->insert(trace.at(i));
      if (!res) {
        break;
      }
    }
  }
  return 0;
}

#endif // !_SIMULATOR_CPP
