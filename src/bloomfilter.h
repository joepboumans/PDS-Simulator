#ifndef _BLOOMFILTER_H
#define _BLOOMFILTER_H

#include "BOBHash32.h"
#include "common.h"
#include "pds.h"
#include <algorithm>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <ios>
#include <iostream>
#include <memory>
#include <set>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#define BUF_SZ 1024 * 1024

class BloomFilter : public PDS {
public:
  BOBHash32 *hash;
  uint32_t n_hash;
  uint32_t length;
  uint32_t n;
  vector<bool> array;
  set<string> tuples;

  double avg_f1 = 0.0;
  double avg_recall = 0.0;

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
    this->csv_header =
        "epoch,Total uniques,Total found,FP,FN,Recall,Precision,F1";
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
    this->true_data.clear();
  }

  void analyze(int epoch) {
    int false_pos = 0;
    int total_pkt = this->true_data.size();
    for (const auto &[s_tuple, count] : this->true_data) {
      FIVE_TUPLE tup(s_tuple);
      if (auto search = this->tuples.find((string)tup);
          search != this->tuples.end()) {
        // Recorded correctly
      } else {
        false_pos++;
      }
    }
    int true_pos = total_pkt - false_pos;
    double recall = (double)true_pos / (true_pos + false_pos);
    double precision = 1; // IN BF can never result in false negatives
    double f1_score = 2 * ((recall * precision) / (precision + recall));
    char msg[100];

    sprintf(msg, "%i,%i,%zu,%i,%i,%.3f,%.3f,%.3f", epoch, total_pkt,
            this->tuples.size(), false_pos, 0, recall, precision, f1_score);
    this->fcsv << msg << std::endl;
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

  void store_data(int epoch) {
    if (!this->fdata.is_open()) {
      std::cout << "Cannot open file " << this->filename_dat << std::endl;
      throw;
    }
    // Write epoch:
    char msg[3];
    sprintf(msg, "%i:", epoch);
    this->fdata.write(msg, sizeof(msg));

    // Print data to file
    char buf[this->length];
    std::copy(this->array.begin(), this->array.end(), buf);
    this->fdata.write(buf, sizeof(buf));
    char endl = '\n';
    this->fdata.write(&endl, sizeof(endl));
    return;
  }

  uint32_t insert(FIVE_TUPLE tuple) {
    // Record true data
    this->true_data[(string)tuple]++;

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
    this->true_data[(string)tuple]++;

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
