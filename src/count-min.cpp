#ifndef _SIMULATOR_CPP
#define _SIMULATOR_CPP

#include "count-min.h"
#include "common.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <math.h>
#include <sys/types.h>

uint32_t CountMin::insert(FIVE_TUPLE tuple) {
  this->true_data[(string)tuple]++;
  for (size_t i = 0; i < this->n_hash; i++) {
    int hash_idx = this->hashing(tuple, i);
    this->counters[i][hash_idx].increment();
  }
  return 0;
}

uint32_t CountMin::hashing(FIVE_TUPLE key, uint32_t k) {
  char c_ftuple[sizeof(FIVE_TUPLE)];
  memcpy(c_ftuple, &key, sizeof(FIVE_TUPLE));
  return this->hash[k].run(c_ftuple, 4) % this->columns;
}

uint32_t CountMin::lookup(FIVE_TUPLE tuple) {
  uint32_t min = std::numeric_limits<uint32_t>::max();
  for (size_t i = 0; i < this->n_hash; i++) {
    int hash_idx = this->hashing(tuple, i);
    min = std::min(min, this->counters[i][hash_idx].count);
  }
  return min;
}

void CountMin::analyze(int epoch) {
  // Use lookup to find tuples
  double average_absolute_error = 0;
  double average_relative_error = 0;
  double n = this->true_data.size();

  for (const auto &[s_tuple, count] : this->true_data) {
    FIVE_TUPLE tuple(s_tuple);
    int diff = count - this->lookup(tuple);
    average_absolute_error += std::abs(diff);
    average_relative_error += (double)std::abs(diff) / count;
  }
  average_absolute_error = average_absolute_error / n;
  average_relative_error = average_relative_error / n;

  std::cout << std::endl
            << epoch << "\t" << average_absolute_error << std::endl;
  std::cout << epoch << "\t" << average_relative_error << std::endl;
  // Flow Size Estimation (Average Relative Error, Average Absolute Error)
  // Heavy Hitter Detection (F1 Score)
  // Flow Size Distribution (Weighted Mean Relative Error)
}

void CountMin::reset() {
  this->true_data.clear();
  for (auto &r : this->counters) {
    for (auto &c : r) {
      c.count = 0;
      c.overflow = false;
    }
  }
}

void CountMin::print_sketch() {
  char msg[100];
  sprintf(msg, "Printing CM sketch of %ix%i with mem sz %i", this->n_hash,
          this->columns, this->mem_sz);
  std::cout << msg << std::endl;
  for (size_t i = 0; i < this->n_hash; i++) {
    std::cout << i << ":\t";
    for (const auto &c : this->counters[i]) {
      std::cout << c.count << " | ";
    }
    std::cout << std::endl;
  }
}
#endif // !_SIMULATOR_CPP
