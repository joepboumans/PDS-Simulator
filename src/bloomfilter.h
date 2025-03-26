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
  vector<vector<BOBHash32>> hash;
  uint32_t n_tables;
  uint32_t n_hashes;
  uint32_t length;
  uint32_t n;
  vector<vector<bool>> filters;
  unordered_set<TUPLE, TupleHash> tuples;
  uint32_t insertions = 0;
  string trace_name;

  double f1 = 0.0;
  double recall = 0.0;
  double precision = 0.0;

  BloomFilter(uint32_t n_tables, uint32_t sz, uint32_t n_hashes, string trace,
              uint8_t tuple_sz)
      : PDS(trace, tuple_sz), n_tables(n_tables), n_hashes(n_hashes),
        length(sz), filters(n_hashes, vector<bool>(sz, false)) {

    // Init hashing
    this->hash =
        vector<vector<BOBHash32>>(n_tables, vector<BOBHash32>(n_hashes));
    for (size_t i = 0; i < n_tables; i++) {
      for (size_t j = 0; j < n_hashes; j++) {
        this->hash[i][j].initialize(750 + i + j);
      }
    }

    // Setup logging
    this->csv_header = "Insertions,Recall,Precision,F1";
    this->name = "AMQ/BloomFilter";
    this->rows = n_tables;
    this->columns = length;
    this->setupLogging();
  }

  uint32_t lookup(TUPLE tuple) {
    for (size_t i = 0; i < this->n_tables; i++) {
      for (size_t j = 0; j < n_hashes; j++) {
        int hash_idx = this->hashing(tuple, i, j);
        if (!filters[i][hash_idx]) {
          continue;
        }
        return 1;
      }
    }
    return 0;
  }

  void reset() {
    for (auto &filter : this->filters) {
      std::fill(filter.begin(), filter.end(), false);
    }
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

    for (const auto &tup : this->tuples) {
      if (this->true_data.find(tup) != this->true_data.end()) {
        continue;
      } else {
        false_neg++;
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
      this->precision = (double)true_pos / (true_pos + false_neg);
    }
    this->f1 = 2 * ((recall * precision) / (precision + recall));

    char msg[100];
    sprintf(msg, "\tInsertions:%i\tRecall:%.5f\tPrecision:%.5f\tF1:%.5f\n",
            this->insertions, this->recall, this->precision, this->f1);
    std::cout << msg;

    // Save data into csv
    char csv[300];
    sprintf(csv, "%i,%.6f,%.6f,%.6f", this->insertions, this->recall,
            this->precision, this->f1);
    this->fcsv << csv << std::endl;
  }

  void print_sketch() {
    uint32_t count = 0;
    for (const auto &filter : this->filters) {
      for (const auto &i : filter) {
        if (i) {
          count++;
        }
      }
    }
    std::cout << std::endl;
    std::cout << "Total filled indexes: " << count << std::endl;
  }

  uint32_t insert(TUPLE tuple) {
    // Record true data
    this->true_data[tuple]++;

    // Insert into each BloomFilter
    bool tuple_inserted = false;
    for (size_t i = 0; i < this->n_tables; i++) {
      for (size_t j = 0; j < this->n_hashes; j++) {
        int hash_idx = this->hashing(tuple, i, j);
        if (!this->filters[i][hash_idx]) {
          this->filters[i][hash_idx] = true;
          tuple_inserted = true;
        }
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

  uint32_t hashing(TUPLE key, uint32_t i, uint32_t j) {
    return hash[i][j].run((const char *)key.num_array, tuple_sz) % this->length;
  }
};

class LazyBloomFilter : public BloomFilter {
public:
  LazyBloomFilter(uint32_t n_tables, uint32_t sz, uint32_t n_hashes,
                  string trace, uint8_t tuple_sz)
      : BloomFilter(n_tables, sz, n_hashes, trace, tuple_sz) {
    this->name = "AMQ/LazyBloomFilter";
    this->setupLogging();
  }
  uint32_t insert(TUPLE tuple) {
    // Record true data
    this->true_data[tuple]++;

    // Lazy insert into BloomFilter, update only single value
    bool tuple_inserted = false;
    for (size_t i = 0; i < this->n_tables; i++) {
      for (size_t j = 0; j < this->n_hashes; j++) {
        int hash_idx = this->hashing(tuple, i, j);
        if (!filters[i][hash_idx]) {
          filters[i][hash_idx] = true;
          tuple_inserted = true;
          break;
        }
      }
    }

    if (tuple_inserted) {
      tuples.insert(tuple);
      this->insertions++;
      return 0;
    }
    return 1;
  }
};
#endif // !_BLOOMFILTER_H
