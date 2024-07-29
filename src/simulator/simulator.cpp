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
  // Insert the trace
  for (size_t i = 0; i < num_pkts; i += (packets_per_epoch)) {
    std::cout << "\rEpoch: " << (int)i / packets_per_epoch << std::flush;
    if (i + packets_per_epoch < num_pkts) {
      this->insert(trace, i, i + packets_per_epoch);
    } else {
      this->insert(trace, i, num_pkts - i);
    }
  }
  std::cout << std::endl;

  for (auto p : this->pds) {
    p->print_sketch();
  }
  std::cout << "Finished insertions, collecting data..." << std::endl;
  // Collect data in PDS
  // Store collected data
  return 0;
}

int Simulator::insert(const TRACE &trace, int start, int end) {
  for (size_t i = start; i < end; i++) {
    FIVE_TUPLE tuple = trace.at(i);
    this->true_data[(string)tuple]++;
    // std::cout << "Insert tuple " << tuple << std::endl;
    for (auto p : this->pds) {
      // std::cout << "into pds";
      int res = p->insert(tuple);
      // Do not continue to the next PDS if return anything other than 0
      if (!res) {
        // std::cout << "Stopping..." << std::endl;
        break;
      }
    }
  }
  return 0;
}

int Simulator::analyze() {
  std::cout << "Hey!";
  return 0;
}
#endif // !_SIMULATOR_CPP
