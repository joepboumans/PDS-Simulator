#ifndef _WATERFALL_FCM_CPP
#define _WATERFALL_FCM_CPP

#include "waterfall-fcm.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <vector>

uint32_t WaterfallFCM::insert(FIVE_TUPLE tuple) {
  this->cuckoo.insert(tuple);
  this->fcm.insert(tuple);
  return 0;
}
uint32_t WaterfallFCM::lookup(FIVE_TUPLE tuple) {
  uint32_t member = this->cuckoo.lookup(tuple);
  uint32_t count = this->fcm.lookup(tuple);
  if (member) {
    return count;
  }
  return 0;
}

void WaterfallFCM::reset() {
  this->cuckoo.reset();
  this->fcm.reset();
}

void WaterfallFCM::analyze(int epoch) {
  this->cuckoo.analyze(epoch);
  return;
}

void WaterfallFCM::print_sketch() {
  this->cuckoo.print_sketch();
  this->fcm.print_sketch();
}
#endif // !_WATERFALL_FCM_CPP
