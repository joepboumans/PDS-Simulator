#ifndef _Q_WATERFALL_CPP
#define _Q_WATERFALL_CPP

#include "qwaterfall.hpp"
#include "common.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <sys/types.h>

template class qWaterfall<FIVE_TUPLE, fiveTupleHash>;
template class qWaterfall<FLOW_TUPLE, flowTupleHash>;

template <typename TUPLE, typename HASH>
uint32_t qWaterfall<TUPLE, HASH>::hashing(TUPLE key, uint32_t k) {
  return hash[k].run((const char *)key.num_array, sizeof(TUPLE));
}

template <typename TUPLE, typename HASH>
uint32_t qWaterfall<TUPLE, HASH>::rehashing(uint32_t hashed_val, uint32_t k) {
  uint8_t in_val[4];
  memcpy(in_val, &hashed_val, sizeof(in_val));
  return hash[k].run((const char *)in_val, sizeof(in_val)) %
         std::numeric_limits<uint32_t>::max();
}

template <typename TUPLE, typename HASH>
uint32_t qWaterfall<TUPLE, HASH>::insert(TUPLE tuple) {
  this->true_data[tuple]++;
  // If it is present CHT do not insert
  if (!this->lookup(tuple)) {
    // Not seen before, or out of table, considerd as new unique and send to
    // data plane
    this->tuples.insert(tuple);
    this->insertions++;
    this->collisions++;

    // Hash the inital tuple for starting insertion
    uint32_t hash_val = this->hashing(tuple, 0);
    uint32_t prev_hash_val = 0;
    uint16_t idx = hash_val >> 16;
    uint16_t val = hash_val;
    // Insert into the first and exit if there was nothing stored
    if (this->tables[0][idx] == 0) {
      this->tables[0][idx] = val;
      return 1;
    } else {
      prev_hash_val = idx << 16 | this->tables[0][idx];
      this->tables[0][idx] = val;
    }

    for (size_t i = 1; i < this->n_tables; i++) {
      hash_val = this->rehashing(prev_hash_val, i);
      idx = hash_val >> 16;
      val = hash_val;
      // Repeat until insertion into empty slot or until we run out of tables
      if (this->tables[i][idx] == 0) {
        this->tables[i][idx] = val;
        return 1;
      } else {
        prev_hash_val = idx << 16 | this->tables[i][idx];
        this->tables[i][idx] = val;
      }
      this->collisions++;
    }
  }
  return 1;
}

template <typename TUPLE, typename HASH>
uint32_t qWaterfall<TUPLE, HASH>::lookup(TUPLE tuple) {
  // Hash the inital tuple for starting insertion
  uint32_t hash_val = this->hashing(tuple, 0);
  uint32_t prev_hash_val = hash_val;
  uint16_t idx = hash_val >> 16;
  uint16_t val = hash_val;
  // Insert into the first and exit if there was nothing stored
  if (this->tables[0][idx] == val) {
    return 1;
  } else {
    for (size_t i = 1; i < this->n_tables; i++) {
      hash_val = this->rehashing(prev_hash_val, i);
      idx = hash_val >> 16;
      val = (uint16_t)hash_val;
      if (this->tables[i][idx] == val) {
        return 1;
      }
      prev_hash_val = hash_val;
    }
  }

  return 0;
}

template <typename TUPLE, typename HASH> void qWaterfall<TUPLE, HASH>::reset() {
  this->insertions = 0;
  this->collisions = 0;
  this->true_data.clear();
  this->tuples.clear();

  for (auto &table : this->tables) {
    for (auto &tuple : table) {
      tuple = 0;
    }
  }
}

template <typename TUPLE, typename HASH>
void qWaterfall<TUPLE, HASH>::print_sketch() {
  char msg[100];
  sprintf(msg, "Printing qWaterfall of %ix%i with mem sz %i", this->n_tables,
          this->table_length, this->mem_sz);
  std::cout << msg << std::endl;
  for (size_t l = 0; l < this->table_length; l++) {
    std::cout << l << ":\t";
    for (size_t i = 0; i < this->n_tables; i++) {
      std::cout << this->tables[i][l] << "\t";
    }
    std::cout << std::endl;
  }
}

template <typename TUPLE, typename HASH>
void qWaterfall<TUPLE, HASH>::analyze(int epoch) {
  double n = this->true_data.size();

  // Find True/False positives
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
  this->load_factor = double(this->insertions) / n;
  std::cout << "[qWaterfall] True data size " << n << std::endl;

  char msg[200];
  sprintf(msg,
          "[qWaterfall] "
          "Insertions:%i\tLoad "
          "Factor:%.5f\tCollisions:%i\tRecall:%.5f\tPrecision:%.5f\tF1:%.5f\n",
          this->insertions, this->load_factor, this->collisions, this->recall,
          this->precision, this->f1);
  std::cout << msg;

  // Save data into csv
  char csv[300];
  sprintf(csv, "%i,%i,%i,%.3f,%.3f,%.3f", epoch, this->insertions,
          this->collisions, this->recall, this->precision, this->f1);
  this->fcsv << csv << std::endl;
}

#endif // !_Q_WATERFALL_CPP
