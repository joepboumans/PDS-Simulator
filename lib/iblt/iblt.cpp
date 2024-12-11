#ifndef _IBLT_CPP
#define _IBLT_CPP

#include "iblt.h"
#include "common.h"
#include <cstdint>

template class IBLT<FIVE_TUPLE, fiveTupleHash>;
template class IBLT<FLOW_TUPLE, flowTupleHash>;

template <typename TUPLE, typename HASH>
uint32_t IBLT<TUPLE, HASH>::hashing(TUPLE key, uint32_t k) {
  return this->hash[k].run((const char *)key.num_array, 4) % this->length;
}

template <typename TUPLE, typename HASH>
uint32_t IBLT<TUPLE, HASH>::insert(TUPLE tuple) {
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

template <typename TUPLE, typename HASH>
uint32_t IBLT<TUPLE, HASH>::lookup(TUPLE tuple) {
  return 1;
}

template <typename TUPLE, typename HASH> void IBLT<TUPLE, HASH>::reset() {
  return;
}

template <typename TUPLE, typename HASH>
void IBLT<TUPLE, HASH>::analyze(int epoch) {
  return;
}

template <typename TUPLE, typename HASH> void IBLT<TUPLE, HASH>::store_data() {
  return;
}

template <typename TUPLE, typename HASH>
void IBLT<TUPLE, HASH>::print_sketch() {
  return;
}

#endif // !_IBLT_CPP
