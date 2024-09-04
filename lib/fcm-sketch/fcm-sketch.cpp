#ifndef _FCM_SKETCH_CPP
#define _FCM_SKETCH_CPP

#include "fcm-sketch.hpp"
#include "EM_FSD.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <map>
#include <vector>

uint32_t FCM_Sketch::hashing(FIVE_TUPLE key, uint32_t k) {
  static char c_ftuple[sizeof(FIVE_TUPLE)];
  memcpy(c_ftuple, &key, sizeof(FIVE_TUPLE));
  return hash.run(c_ftuple, sizeof(FIVE_TUPLE)) % this->stages_sz[k];
}

uint32_t FCM_Sketch::insert(FIVE_TUPLE tuple) {
  this->true_data[(string)tuple]++;

  uint32_t hash_idx = this->hashing(tuple, 0);
  uint32_t c = 0;
  for (size_t s = 0; s < n_stages; s++) {
    Counter *curr_counter = &this->stages[s][hash_idx];
    if (curr_counter->overflow) {
      // Check for complete overflow
      if (s == n_stages - 1) {
        return 1;
      }
      c += curr_counter->count;
      hash_idx = hash_idx / this->k;
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

uint32_t FCM_Sketch::lookup(FIVE_TUPLE tuple) {
  uint32_t hash_idx = this->hashing(tuple, 0);
  uint32_t c = 0;
  for (size_t s = 0; s < n_stages; s++) {
    Counter *curr_counter = &this->stages[s][hash_idx];
    if (curr_counter->overflow) {
      // Check for complete overflow
      if (s == n_stages - 1) {
        return -1;
      }
      c += curr_counter->count;
      hash_idx = hash_idx / this->k;
      continue;
    }
    c += curr_counter->count;
    return c;
  }
  return -1;
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
  vector<uint32_t> true_fsd;

  for (const auto &[tuple, count] : this->true_data) {
    // // Flow Size Distribution (Weighted Mean Relative Error)
    if (true_fsd.size() < count) {
      true_fsd.resize(count + 1);
    }
    true_fsd[count]++;
    // // Flow Size Estimation (Average Relative Error, Average Absolute Error)
    int diff = count - this->lookup(tuple);

    this->average_absolute_error += std::abs(diff);
    this->average_relative_error += ((double)std::abs(diff) / count);

    // Heavy Hitter Detection (F1 Score)
    if (count > this->hh_threshold) {
      if (auto search = this->HH_candidates.find(tuple);
          search != this->HH_candidates.end()) {
        true_pos++;
      } else {
        false_neg++;
      }
      continue;
    }
    if (auto search = this->HH_candidates.find(tuple);
        search == this->HH_candidates.end()) {
      true_neg++;
    } else {
      false_pos++;
    }
  }
  this->average_absolute_error = this->average_absolute_error / n;
  this->average_relative_error = this->average_relative_error / n;

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

  this->wmre = 0.0;
  double wmre_nom = 0.0;
  double wmre_denom = 0.0;
  // WMRE - Flow Size Distribution
  vector<double> em_fsd = this->get_distribution();
  for (size_t i = 0; i < true_fsd.size(); i++) {
    if (true_fsd[i] == 0) {
      continue;
    }
    wmre_nom += std::abs(true_fsd[i] - em_fsd[i]);
    wmre_denom += double(true_fsd[i] + em_fsd[i]) / 2;
  }
  this->wmre = wmre_nom / wmre_denom;
  printf("WMRD : %f\n", this->wmre);
  printf("True FSD size : %zu\n", true_fsd.size());
  // char msg[200];
  // sprintf(msg,
  //         "\tTP:%i\tFP:%i\tRecall:%.3f\tPrecision:%.3f\tF1:%.3f\tAAE:%.3f\tARE:"
  //         "%.3f",
  //         true_pos, false_pos, this->recall, this->precision, this->f1,
  //         this->average_absolute_error, this->average_relative_error);
  // std::cout << msg;
  // Save data into csv
  char csv[300];
  sprintf(csv, "%i,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f", epoch,
          this->average_relative_error, this->average_absolute_error,
          this->wmre, this->recall, this->precision, this->f1);
  this->fcsv << csv << std::endl;
}

vector<double> FCM_Sketch::get_distribution() {
  // stage, idx, (count, degree)
  vector<vector<vector<uint32_t>>> summary(this->n_stages);
  // stage, idx of entry, idx of merging paths/predecessors
  vector<vector<vector<uint32_t>>> collision_paths(this->n_stages);
  // degree, idx, count
  vector<vector<uint32_t>> virtual_counters(std::pow(this->k, this->n_stages));
  uint32_t max_counter_value = 0;
  uint32_t max_degree = 0;

  for (size_t stage = 0; stage < this->n_stages; stage++) {
    summary[stage].resize(this->stages_sz[stage], vector<uint32_t>(2, 0));
    collision_paths[stage].resize(this->stages_sz[stage]);

    for (size_t i = 0; i < this->stages_sz[stage]; i++) {
      summary[stage][i][0] = this->stages[stage][i].count;
      summary[stage][i][1] = 1;

      if (stage > 0) {
        // Add overflow from previous stages
        for (size_t k = 0; k < this->k; k++) {
          uint32_t child_idx = i * this->k + k;
          if (this->stages[stage - 1][child_idx].overflow) {
            // Add childs degree and count
            summary[stage][i][1] += summary[stage - 1][child_idx][1];
            summary[stage][i][0] += summary[stage - 1][child_idx][0];
            collision_paths[stage][i].push_back(child_idx);
          }
        }
      }

      // If not overflowing and not 0, add to VCs with degree
      if (!this->stages[stage][i].overflow && summary[stage][i][0] > 0) {
        virtual_counters[summary[stage][i][1]].push_back(summary[stage][i][0]);
        max_counter_value = std::max(max_counter_value, summary[stage][i][0]);
        max_degree = std::max(max_degree, summary[stage][i][1]);
      }
    }
  }

  // for (size_t st = 0; st < virtual_counters.size(); st++) {
  //   if (virtual_counters[st].size() == 0) {
  //     continue;
  //   }
  //   std::cout << "Degree: " << st << std::endl;
  //   for (auto &val : virtual_counters[st]) {
  //     std::cout << " " << val;
  //   }
  //   std::cout << std::endl;
  // }

  EMFSD_ld EM;
  EM.set_counters(max_counter_value, max_degree, virtual_counters,
                  this->stages_sz[0]);
  EM.next_epoch();
  return EM.ns;
}
#endif // !_FCM_SKETCH_CPP
