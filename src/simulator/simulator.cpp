#ifndef _SIMULATOR_CPP
#define _SIMULATOR_CPP
#include "simulator.h"

int Simulator::run(const TRACE &trace, unsigned int duration) {
  unsigned long num_pkts = trace.size();
  unsigned long line_rate = num_pkts / duration; // per second
  unsigned long packets_per_epoch = this->epoch * line_rate;
  std::cout << "Starting run with " << this->n_pds << " stages over "
            << num_pkts << " packets" << " with a line rate of " << line_rate
            << " p/s over " << duration / this->epoch << " epochs" << std::endl;

  // Split into epochs for simulations
  for (size_t i = 0; i < num_pkts; i += (packets_per_epoch)) {
    int epoch = (int)i / packets_per_epoch;
    std::cout << "\rEpoch: " << epoch << std::endl;
    if (i + packets_per_epoch < num_pkts) {
      this->insert(trace, i, i + packets_per_epoch);
    } else {
      this->insert(trace, i, num_pkts - i);
    }
    // Store data, analyze data and reset the PDS
    for (auto p : this->pds) {
      p->store_data(epoch);
      p->analyze(this->true_data);
      p->reset();
    }
    this->true_data.clear();
  }

  return 0;
}

int Simulator::insert(const TRACE &trace, int start, int end) {
  for (size_t i = start; i < end; i++) {
    FIVE_TUPLE tuple = trace.at(i);
    this->true_data[(string)tuple]++;
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
