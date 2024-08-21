#ifndef _FCM_SKETCH_CPP
#define _FCM_SKETCH_CPP

#include "fcm-sketch.h"
#include <cstdint>

uint32_t FCM_Sketch::hashing(FIVE_TUPLE key) {
  char c_ftuple[sizeof(FIVE_TUPLE)];
  memcpy(c_ftuple, &key, sizeof(FIVE_TUPLE));
  return hash.run(c_ftuple, 4) % stages_sz[n_stages - 1];
}

uint32_t FCM_Sketch::insert(FIVE_TUPLE tuple) {
  this->true_data[(string)tuple]++;

  uint32_t hash_idx = this->hashing(tuple);
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
void FCM_Sketch::print_sketch() {
  for (size_t s = 0; s < n_stages; s++) {
    std::cout << "Stage " << s << " with " << this->stages_sz[s] << " counters"
              << std::endl;
    for (size_t i = 0; i < this->stages_sz[s]; i++) {
      std::cout << this->stages[s][i].count << " ";
    }
    std::cout << std::endl;
  }
}

void FCM_Sketch::reset() {
  this->true_data.clear();
  for (auto &stage : this->stages) {
    for (auto &counter : stage) {
      counter.count = 0;
      counter.overflow = false;
    }
  }
}

void FCM_Sketch::analyze(int epoch) { return; }
#endif // !_FCM_SKETCH_CPP
