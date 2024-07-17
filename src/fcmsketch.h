#ifndef _FCM_SKETCH_H
#define _FCM_SKETCH_H

#include "common.h"
#include "counter.h"
#include "pds.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <vector>

class FCM_Sketch : public PDS {
  vector<Counter *> stages;

public:
  FCM_Sketch(uint32_t n_roots, uint32_t n_stages = 3, uint32_t k = 8) {
    // Check if structure is possible, max counter is 32bit

    // Maximum 32 bit counter
    uint32_t max_counter = 32;
    uint32_t sz_stages[n_stages];
    std::cout << "Got settings " << n_roots << " " << max_counter << std::endl;
    for (int i = n_stages - 1; i >= 0; --i) {
      sz_stages[i] = n_roots;
      n_roots *= k;
      std::cout << sz_stages[i] << std::endl;
    }

    for (size_t i = 0; i < n_stages; i++) {
      Counter *stage = new Counter[sz_stages[i]];
      for (size_t j = 0; j < sz_stages[j]; j++) {
        stage[j] = Counter(max_counter);
        // std::cout << stage[j].count << std::endl;
      }
      stages.push_back(stage);
    }
    std::cout << sz_stages[0] << std::endl;
    // for (auto stage : stages) {
    //   for (size_t i = 0; i < sz_stages[0]; i++) {
    //     std::cout << stage[i].count << std::endl;
    //   }
    // }
  }
  //
  // ~FCM_Sketch() {
  //
  //   for (auto v : this->stages) {
  //     v.clear();
  //   }
  // }
  int insert(FIVE_TUPLE tuple) { return 0; }
  int remove(FIVE_TUPLE tuple) { return 0; }
  int lookup(FIVE_TUPLE tuple) { return 0; }

private:
};

#endif // !_FCM_SKETCH_H
