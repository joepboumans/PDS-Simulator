#ifndef _COUNTER_H
#define _COUNTER_H

#include <cstdint>
class Counter {
public:
  uint32_t count = 0;
  uint32_t max_count;
  bool overflow = false;
  Counter(uint32_t max) { this->max_count = max; }

  uint32_t increment(uint32_t c = 1) {
    if (this->overflow) {
      return 1;
    }
    this->count += c;
    if (this->count >= this->max_count) {
      this->overflow = true;
    }
    return 0;
  }

  uint32_t decrement(uint32_t c = 1) {
    if (this->overflow) {
      this->overflow = false;
    }
    if (this->count > 0) {
      this->count -= c;
      return 0;
    }
    return 1;
  }
};

#endif // !_COUNTER_H
