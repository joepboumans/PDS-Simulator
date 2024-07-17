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
#include <iostream>
#include <ostream>
#include <vector>

class FCM_Sketch : public PDS {
private:
  vector<Counter *> stages;
  BOBHash32 hash;
  uint32_t n_stages;
  uint32_t *stages_sz;
  uint32_t k;

  int hashing(FIVE_TUPLE key) {
    char c_ftuple[sizeof(FIVE_TUPLE)];
    memcpy(c_ftuple, &key, sizeof(FIVE_TUPLE));
    return hash.run(c_ftuple, 4) % stages_sz[0];
  }

public:
  FCM_Sketch(uint32_t n_roots, uint32_t n, uint32_t n_stages = 3,
             uint32_t k = 8) {
    // Check if structure is possible, max counter is 32bit

    // Maximum 32 bit counter
    uint32_t max_counter = 32;
    uint32_t *sz_stages = new uint32_t[n_stages];
    std::cout << "Got settings " << n_roots << " " << max_counter << std::endl;
    for (int i = n_stages - 1; i >= 0; --i) {
      sz_stages[i] = n_roots;
      n_roots *= k;
      std::cout << sz_stages[i] << std::endl;
    }

    // Setup stages with appropriate sizes
    for (size_t i = 0; i < n_stages; i++) {
      Counter *stage = new Counter[sz_stages[i]];
      for (size_t j = 0; j < sz_stages[j]; j++) {
        stage[j] = Counter(max_counter);
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
      char msg[30];
      sprintf(msg, "(%i:%i) : %i\n", (int)s, hash_idx,
              this->stages[s][hash_idx].count);
      std::cout << msg;
      break;
    }
    return 0;
  }

  void print_sketch() {
    for (size_t s = 0; s < n_stages; s++) {
      std::cout << "Stage " << s << " with size " << this->stages_sz[s]
                << std::endl;
      for (size_t i = 0; i < this->stages_sz[s]; i++) {
        std::cout << this->stages[s][i].count << " ";
      }
      std::cout << std::endl;
    }
  }
  // int remove(FIVE_TUPLE tuple) { return 0; }
  // int lookup(FIVE_TUPLE tuple) { return 0; }
};

#endif // !_FCM_SKETCH_H
