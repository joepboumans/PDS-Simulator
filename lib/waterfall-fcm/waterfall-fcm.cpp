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
  this->fcm.analyze(epoch);
  // Save data into csv
  char csv[300];
  this->insertions = this->cuckoo.insertions;
  this->f1_member = this->cuckoo.f1;
  this->average_absolute_error = this->fcm.average_absolute_error;
  this->average_relative_error = this->fcm.average_relative_error;
  this->wmre = this->fcm.wmre;
  this->f1_hh = this->fcm.f1;
  sprintf(csv, "%i,%.3f,%.3f,%.3f,%.3f,%i,%.3f", epoch,
          this->average_relative_error, this->average_absolute_error,
          this->wmre, this->f1_hh, this->cuckoo.insertions, this->cuckoo.f1);
  this->fcsv << csv << std::endl;
  return;
}

void WaterfallFCM::get_distribution(set<FIVE_TUPLE> tuples) {}

void WaterfallFCM::print_sketch() {
  this->cuckoo.print_sketch();
  this->fcm.print_sketch();
}

void WaterfallFCM::set_estimate_fsd(bool onoff) {
  this->fcm.estimate_fsd = onoff;
}
#endif // !_WATERFALL_FCM_CPP
