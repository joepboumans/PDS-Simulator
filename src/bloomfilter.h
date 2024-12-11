#ifndef _BLOOMFILTER_H
#define _BLOOMFILTER_H

#include "BOBHash32.h"
#include "common.h"
#include "pds.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <set>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define BUF_SZ 1024 * 1024

class BloomFilter : public PDS {
public:
  BOBHash32 *hash;
  uint32_t n_hash;
  uint32_t length;
  uint32_t n;
  vector<bool> array;
  unordered_set<FIVE_TUPLE, fiveTupleHash> tuples;
  uint32_t insertions = 0;

  double f1 = 0.0;
  double recall = 0.0;
  double precision = 0.0;

  BloomFilter(uint32_t sz, uint32_t k, string trace, uint32_t n_stage,
              uint32_t n_struct)
      : PDS(trace, n_stage, n_struct, sz), array(sz, false) {

    // Init hashing
    this->hash = new BOBHash32[k];
    for (size_t i = 0; i < k; i++) {
      this->hash[i].initialize(n_struct * k + i);
    }

    // Setup vars
    this->length = sz;
    this->n_hash = k;
    this->n = n_struct;

    // Setup logging
    this->csv_header = "Epoch,Insertions,Recall,Precision,F1";
  }

  virtual void setName() { this->name = "BloomFilter"; }

  ~BloomFilter() {
    this->array.clear();
    this->fdata.close();
    this->fcsv.close();
  }

  uint32_t lookup(FIVE_TUPLE tuple) {
    for (size_t i = 0; i < this->n_hash; i++) {
      int hash_idx = this->hashing(tuple, i);
      if (!array[hash_idx]) {
        return 0;
      }
    }
    return 1;
  }

  void reset() {
    std::fill(this->array.begin(), this->array.end(), false);
    this->tuples.clear();
    this->insertions = 0;
    this->true_data.clear();
  }

  void analyze(int epoch) {
    int true_pos = 0, false_pos = 0, true_neg = 0, false_neg = 0;
    for (const auto &[s_tuple, count] : this->true_data) {
      if (auto search = this->tuples.find(s_tuple);
          search != this->tuples.end()) {
        // Recorded correctly
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

    char msg[100];
    sprintf(msg, "\tInsertions:%i\tRecall:%.3f\tPrecision:%.3f\tF1:%.3f",
            this->insertions, this->recall, this->precision, this->f1);
    std::cout << msg;

    // Save data into csv
    char csv[300];
    sprintf(csv, "%i,%i,%.3f,%.3f,%.3f", epoch, this->insertions, this->recall,
            this->precision, this->f1);
    this->fcsv << csv << std::endl;
  }

  void print_sketch() {
    uint32_t count = 0;
    for (auto i : this->array) {
      if (i) {
        count++;
      }
    }
    std::cout << std::endl;
    std::cout << "Total filled indexes: " << count << std::endl;
  }

  uint32_t insert(FIVE_TUPLE tuple) {
    // Record true data
    this->true_data[tuple]++;

    // Perform hashing
    bool tuple_inserted = false;
    for (size_t i = 0; i < this->n_hash; i++) {
      int hash_idx = this->hashing(tuple, i);
      if (!this->array[hash_idx]) {
        this->array[hash_idx] = true;
        tuple_inserted = true;
      }
    }

    // Record unqiue tuples
    if (tuple_inserted) {
      tuples.insert((string)tuple);
      this->insertions++;
      return 0;
    }
    return 1;
  }

  uint32_t hashing(FIVE_TUPLE key, uint32_t k) {
    char c_ftuple[sizeof(FIVE_TUPLE)];
    memcpy(c_ftuple, &key, sizeof(FIVE_TUPLE));
    return this->hash[k].run(c_ftuple, 4) % this->length;
  }
};

class LazyBloomFilter : public BloomFilter {
public:
  void setName() { this->name = "LazyBloomFilter"; }

  LazyBloomFilter(uint32_t sz, uint32_t k, string trace, uint32_t n_stage,
                  uint32_t n_struct)
      : BloomFilter(sz, k, trace, n_stage, n_struct) {}
  uint32_t insert(FIVE_TUPLE tuple) {
    this->true_data[tuple]++;

    for (size_t i = 0; i < this->n_hash; i++) {
      int hash_idx = this->hashing(tuple, i);
      // Only update this index and store new tuple
      if (!array[hash_idx]) {
        array[hash_idx] = true;
        tuples.insert((string)tuple);
        return 0;
      }
    }
    return 1;
  }
};
#endif // !_BLOOMFILTER_H
