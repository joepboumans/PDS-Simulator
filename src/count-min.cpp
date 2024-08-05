#ifndef _SIMULATOR_CPP
#define _SIMULATOR_CPP

#include "count-min.h"
#include "common.h"
#include <cstdio>

int CountMin::insert(FIVE_TUPLE tuple) {
  for (size_t i = 0; i < this->n_hash; i++) {
    int hash_idx = this->hashing(tuple, i);
    this->counters[i][hash_idx].increment();
  }
  return 0;
}

int CountMin::hashing(FIVE_TUPLE key, uint32_t k) {
  char c_ftuple[sizeof(FIVE_TUPLE)];
  memcpy(c_ftuple, &key, sizeof(FIVE_TUPLE));
  return this->hash[k].run(c_ftuple, 4) % this->columns;
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
