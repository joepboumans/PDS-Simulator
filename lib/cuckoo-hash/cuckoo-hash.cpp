#ifndef _CUCKOO_HASH_CPP
#define _CUCKOO_HASH_CPP

#include "cuckoo-hash.hpp"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <math.h>
#include <ostream>
#include <sys/types.h>

template class CuckooHash<FIVE_TUPLE, fiveTupleHash>;
template class CuckooHash<FLOW_TUPLE, flowTupleHash>;

template <typename TUPLE, typename HASH>
uint32_t CuckooHash<TUPLE, HASH>::insert(TUPLE tuple) {
  this->true_data[tuple]++;
  // If it is present CHT do not insert
  if (!this->lookup(tuple)) {
    // Not seen before, or out of table, considerd as new unique and send to
    // data plane
    this->tuples.insert(tuple);
    this->insertions++;

    TUPLE prev_tuple = tuple;
    for (size_t i = 0; i < this->n_tables; i++) {
      uint32_t idx = this->hashing(prev_tuple, i);
      TUPLE empty = {};
      // Repeat until insertion into empty slot or until we run out of tables
      if (this->tables[i][idx] == empty) {
        this->tables[i][idx] = prev_tuple;
        return 1;
      } else {
        TUPLE tmp_tuple = this->tables[i][idx];
        this->tables[i][idx] = prev_tuple;
        prev_tuple = tmp_tuple;
      }
    }
  }
  return 1;
}

template <typename TUPLE, typename HASH>
uint32_t CuckooHash<TUPLE, HASH>::hashing(TUPLE key, uint32_t k) {
  return hash[k].run((const char *)key.num_array, sizeof(TUPLE)) % this->length;
}

template <typename TUPLE, typename HASH>
uint32_t CuckooHash<TUPLE, HASH>::lookup(TUPLE tuple) {
  for (size_t i = 0; i < this->n_tables; i++) {
    int idx = this->hashing(tuple, i);
    if (this->tables[i][idx] == tuple) {
      return 1;
    }
  }

  return 0;
}

template <typename TUPLE, typename HASH>
void CuckooHash<TUPLE, HASH>::analyze(int epoch) {
  double n = this->true_data.size();

  int true_pos = 0, false_pos = 0, true_neg = 0, false_neg = 0;

  for (const auto &[tuple, count] : this->true_data) {
    if (auto search = this->tuples.find(tuple); search != this->tuples.end()) {
      true_pos++;
    } else {
      false_pos++;
    }
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

  // Load factor
  this->load_factor = double(this->insertions) / this->tuples.size();

  std::cout << std::endl;
  char msg[200];
  sprintf(msg,
          "[CHT] Insertions:%i\tLoad "
          "factor:%.5f\tRecall:%.3f\tPrecision:%.3f\tF1:%.3f\n",
          this->insertions, this->load_factor, this->recall, this->precision,
          this->f1);
  std::cout << msg;

  // Save data into csv
  char csv[300];
  sprintf(csv, "%i,%i,%.3f,%.3f,%.3f", epoch, this->insertions, this->recall,
          this->precision, this->f1);
  this->fcsv << csv << std::endl;
}

template <typename TUPLE, typename HASH> void CuckooHash<TUPLE, HASH>::reset() {
  this->insertions = 0;
  this->true_data.clear();
  this->tuples.clear();

  TUPLE empty = {};
  for (auto &table : this->tables) {
    for (auto &tuple : table) {
      tuple = empty;
    }
  }
}

template <typename TUPLE, typename HASH>
void CuckooHash<TUPLE, HASH>::print_sketch() {
  char msg[100];
  sprintf(msg, "Printing Cuckoo Hash of %ix%i with mem sz %i", this->n_tables,
          this->length, this->mem_sz);
  std::cout << msg << std::endl;
  for (size_t l = 0; l < this->length; l++) {
    std::cout << l << ":\t";
    for (size_t i = 0; i < this->n_tables; i++) {
      std::cout << this->tables[i][l] << "\t";
    }
    std::cout << std::endl;
  }
}
#endif // !_CUCKOO_HASH_CPP
