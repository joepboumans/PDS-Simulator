#ifndef _IBLT_CPP
#define _IBLT_CPP

#include "iblt.h"
#include <cstdint>

uint32_t IBLT::hashing(FIVE_TUPLE key, uint32_t k) {
  char c_ftuple[sizeof(FIVE_TUPLE)];
  memcpy(c_ftuple, &key, sizeof(FIVE_TUPLE));
  return this->hash[k].run(c_ftuple, 4) % this->length;
}

uint32_t IBLT::insert(FIVE_TUPLE tuple) {
  // Record true data
  this->true_data[(string)tuple]++;

  for (size_t i = 0; i < this->n_hash; i++) {
    int hash_idx = this->hashing(tuple, i);
    this->table[hash_idx].count++;
    this->table[hash_idx].tupleXOR ^= tuple;
    this->table[hash_idx].hashXOR ^= hash_idx;
  }
  return 0;
}

uint32_t IBLT::lookup(FIVE_TUPLE tuple) { return 1; }

void IBLT::reset() { return; }
void IBLT::analyze(int epoch) { return; }
void IBLT::store_data() { return; }
void IBLT::print_sketch() { return; }
#endif // !_IBLT_CPP
