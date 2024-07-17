#ifndef _BLOOMFILTER_H
#define _BLOOMFILTER_H

#include "BOBHash32.h"
#include "common.h"
#include "pds.h"
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <vector>
class BloomFilter : PDS {
  vector<bool> array;
  BOBHash32 *hash;
  uint32_t n_hash;
  uint32_t length;

public:
  BloomFilter(uint32_t sz, uint32_t n, uint32_t k) {
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
  ~BloomFilter() { this->array.clear(); }

  int insert(FIVE_TUPLE tuple) {
    for (size_t i = 0; i < this->n_hash; i++) {
      int hash_idx = this->hashing(tuple, i);
      if (!array[hash_idx]) {
        array[hash_idx] = true;
      }
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

  void print_sketch() {
    for (auto i : this->array) {
      std::cout << i << " ";
    }
    std::cout << std::endl;
  }

private:
  int hashing(FIVE_TUPLE key, uint32_t k) {
    char c_ftuple[sizeof(FIVE_TUPLE)];
    memcpy(c_ftuple, &key, sizeof(FIVE_TUPLE));
    return hash[k].run(c_ftuple, 4) % this->length;
  }
};

#endif // !_BLOOMFILTER_H
