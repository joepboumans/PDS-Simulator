#ifndef _SIMULATOR_CPP
#define _SIMULATOR_CPP

#include "count-min.h"
#include "common.h"
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <math.h>
#include <sys/types.h>

uint32_t CountMin::insert(FIVE_TUPLE tuple) {
  for (size_t i = 0; i < this->n_hash; i++) {
    int hash_idx = this->hashing(tuple, i);
    this->counters[i][hash_idx].increment();
  }
  return 0;
}

uint32_t CountMin::hashing(FIVE_TUPLE key, uint32_t k) {
  char c_ftuple[sizeof(FIVE_TUPLE)];
  memcpy(c_ftuple, &key, sizeof(FIVE_TUPLE));
  return this->hash[k].run(c_ftuple, 4) % this->columns;
}

uint32_t CountMin::lookup(FIVE_TUPLE tuple) {
  uint32_t min = std::numeric_limits<uint32_t>::max();
  for (size_t i = 0; i < this->n_hash; i++) {
    int hash_idx = this->hashing(tuple, i);
    min = std::min(min, this->counters[i][hash_idx].count);
  }
  return min;
}

void CountMin::print_sketch() {
  char msg[100];
  sprintf(msg, "Printing CM sketch of %ix%i with mem sz %i", this->n_hash,
          this->columns, this->mem_sz);
  std::cout << msg << std::endl;
  for (size_t i = 0; i < this->n_hash; i++) {
    std::cout << i << ":\t";
    for (auto c : this->counters[i]) {
      std::cout << c.count << " | ";
    }
    std::cout << std::endl;
  }
}
#endif // !_SIMULATOR_CPP
