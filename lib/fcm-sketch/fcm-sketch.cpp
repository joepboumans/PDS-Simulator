#ifndef _FCM_SKETCH_CPP
#define _FCM_SKETCH_CPP

#include "fcm-sketch.h"
#include <cstdint>
#include <map>

uint32_t FCM_Sketch::hashing(FIVE_TUPLE key, uint32_t k) {
  char c_ftuple[sizeof(FIVE_TUPLE)];
  memcpy(c_ftuple, &key, sizeof(FIVE_TUPLE));
  return hash.run(c_ftuple, sizeof(FIVE_TUPLE)) % this->stages_sz[k];
}

uint32_t FCM_Sketch::insert(FIVE_TUPLE tuple) {
  this->true_data[(string)tuple]++;

  for (size_t s = 0; s < n_stages; s++) {
    uint32_t hash_idx = this->hashing(tuple, s);
    uint32_t c = 0;
    Counter *curr_counter = &this->stages[s][hash_idx];
    if (curr_counter->overflow) {
      // Check for complete overflow
      if (s == n_stages - 1) {
        return 1;
      }
      c += curr_counter->count;
      // hash_idx = hash_idx >> this->k;
      continue;
    }
    curr_counter->increment();
    c += curr_counter->count;
    if (c > this->hh_threshold) {
      this->HH_candidates.insert((string)tuple);
    }
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
  this->HH_candidates.clear();

  for (auto &stage : this->stages) {
    for (auto &counter : stage) {
      counter.count = 0;
      counter.overflow = false;
    }
  }
}

void FCM_Sketch::analyze(int epoch) {
  // Use lookup to find tuples
  this->average_absolute_error = 0;
  this->average_relative_error = 0;
  double n = this->true_data.size();

  // std::cout << std::endl;
  // Heavy Hitter Detection (F1 Score)
  int true_pos = 0, false_pos = 0, true_neg = 0, false_neg = 0;

  // Flow Size Distribution (Weighted Mean Relative Error)
  double wmre = 0.0;
  map<uint32_t, uint32_t> true_fsd;
  map<uint32_t, uint32_t> e_fsd;

  for (const auto &[s_tuple, count] : this->true_data) {
    // // Flow Size Distribution (Weighted Mean Relative Error)
    // true_fsd[count]++;
    // e_fsd[this->lookup(s_tuple)]++;
    //
    // // Flow Size Estimation (Average Relative Error, Average Absolute Error)
    // FIVE_TUPLE tuple(s_tuple);
    // int diff = count - this->lookup(tuple);
    //
    // this->average_absolute_error += std::abs(diff);
    // this->average_relative_error += ((double)std::abs(diff) / count);

    // Heavy Hitter Detection (F1 Score)
    if (count > this->hh_threshold) {
      if (auto search = this->HH_candidates.find(s_tuple);
          search != this->HH_candidates.end()) {
        true_pos++;
      } else {
        false_neg++;
      }
      continue;
    }
    if (auto search = this->HH_candidates.find(s_tuple);
        search == this->HH_candidates.end()) {
      true_neg++;
    } else {
      false_pos++;
    }
  }
  //
  // this->average_absolute_error = this->average_absolute_error / n;
  // this->average_relative_error = this->average_relative_error / n;

  // Heavy Hitter Detection (F1 Score)
  if (true_pos == 0 && false_pos == 0) {
    this->recall = 1.0;
  } else {
    this->recall = (double)true_pos / (true_pos + false_pos);
  }
  if (true_neg == 0 && false_neg == 0) {
    this->precision = 1.0;
  } else {
    this->precision = (double)true_neg / (true_neg + false_neg);
  }
  this->f1 = 2 * ((recall * precision) / (precision + recall));

  // char msg[200];
  // sprintf(msg, "\tTP:%i\tFP:%i\tRecall:%.3f\tPrecision:%.3f\tF1:%.3f",
  // true_pos,
  //         false_pos, this->recall, this->precision, this->f1);
  // std::cout << epoch << msg << std::endl;
  // Save data into csv
  char csv[300];
  sprintf(csv, "%i,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f", epoch,
          this->average_relative_error, this->average_absolute_error,
          this->wmre, this->recall, this->precision, this->f1);
  this->fcsv << csv << std::endl;
}
#endif // !_FCM_SKETCH_CPP
