#ifndef _COUNTER_H
#define _COUNTER_H

#include <cstdint>
class Counter {
public:
  uint32_t count = 0;
  uint32_t max_count;
  uint32_t max_reg;
  bool overflow = false;
  Counter() {}
  Counter(uint32_t max) : max_count(max - 1), max_reg(max) {}

  uint32_t increment(uint32_t c = 1) {
    if (this->overflow) {
      return 1;
    }
    this->count += c;
    if (this->count > this->max_count) {
      this->overflow = true;
      this->count = this->max_reg;
      return 1;
    }
    return 0;
  }

  uint32_t decrement(uint32_t c = 1) {
    if (this->overflow) {
      this->overflow = false;
      this->count = this->max_count;
    }
    if (this->count >= c) {
      this->count -= c;
      return 0;
    } else {
      c -= this->count;
      this->count = 0;
      return c;
    }
    return 0;
  }
};
#endif // !_COUNTER_H
