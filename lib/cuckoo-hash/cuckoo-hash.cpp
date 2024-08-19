#ifndef _CUCKOO_HASH_CPP
#define _CUCKOO_HASH_CPP

#include "cuckoo-hash.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <math.h>
#include <sys/types.h>

uint32_t CuckooHash::insert(FIVE_TUPLE tuple) {
  this->true_data[(string)tuple]++;
  return 0;
}

uint32_t CuckooHash::hashing(FIVE_TUPLE key, uint32_t k) {
  char c_ftuple[sizeof(FIVE_TUPLE)];
  memcpy(c_ftuple, &key, sizeof(FIVE_TUPLE));
  return this->hash[k].run(c_ftuple, 4) % this->n_tables;
}

uint32_t CuckooHash::lookup(FIVE_TUPLE tuple) {
  uint32_t min = std::numeric_limits<uint32_t>::max();
  for (size_t i = 0; i < this->n_hash; i++) {
    int hash_idx = this->hashing(tuple, i);
    // min = std::min(min, this->counters[i][hash_idx].count);
  }
  return min;
}

void CuckooHash::analyze(int epoch) {
  double n = this->true_data.size();

  int true_pos = 0, false_pos = 0, true_neg = 0, false_neg = 0;

  for (const auto &[s_tuple, count] : this->true_data) {
  }

  // F1 Score
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

  // sprintf(msg, "\tRecall:%.3f\tPrecision:%.3f\tF1:%.3f", this->recall,
  //         this->precision, this->f1);
  // std::cout << epoch << msg << std::endl;

  // Save data into csv
  char csv[300];
  // sprintf(csv, "%i,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f", epoch,
  //         this->average_relative_error, this->average_absolute_error,
  //         this->wmre, this->recall, this->precision, this->f1);
  this->fcsv << csv << std::endl;
}

void CuckooHash::reset() { this->true_data.clear(); }

void CuckooHash::print_sketch() {
  char msg[100];
  sprintf(msg, "Printing CM sketch of %ix%i with mem sz %i", this->n_hash,
          this->n_tables, this->mem_sz);
  std::cout << msg << std::endl;
  for (size_t i = 0; i < this->n_hash; i++) {
    std::cout << i << ":\t";
    std::cout << std::endl;
  }
}
#endif // !_CUCKOO_HASH_CPP
