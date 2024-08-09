#ifndef _SIMULATOR_CPP
#define _SIMULATOR_CPP

#include "count-min.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <math.h>
#include <numeric>
#include <set>
#include <sys/types.h>
#include <vector>

uint32_t CountMin::insert(FIVE_TUPLE tuple) {
  this->true_data[(string)tuple]++;
  vector<uint32_t> hh_overflow(this->n_hash, false);

  for (size_t i = 0; i < this->n_hash; i++) {
    int hash_idx = this->hashing(tuple, i);
    this->counters[i][hash_idx].increment();
    if (this->counters[i][hash_idx].count >= this->hh_threshold) {
      hh_overflow[i] = 1;
    }
  }
  if ((uint32_t)std::accumulate(hh_overflow.begin(), hh_overflow.end(), 0) ==
      this->n_hash) {
    this->HH_candidates.insert((string)tuple);
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
  this->average_absolute_error = 0;
  this->average_relative_error = 0;
  double n = this->true_data.size();

  std::cout << std::endl;

  // Flow Size Estimation (Average Relative Error, Average Absolute Error)
  for (const auto &[s_tuple, count] : this->true_data) {
    FIVE_TUPLE tuple(s_tuple);
    int diff = count - this->lookup(tuple);

    this->average_absolute_error += std::abs(diff);
    this->average_relative_error += ((double)std::abs(diff) / count);
  }

  this->average_absolute_error = this->average_absolute_error / n;
  this->average_relative_error = this->average_relative_error / n;
  std::cout << epoch << "\tAAE: " << this->average_absolute_error << std::endl;
  std::cout << epoch << "\tARE: " << this->average_relative_error << std::endl;
  std::cout << "n = " << n << std::endl;

  // Heavy Hitter Detection (F1 Score)
  int true_pos = 0, false_pos = 0, true_neg = 0, false_neg = 0;
  set<string> true_hh, true_not_hh;
  for (const auto &[s_tuple, count] : this->true_data) {
    if (count >= this->hh_threshold) {
      true_hh.insert(s_tuple);
      continue;
    }
    true_not_hh.insert(s_tuple);
  }
  for (const auto &hh : true_hh) {
    if (auto search = this->HH_candidates.find(hh);
        search != this->HH_candidates.end()) {
      true_pos++;
    } else {
      false_neg++;
    }
  }
  for (const auto &not_hh : true_not_hh) {
    if (auto search = this->HH_candidates.find(not_hh);
        search == this->HH_candidates.end()) {
      true_neg++;
    } else {
      false_pos++;
    }
  }
  std::cout << epoch << " Heavy hitters" << std::endl;
  char msg[200];
  sprintf(msg, "\tTP:%i\tFP:%i\tTN:%i\tFN:%i", true_pos, false_pos, true_neg,
          false_neg);
  std::cout << epoch << msg << std::endl;

  if (true_pos == 0 && false_pos == 0) {
    this->recall = 1.0;
  } else {
    this->recall = (double)true_pos / (true_pos + false_pos);
  }
  if (true_neg == 0 && false_neg == 0) {
    this->precision = 1.0;
  } else {
    this->precision = (double)true_neg / (true_neg + false_neg);
  }
  this->f1 = 2 * ((recall * precision) / (precision + recall));

  sprintf(msg, "\tRecall:%.3f\tPrecision:%.3f\tF1:%.3f", this->recall,
          this->precision, this->f1);
  std::cout << epoch << msg << std::endl;
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
