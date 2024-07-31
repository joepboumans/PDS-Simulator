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
#include <unistd.h>
#include <unordered_map>
#include <vector>

#define BUF_SZ 1024 * 1024

class BloomFilter : public PDS {
public:
  char filename_dat[400];
  char filename_csv[400];
  vector<bool> array;
  set<string> tuples;

  BOBHash32 *hash;
  uint32_t n_hash;
  uint32_t length;

  double avg_f1 = 0.0;
  double avg_recall = 0.0;

  std::ofstream fdata;
  std::ofstream fcsv;
  char data_buf[BUF_SZ];
  char csv_buf[BUF_SZ];

  BloomFilter(uint32_t sz, uint32_t n, uint32_t k) {
    sprintf(this->filename_dat, "results/%s_%i.dat", typeid(this).name(), n);
    sprintf(this->filename_csv, "results/%s_%i.csv", typeid(this).name(), n);
    // Remove previous data file
    std::remove(filename_dat);
    std::remove(filename_csv);
    // Open files

    std::cout << "Removed files, opening new files..." << std::endl;
    std::cout << this->filename_dat << std::endl;

    this->fdata.open(this->filename_dat, ios::out | ios_base::app);
    this->fdata.rdbuf()->pubsetbuf(this->data_buf, BUF_SZ);
    this->fcsv.open(this->filename_csv, std::ios::out);
    this->fcsv.rdbuf()->pubsetbuf(this->csv_buf, BUF_SZ);
    std::cout << "Opened files" << std::endl;
    char msg[100] = "epoch,total,false_pos,recall,precision,f1";
    this->fcsv << msg << std::endl;

    for (size_t i = 0; i < sz; i++) {
      array.push_back(false);
    }

    this->hash = new BOBHash32[k];
    for (size_t i = 0; i < k; i++) {
      this->hash[i].initialize(n * k + i);
    }
    this->length = sz;
    this->n_hash = k;
  }
  ~BloomFilter() {
    this->array.clear();
    this->fdata.close();
    this->fcsv.close();
  }

  int insert(FIVE_TUPLE tuple) {
    bool tuple_inserted = false;
    for (size_t i = 0; i < this->n_hash; i++) {
      int hash_idx = this->hashing(tuple, i);
      if (!array[hash_idx]) {
        array[hash_idx] = true;
        tuple_inserted = true;
      }
    }

    if (tuple_inserted) {
      tuples.insert((string)tuple);
    }
    return 0;
  }

  int lookup(FIVE_TUPLE tuple) {
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
  }

  void analyze(unordered_map<string, uint32_t> true_data, int epoch) {
    int false_pos = 0;
    int total_pkt = true_data.size();
    for (const auto &[s_tuple, count] : true_data) {
      FIVE_TUPLE tup(s_tuple);
      if (auto search = this->tuples.find((string)tup);
          search != this->tuples.end()) {
        // Recorded correctly
      } else {
        false_pos++;
      }
    }
    // std::cout << "Total found " << this->tuples.size() << std::endl;
    // std::cout << "False Positives: " << false_pos << std::endl;
    // std::cout << "Total num_pkt " << total_pkt << std::endl;
    int true_pos = total_pkt - false_pos;
    double recall = (double)true_pos / (true_pos + false_pos);
    double precision = 1; // IN BF can never result in false negatives
    double f1_score = 2 * ((recall * precision) / (precision + recall));
    char msg[100];
    // sprintf(msg, "Recall: %.3f\tPrecision: %.3f\nF1-Score: %.3f", recall,
    // std::cout << msg << std::endl;
    sprintf(msg, "%i,%i,%zu,%i,%.3f,%.3f,%.3f", epoch, total_pkt,
            this->tuples.size(), false_pos, recall, precision, f1_score);
    this->fcsv << msg << std::endl;
  }

  void print_sketch() {
    uint32_t count = 0;
    for (auto i : this->array) {
      // std::cout << i;
      if (i) {
        count++;
      }
    }
    std::cout << std::endl;
    std::cout << "Total filled indexes: " << count << std::endl;
  }

  int hashing(FIVE_TUPLE key, uint32_t k) {
    char c_ftuple[sizeof(FIVE_TUPLE)];
    memcpy(c_ftuple, &key, sizeof(FIVE_TUPLE));
    return hash[k].run(c_ftuple, 4) % this->length;
  }

  void store_data(int epoch) {
    if (!this->fdata.is_open()) {
      std::cout << "Cannot open file " << this->filename_dat << std::endl;
      throw;
    }
    try {
      char msg[3];
      sprintf(msg, "%i:", epoch);
      this->fdata.write(msg, sizeof(msg));
    } catch (exception e) {
      std::cout << "Failed writing to file" << std::endl;
      std::cout << e.what() << std::endl;
      throw e;
    }

    // Print data to file
    char buf[this->length];
    std::copy(this->array.begin(), this->array.end(), buf);
    this->fdata.write(buf, sizeof(buf));
    char endl = '\n';
    this->fdata.write(&endl, sizeof(endl));
    return;
  }
};

class LazyBloomfilter : public BloomFilter {
public:
  using BloomFilter::BloomFilter;
  // LazyBloomfilter(uint32_t sz, uint32_t n, uint32_t k)
  //     : BloomFilter(sz, n, k) {}

  int insert(FIVE_TUPLE tuple) {
    for (size_t i = 0; i < this->n_hash; i++) {
      int hash_idx = this->hashing(tuple, i);
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
