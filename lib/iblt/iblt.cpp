#ifndef _IBLT_CPP
#define _IBLT_CPP

#include "iblt.h"
#include "common.h"
#include <cstdint>

uint32_t IBLT::hashing(TUPLE key, uint32_t k) {
  return this->hash[k].run((const char *)key.num_array, key.sz) % this->length;
}

uint32_t IBLT::insert(TUPLE tuple) {
  // Record true data
  this->true_data[tuple]++;

  for (size_t i = 0; i < this->n_hash; i++) {
    int hash_idx = this->hashing(tuple, i);
    this->table[hash_idx].count++;
    this->table[hash_idx].tupleXOR ^= tuple;
    this->table[hash_idx].hashXOR ^= hash_idx;
  }
  return 0;
}

uint32_t IBLT::lookup(TUPLE tuple) { return 1; }

void IBLT::reset() { return; }

void IBLT::analyze(int epoch) { return; }

void IBLT::store_data() { return; }

void IBLT::print_sketch() { return; }

#endif // !_IBLT_CPP
