#ifndef _FCM_SKETCHES_CPP
#define _FCM_SKETCHES_CPP

#include "fcm-sketches.hpp"
#include "EMS_FSD.hpp"
#include "common.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <limits>
#include <sys/types.h>
#include <vector>

uint32_t FCM_Sketches::hashing(TUPLE key, uint32_t d) {
  return this->hash[d].run((const char *)key.num_array, key.sz) %
         this->stages_sz[0];
}

uint32_t FCM_Sketches::insert(TUPLE tuple) {
  this->true_data[tuple]++;

  for (size_t d = 0; d < this->depth; d++) {
    uint32_t c = 0;

    uint32_t hash_idx = this->hashing(tuple, d);
    for (size_t s = 0; s < n_stages; s++) {
      Counter *curr_counter = &this->stages[d][s][hash_idx];
      curr_counter->increment();
      c += curr_counter->count;
      if (curr_counter->overflow) {
        // Check for complete overflow
        if (s == n_stages - 1) {
          return 0;
        }
        hash_idx = uint32_t(hash_idx / this->k);
        continue;
      }

      if (c > 18000000) {
        std::cout << "Big number insert by tuples: " << tuple << std::endl;
      }
      if (c > 18100000) {
        exit(1);
      }
      if (c > this->hh_threshold) {
        this->HH_candidates.insert(tuple);
      }
      break;
    }
  }
  return 1;
}

uint32_t FCM_Sketches::insert(TUPLE tuple, uint32_t idx) {
  this->true_data[tuple]++;

  uint32_t hash_idx = idx;
  if (hash_idx >= this->stages_sz[0]) {
    std::cout << "Given idx out of range" << std::endl;
    exit(1);
  }
  for (size_t d = 0; d < this->depth; d++) {
    uint32_t c = 0;
    for (size_t s = 0; s < n_stages; s++) {
      Counter *curr_counter = &this->stages[d][s][hash_idx];
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
  }
  return 1;
}

// Lookup value in all sketches and return minimal value
uint32_t FCM_Sketches::lookup(TUPLE tuple) {
  uint32_t ret = std::numeric_limits<uint32_t>::max();
  for (size_t d = 0; d < this->depth; d++) {
    uint32_t c = 0;
    uint32_t hash_idx = this->hashing(tuple, d);

    for (size_t s = 0; s < n_stages; s++) {
      Counter *curr_counter = &this->stages[d][s][hash_idx];
      if (curr_counter->overflow) {
        // Check for complete overflow
        if (s == n_stages - 1) {
          ret = std::min(ret, c);
          break;
        }
        c += curr_counter->count;
        hash_idx = uint32_t(hash_idx / this->k);
        continue;
      }
      c += curr_counter->count;
      ret = std::min(ret, c);
      break;
    }
  }
  return ret;
}

// Lookup the count of a tuple at depth d
uint32_t FCM_Sketches::lookup_sketch(TUPLE tuple, uint32_t d) {
  uint32_t ret = std::numeric_limits<uint32_t>::max();
  uint32_t c = 0;
  uint32_t hash_idx = this->hashing(tuple, d);

  for (size_t s = 0; s < n_stages; s++) {
    Counter *curr_counter = &this->stages[d][s][hash_idx];
    if (curr_counter->overflow) {
      // Check for complete overflow
      if (s == n_stages - 1) {
        ret = std::min(ret, c);
        break;
      }
      c += curr_counter->count;
      hash_idx = uint32_t(hash_idx / this->k);
      continue;
    }
    c += curr_counter->count;
    ret = std::min(ret, c);
    break;
  }
  return ret;
}

// Lookup the degree of a tuple at depth d
uint32_t FCM_Sketches::lookup_degree(TUPLE tuple, uint32_t d) {
  uint32_t degree = 1;
  uint32_t hash_idx = this->hashing(tuple, d);

  for (size_t s = 0; s < this->n_stages; s++) {
    Counter *curr_counter = &this->stages[d][s][hash_idx];
    if (!curr_counter->overflow) {

      if (degree == 1 && s > 0) {
        std::cout << "Lookup degree at mid with tuple: " << tuple
                  << " and degree: " << degree << " at stage " << s
                  << std::endl;
      }
      return degree;
    }

    // Base idx for sibling counters
    hash_idx = uint32_t(hash_idx / this->k) * this->k;
    for (size_t i = hash_idx; i < hash_idx + this->k; i++) {
      Counter *sibling_counter = &this->stages[d][s][i];
      if (sibling_counter->overflow) {
        degree++;
      }
    }

    // Set hash_idx for next layer
    hash_idx = uint32_t(hash_idx / this->k);
  }
  std::cout << "Lookup degree at end with tuple: " << tuple << std::endl;
  return degree;
}

uint32_t FCM_Sketches::subtract(TUPLE tuple, uint32_t count) {
  for (size_t d = 0; d < this->depth; d++) {
    for (int s = n_stages - 1; s >= 0; s--) {
      // Get correct hash_idx for counter
      uint32_t hash_idx = this->hashing(tuple, d);
      for (int i = 0; i < s; i++) {
        hash_idx = uint32_t(hash_idx / this->k);
      }

      // Subtract count and exit if no remained
      Counter *curr_counter = &this->stages[d][s][hash_idx];
      uint32_t remain = curr_counter->decrement(count);
      if (!remain) {
        return 0;
      }
      count = remain;
    }
  }
  return 1;
}

void FCM_Sketches::print_sketch() {
  std::cout << "[FCM] ------------------" << std::endl;
  for (size_t d = 0; d < this->depth; d++) {
    std::cout << "[FCM] Depth " << d << std::endl;
    for (size_t s = 0; s < n_stages; s++) {
      std::cout << "[FCM] Stage " << s << " with " << this->stages_sz[s]
                << " counters" << std::endl;
      std::cout << string(s * 3, ' ');
      for (size_t i = 0; i < this->stages_sz[s]; i++) {
        std::cout << this->stages[d][s][i].count << " ";
      }
      std::cout << std::endl;
    }
  }
  std::cout << "[FCM] ------------------" << std::endl;
}

void FCM_Sketches::reset() {
  this->true_data.clear();
  this->HH_candidates.clear();

  for (auto &sketch : this->stages) {
    for (auto &stage : sketch) {
      for (auto &counter : stage) {
        counter.count = 0;
        counter.overflow = false;
      }
    }
  }
}

void FCM_Sketches::analyze(int epoch) {
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
    // Flow Size Distribution (Weighted Mean Relative Error)
    true_fsd[count]++;
    // Flow Size Estimation (Average Relative Error, Average Absolute Error)
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
  double empty_counters = 0;
  for (size_t d = 0; d < this->depth; d++) {
    for (size_t i = 0; i < this->stages_sz[0]; i++) {
      if (this->stages[d][0][i].count == 0) {
        empty_counters++;
      }
    }
  }
  if (empty_counters == 0) {
    printf("[CARD] No empty counters, set to 1 to prevent div by 0\n");
    empty_counters = 1;
  }

  int em_cardinality = (int)(W1 * std::log((double)W1 / empty_counters));
  double err_cardinality =
      std::abs(em_cardinality - (int)this->true_data.size()) /
      (double)this->true_data.size();

  printf("[CARD] RE of Cardinality : %2.6f (Est=%8.0d, true=%8.0d)\n",
         err_cardinality, em_cardinality, (int)this->true_data.size());

  // Flow Size Distribution (Weighted Mean Relative Error)
  if (this->estimate_fsd) {
    double wmre = this->get_distribution(true_fsd);
    std::cout << "[FCM Sketches] WMRE : " << wmre << std::endl;
  }
  if (!this->store_results) {
    return;
  }
  // Save data into csv
  char csv[300];
  sprintf(csv, "%.3f,%.3f,%.3f,%.3f,%.3f", this->average_relative_error,
          this->average_absolute_error, this->recall, this->precision,
          this->f1);
  this->fcsv << csv << std::endl;
}

double FCM_Sketches::get_distribution(vector<uint32_t> &true_fsd) {
  uint32_t max_counter_value = 0;
  uint32_t max_degree = 0;
  // Summarize sketch and find collisions
  // depth, stage, idx, (count, degree, min_value)
  vector<vector<vector<vector<uint32_t>>>> summary(this->n_stages);
  // depth, stage, idx, layer, (collisions, min value)
  vector<vector<vector<vector<vector<uint32_t>>>>> overflow_paths(
      this->n_stages);

  std::cout << "[EMS_FSD] Setting up summary and overflow paths..."
            << std::endl;
  // Setup sizes for summary and overflow_paths
  for (size_t d = 0; d < this->depth; d++) {
    summary[d].resize(this->n_stages);
    overflow_paths[d].resize(this->n_stages);
    for (size_t stage = 0; stage < this->n_stages; stage++) {
      summary[d][stage].resize(this->stages_sz[stage], vector<uint32_t>(2, 0));
      overflow_paths[d][stage].resize(this->stages_sz[stage]);

      for (size_t i = 0; i < this->stages_sz[stage]; i++) {
        overflow_paths[d][stage][i].resize(stage + 1, vector<uint32_t>(2, 0));
      }
    }
  }
  std::cout << "[EMS_FSD] ...done!" << std::endl;
  std::cout << "[EMS_FSD] Setting up virtual counters and thresholds..."
            << std::endl;
  // Create virtual counters based on degree and count
  // depth, degree, count value, n
  vector<vector<vector<uint32_t>>> virtual_counters;
  virtual_counters.resize(this->depth);
  for (size_t d = 0; d < this->depth; d++) {
    virtual_counters[d].resize(this->n_stages);
  }
  // depth, degree, count value, layer, (collision, min counter value)
  vector<vector<vector<vector<vector<uint32_t>>>>> thresholds;
  thresholds.resize(this->depth);
  for (size_t d = 0; d < this->depth; d++) {
    virtual_counters[d].resize(std::pow(this->k, this->n_stages));
    thresholds[d].resize(std::pow(this->k, this->n_stages));
  }

  std::cout << "[EMS_FSD] ...done!" << std::endl;
  std::cout << "[EMS_FSD] Load count from sketches into virtual counters and "
               "thresholds..."
            << std::endl;
  for (size_t d = 0; d < this->depth; d++) {
    for (size_t stage = 0; stage < this->n_stages; stage++) {
      for (size_t i = 0; i < this->stages_sz[stage]; i++) {
        summary[d][stage][i][0] = this->stages[d][stage][i].count;
        if (stage == 0) {
          summary[d][stage][i][1] = 1;
        }
        // If overflown increase the minimal value for the collisions
        if (this->stages[d][stage][i].overflow) {
          overflow_paths[d][stage][i][stage][1] =
              this->stages[d][stage][i].count;
        }

        // Start checking childeren from stage 1 and up
        if (stage > 0) {
          uint32_t overflown = 0;
          bool child_overflown = false;
          // Loop over all childeren
          for (size_t k = 0; k < this->k; k++) {
            uint32_t child_idx = i * this->k + k;
            // Add childs count, degree and min_value to current counter
            if (this->stages[d][stage - 1][child_idx].overflow) {
              summary[d][stage][i][0] += summary[d][stage - 1][child_idx][0];
              summary[d][stage][i][1] += summary[d][stage - 1][child_idx][1];
              // If any of my predecessors have overflown, add them to my
              // overflown paths
              overflown++;
              child_overflown = true;
              for (size_t j = 0;
                   j < overflow_paths[d][stage - 1][child_idx].size(); j++) {
                overflow_paths[d][stage][i][j][0] +=
                    overflow_paths[d][stage - 1][child_idx][j][0];
                overflow_paths[d][stage][i][j][1] =
                    overflow_paths[d][stage - 1][child_idx][j][1];
              }
            }
          }
          // If more than one has been overflown or my pred has overflown and I
          // have overflown, add me to the threshold as well
          if (overflown > 1 or child_overflown) {
            overflow_paths[d][stage][i][stage - 1][0] = overflown;
            overflow_paths[d][stage][i][stage][1] = summary[d][stage][i][0];
          }
        }

        // If not overflown and non-zero, we are at the end of the path
        if (!this->stages[d][stage][i].overflow &&
            summary[d][stage][i][0] > 0) {
          uint32_t count = summary[d][stage][i][0];
          uint32_t degree = summary[d][stage][i][1] - 1;
          // Add entry to VC with its degree [1] and count [0]
          virtual_counters[d][degree].push_back(count);
          max_counter_value = std::max(max_counter_value, count);
          max_degree = std::max(max_degree, degree);

          // Remove zero thresholds
          for (size_t j = overflow_paths[d][stage][i].size() - 1; j > 0; j--) {
            if (overflow_paths[d][stage][i][j][0] == 0) {
              overflow_paths[d][stage][i].erase(
                  overflow_paths[d][stage][i].begin() + j);
            }
          }
          std::reverse(overflow_paths[d][stage][i].begin(),
                       overflow_paths[d][stage][i].end());
          thresholds[d][degree].push_back(overflow_paths[d][stage][i]);
        }
      }
    }
  }

  std::cout << "[EMS_FSD] ...done!" << std::endl;
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

  std::cout << "[EMS_FSD] Initializing EMS_FSD..." << std::endl;
  EMSFSD EM(this->stages_sz, thresholds, max_counter_value, max_degree,
            this->depth, virtual_counters);
  std::cout << "[EMS_FSD] ...done!" << std::endl;
  auto total_start = std::chrono::high_resolution_clock::now();
  for (size_t i = 0; i < this->em_iters; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    EM.next_epoch();
    vector<double> ns = EM.ns;
    auto stop = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<chrono::milliseconds>(stop - start);
    auto total_time =
        std::chrono::duration_cast<chrono::milliseconds>(stop - total_start);
    uint32_t max_len = std::max(true_fsd.size(), ns.size());
    true_fsd.resize(max_len);
    ns.resize(max_len);

    double wmre_nom = 0.0;
    double wmre_denom = 0.0;
    for (size_t i = 0; i < max_len; i++) {
      wmre_nom += std::abs(double(true_fsd[i]) - ns[i]);
      wmre_denom += double((double(true_fsd[i]) + ns[i]) / 2);
    }
    wmre = wmre_nom / wmre_denom;

    if (!this->store_results) {
      continue;
    }

    char csv[300];
    sprintf(csv, "%u,%.3ld,%.3ld,%.3f,%.3f", this->em_iters, time.count(),
            total_time.count(), wmre, EM.n_new);
    this->fcsv_em << csv << std::endl;
  }
  return wmre;
}
#endif // !_FCM_SKETCHES_CPP
