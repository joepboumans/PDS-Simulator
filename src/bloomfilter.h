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
  uint32_t n_tables;
  uint32_t length;
  uint32_t n;
  vector<bool> array;
  unordered_set<TUPLE, TupleHash> tuples;
  uint32_t insertions = 0;
  string trace_name;

  double f1 = 0.0;
  double recall = 0.0;
  double precision = 0.0;

  BloomFilter(uint32_t n_tables, uint32_t sz, string trace, uint8_t tuple_sz)
      : PDS(trace, tuple_sz), n_tables(n_tables), length(sz), array(sz, false) {

    // Init hashing
    this->hash = new BOBHash32[n_tables];
    for (size_t i = 0; i < n_tables; i++) {
      this->hash[i].initialize(750 + i);
    }

    // Setup logging
    this->csv_header = "Insertions,Collisions,Recall,Precision,F1";
    this->name = "AMQ/BloomFilter";
    this->rows = n_tables;
    this->columns = length;
    this->setupLogging();
  }

  uint32_t lookup(TUPLE tuple) {
    for (size_t i = 0; i < this->n_tables; i++) {
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
    sprintf(csv, "%i,%.3f,%.3f,%.3f", this->insertions, this->recall,
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

  uint32_t insert(TUPLE tuple) {
    // Record true data
    this->true_data[tuple]++;

    // Perform hashing
    bool tuple_inserted = false;
    for (size_t i = 0; i < this->n_tables; i++) {
      int hash_idx = this->hashing(tuple, i);
      if (!this->array[hash_idx]) {
        this->array[hash_idx] = true;
        tuple_inserted = true;
      }
    }

    // Record unqiue tuples
    if (tuple_inserted) {
      tuples.insert(tuple);
      this->insertions++;
      return 0;
    }
    return 1;
  }

  uint32_t hashing(TUPLE key, uint32_t k) {
    return hash[k].run((const char *)key.num_array, tuple_sz) % this->length;
  }
};

class LazyBloomFilter : public BloomFilter {
public:
  void setName() { this->name = "LazyBloomFilter"; }

  LazyBloomFilter(uint32_t n_tables, uint32_t sz, string trace,
                  uint8_t tuple_sz)
      : BloomFilter(n_tables, sz, trace, tuple_sz) {}
  uint32_t insert(TUPLE tuple) {
    this->true_data[tuple]++;

    for (size_t i = 0; i < this->n_tables; i++) {
      int hash_idx = this->hashing(tuple, i);
      // Only update this index and store new tuple
      if (!array[hash_idx]) {
        array[hash_idx] = true;
        tuples.insert(tuple);
        return 0;
      }
    }
    return 1;
  }
};
#endif // !_BLOOMFILTER_H
