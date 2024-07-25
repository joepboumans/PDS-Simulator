#ifndef _SIMULATOR_CPP
#define _SIMULATOR_CPP
#include "simulator.h"

int Simulator::run(const TRACE &trace) {
  unsigned long duration = trace.size();
  std::cout << "Starting run with " << this->n_pds << " stages over "
            << duration << " packets" << std::endl;
  // Insert the trace
  for (size_t i = 0; i < duration; i += this->epoch) {
    if (i + this->epoch < duration) {
      this->insert(trace, i, i + this->epoch);
    } else {
      this->insert(trace, i, duration - i);
    }
  }

  for (auto p : this->pds) {
    p.print_sketch();
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
    for (size_t j = 0; j < this->pds.size(); j++) {
      int res = this->pds[j].insert(tuple);
      // Do not continue to the next PDS if return anything other than 0
      if (res) {
        std::cout << "Stopping..." << std::endl;
        return 0;
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
