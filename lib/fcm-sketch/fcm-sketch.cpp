#ifndef _FCM_SKETCH_CPP
#define _FCM_SKETCH_CPP

#include "fcm-sketch.hpp"
#include "EM_FSD.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <vector>

uint32_t FCM_Sketch::hashing(FIVE_TUPLE key, uint32_t k) {
  return hash.run((const char *)key.num_array, sizeof(FIVE_TUPLE)) %
         this->stages_sz[k];
}

uint32_t FCM_Sketch::insert(FIVE_TUPLE tuple) {
  this->true_data[tuple]++;

  uint32_t hash_idx = this->hashing(tuple, 0);
  uint32_t c = 0;
  for (size_t s = 0; s < n_stages; s++) {
    Counter *curr_counter = &this->stages[s][hash_idx];
    if (curr_counter->increment()) {
      // Check for complete overflow
      if (s == n_stages - 1) {
        return 1;
      }
      c += curr_counter->count;
      hash_idx = hash_idx / this->k;
      continue;
    }
    c += curr_counter->count;
    if (c > this->hh_threshold) {
      this->HH_candidates.insert(tuple);
    }
    break;
  }
  return 1;
}

uint32_t FCM_Sketch::insert(FIVE_TUPLE tuple, uint32_t idx) {
  this->true_data[tuple]++;

  uint32_t hash_idx = idx;
  if (hash_idx >= this->stages_sz[0]) {
    std::cout << "Given idx out of range" << std::endl;
    exit(1);
  }
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
      this->HH_candidates.insert(tuple);
    }
    break;
  }
  return 1;
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
  std::cout << "[FCM] ------------------" << std::endl;
  for (size_t s = 0; s < n_stages; s++) {
    std::cout << "[FCM] Stage " << s << " with " << this->stages_sz[s]
              << " counters" << std::endl;
    std::cout << string(s * 3, ' ');
    for (size_t i = 0; i < this->stages_sz[s]; i++) {
      std::cout << this->stages[s][i].count << " ";
    }
    std::cout << std::endl;
  }
  std::cout << "[FCM] ------------------" << std::endl;
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

  // Heavy Hitter Detection (F1 Score)
  int true_pos = 0, false_pos = 0, true_neg = 0, false_neg = 0;

  // Flow Size Distribution (Weighted Mean Relative Error)
  using pair_type = decltype(this->true_data)::value_type;
  auto max_count =
      std::max_element(this->true_data.begin(), this->true_data.end(),
                       [](const pair_type &p1, const pair_type &p2) {
                         return p1.second < p2.second;
                       });

  vector<uint32_t> true_fsd(max_count->second + 1);
  double wmre = 0.0;

  for (const auto &[tuple, count] : this->true_data) {
    // // Flow Size Distribution (Weighted Mean Relative Error)
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

  // Cardinality
  double avgnum_empty_counter = 0;
  for (size_t i = 0; i < this->stages_sz[0]; i++) {

    if (this->stages[0][i].count == 0) {
      avgnum_empty_counter++;
    }
  }
  if (avgnum_empty_counter == 0) {
    printf("[CARD] No empty counters, set to 1 to prevent div by 0\n");
    avgnum_empty_counter = 1;
  }
  int em_cardinality =
      (int)(this->stages_sz[0] *
            std::log((double)this->stages_sz[0] / avgnum_empty_counter));
  double err_cardinality =
      std::abs(em_cardinality - (int)this->true_data.size()) /
      (double)this->true_data.size();

  printf("[CARD] RE of Cardinality : %2.6f (Est=%8.0d, true=%8.0d)\n",
         err_cardinality, em_cardinality, (int)this->true_data.size());

  long em_time = 0;
  // WMRE - Flow Size Distribution
  if (this->estimate_fsd) {
    auto start = std::chrono::high_resolution_clock::now();
    vector<double> em_fsd = this->get_distribution();

    uint32_t max_len = std::max(true_fsd.size(), em_fsd.size());
    true_fsd.resize(max_len);
    em_fsd.resize(max_len);

    for (size_t i = 0; i < max_len; i++) {
      wmre_nom += std::abs(double(true_fsd[i]) - em_fsd[i]);
      wmre_denom += (double(true_fsd[i]) + em_fsd[i]) / 2;
    }
    this->wmre = wmre_nom / wmre_denom;
    auto stop = std::chrono::high_resolution_clock::now();
    auto time = duration_cast<std::chrono::milliseconds>(stop - start);
    em_time = time.count();
    printf("[EM_FSD] WMRE : %f\n", this->wmre);
    printf("[EM_FSD] Total time %li ms\n", em_time);
  }
  // Save data into csv
  char csv[300];
  sprintf(csv, "%i,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%li,%i", epoch,
          this->average_relative_error, this->average_absolute_error,
          this->wmre, this->recall, this->precision, this->f1, em_time,
          this->em_iters);
  this->fcsv << csv << std::endl;
}

vector<double> FCM_Sketch::get_distribution() {
  uint32_t max_counter_value = 0;
  uint32_t max_degree = 0;
  // Summarize sketch and find collisions
  // stage, idx, (count, degree, min_value)
  vector<vector<vector<uint32_t>>> summary(this->n_stages);
  // stage, idx, layer, (collisions, min value)
  vector<vector<vector<vector<uint32_t>>>> overflow_paths(this->n_stages);

  // Setup sizes for summary and overflow_paths
  for (size_t stage = 0; stage < this->n_stages; stage++) {
    summary[stage].resize(this->stages_sz[stage], vector<uint32_t>(2, 0));
    overflow_paths[stage].resize(this->stages_sz[stage]);

    for (size_t i = 0; i < this->stages_sz[stage]; i++) {
      overflow_paths[stage][i].resize(stage + 1, vector<uint32_t>(2, 0));
    }
  }
  // Create virtual counters based on degree and count
  // degree, count value, n
  vector<vector<uint32_t>> virtual_counters(std::pow(this->k, this->n_stages));
  // degree, count value, layer, (collision, min counter value)
  vector<vector<vector<vector<uint32_t>>>> thresholds(
      std::pow(this->k, this->n_stages));

  for (size_t stage = 0; stage < this->n_stages; stage++) {
    for (size_t i = 0; i < this->stages_sz[stage]; i++) {
      summary[stage][i][0] = this->stages[stage][i].count;
      if (stage == 0) {
        summary[stage][i][1] = 1;
      }
      // If overflown increase the minimal value for the collisions
      if (this->stages[stage][i].overflow) {
        overflow_paths[stage][i][stage][1] = this->stages[stage][i].count;
      }

      // Start checking childeren from stage 1 and up
      if (stage > 0) {
        uint32_t overflown = 0;
        uint32_t imm_overflow = 0;
        bool child_overflown = false;
        // Loop over all childeren
        for (size_t k = 0; k < this->k; k++) {
          uint32_t child_idx = i * this->k + k;
          // Add childs count, degree and min_value to current counter
          if (this->stages[stage - 1][child_idx].overflow) {
            summary[stage][i][0] += summary[stage - 1][child_idx][0];
            summary[stage][i][1] += summary[stage - 1][child_idx][1];
            // If any of my predecessors have overflown, add them to my
            // overflown paths
            overflown++;
            child_overflown = true;
            for (size_t j = 0; j < overflow_paths[stage - 1][child_idx].size();
                 j++) {
              overflow_paths[stage][i][j][0] +=
                  overflow_paths[stage - 1][child_idx][j][0];
              overflow_paths[stage][i][j][1] =
                  overflow_paths[stage - 1][child_idx][j][1];
            }
          }
        }
        // If more than one has been overflown or my pred has overflown and I
        // have overflown, add me to the threshold as well
        if (overflown > 1 or child_overflown) {
          overflow_paths[stage][i][stage - 1][0] = overflown;
          overflow_paths[stage][i][stage][1] = summary[stage][i][0];
        }
      }

      // If not overflown and non-zero, we are at the end of the path
      if (!this->stages[stage][i].overflow && summary[stage][i][0] > 0) {
        uint32_t count = summary[stage][i][0];
        uint32_t degree = summary[stage][i][1] - 1;
        // Add entry to VC with its degree [1] and count [0]
        virtual_counters[degree].push_back(count);
        max_counter_value = std::max(max_counter_value, count);
        max_degree = std::max(max_degree, degree);

        // Remove zero thresholds
        for (size_t j = overflow_paths[stage][i].size() - 1; j > 0; j--) {
          if (overflow_paths[stage][i][j][0] == 0) {
            overflow_paths[stage][i].erase(overflow_paths[stage][i].begin() +
                                           j);
          }
        }
        std::reverse(overflow_paths[stage][i].begin(),
                     overflow_paths[stage][i].end());
        thresholds[degree].push_back(overflow_paths[stage][i]);
      }
    }
  }

  // for (size_t d = 0; d < thresholds.size(); d++) {
  //   if (thresholds[d].size() == 0) {
  //     continue;
  //   }
  //   std::cout << "Degree: " << d << std::endl;
  //   for (size_t i = 0; i < thresholds[d].size(); i++) {
  //     std::cout << "i " << i << ":";
  //     for (size_t l = 0; l < thresholds[d][i].size(); l++) {
  //       std::cout << "\t" << l;
  //       for (auto &col : thresholds[d][i][l]) {
  //         std::cout << " " << col;
  //       }
  //     }
  //     std::cout << std::endl;
  //   }
  // }

  // std::cout << std::endl;
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
  // std::cout << "Maximum degree is: " << max_degree << std::endl;
  // std::cout << "Maximum counter value is: " << max_counter_value <<
  // std::endl;

  EMFSD EM(this->stages_sz, thresholds, max_counter_value, max_degree,
           virtual_counters);
  for (size_t i = 0; i < this->em_iters; i++) {
    EM.next_epoch();
  }
  vector<double> output = EM.ns;
  return output;
}
#endif // !_FCM_SKETCH_CPP
