#ifndef _FCM_SKETCH_H
#define _FCM_SKETCH_H

#include "BOBHash32.h"
#include "common.h"
#include "counter.h"
#include "pds.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <limits>
#include <ostream>
#include <unordered_map>
#include <vector>

class FCM_Sketch : public PDS {
private:
  unordered_map<string, uint32_t> true_data;
  vector<Counter *> stages;
  BOBHash32 hash;
  uint32_t n_stages;
  uint32_t *stages_sz;
  uint32_t k;

  int hashing(FIVE_TUPLE key) {
    char c_ftuple[sizeof(FIVE_TUPLE)];
    memcpy(c_ftuple, &key, sizeof(FIVE_TUPLE));
    string s_ftuple = c_ftuple;
    this->true_data[s_ftuple]++;
    return hash.run(c_ftuple, 4) % stages_sz[0];
  }

public:
  FCM_Sketch(uint32_t n_roots, uint32_t n, uint32_t n_stages = 3,
             uint32_t k = 8) {
    // Check if structure is possible, max counter is 32bit

    // Maximum 32 bit counter
    uint32_t max_bits = 16;
    uint32_t max_count = std::numeric_limits<uint32_t>::max();
    uint32_t max_counter[n_stages];
    uint32_t *sz_stages = new uint32_t[n_stages];
    for (int i = n_stages - 1; i >= 0; --i) {
      sz_stages[i] = n_roots;
      max_counter[i] = max_count;
      n_roots *= k;
      max_count = max_count >> max_bits;
      max_bits /= 2;
      std::cout << sz_stages[i] << " " << max_counter[i] << std::endl;
    }

    // Setup stages with appropriate sizes
    for (size_t i = 0; i < n_stages; i++) {
      Counter *stage = new Counter[sz_stages[i]];
      for (size_t j = 0; j < sz_stages[j]; j++) {
        stage[j] = Counter(max_counter[j]);
      }
      stages.push_back(stage);
    }
    this->hash.initialize(n);
    this->stages_sz = sz_stages;
    this->k = k;
    this->n_stages = n_stages;
  }

  ~FCM_Sketch() {
    for (auto v : this->stages) {
      delete v;
    }
  }
  int insert(FIVE_TUPLE tuple) {
    int hash_idx = this->hashing(tuple);
    for (size_t s = 0; s < n_stages; s++) {
      if (this->stages[s][hash_idx].overflow) {
        // Check for complete overflow
        if (s == n_stages - 1) {
          return 1;
        }
        continue;
      }
      this->stages[s][hash_idx].increment();
      break;
    }
    return 0;
  }

  void print_sketch() {
    for (size_t s = 0; s < n_stages; s++) {
      std::cout << "Stage " << s << " with " << this->stages_sz[s]
                << " counters" << std::endl;
      for (size_t i = 0; i < this->stages_sz[s]; i++) {
        std::cout << this->stages[s][i].count << " ";
      }
      std::cout << std::endl;
    }
    for (const auto &n : this->true_data) {
      char *c = n.first.data();
      FIVE_TUPLE tuple;
      memcpy(&tuple, n.first, sizeof(FIVE_TUPLE));
      print_five_tuple(tuple);
      std::cout << n.second << std::endl;
    }
  }
};

#endif // !_FCM_SKETCH_H
