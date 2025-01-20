#ifndef _FCM_SKETCHES_CPP
#define _FCM_SKETCHES_CPP

#include "fcm-sketches.hpp"
#include "EM_FCMS_org.hpp"
#include "EM_WFCM.hpp"
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
  return this->hash[d].run((const char *)key.num_array, this->tuple_sz) %
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
        hash_idx = uint32_t(hash_idx / this->k);
        continue;
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
  uint32_t c = 0;
  uint32_t hash_idx = this->hashing(tuple, d);

  for (size_t s = 0; s < n_stages; s++) {
    Counter *curr_counter = &this->stages[d][s][hash_idx];
    if (curr_counter->overflow) {
      c += curr_counter->count;
      hash_idx = uint32_t(hash_idx / this->k);
      continue;
    }
    c += curr_counter->count;
    break;
  }
  return c;
}

uint32_t FCM_Sketches::lookup_degree_rec(uint32_t hash_idx, uint32_t d,
                                         uint32_t s) {
  if (s == 0) {
    return 1;
  }

  uint32_t degree = 0;
  hash_idx = uint32_t(hash_idx / this->k);
  for (size_t i = hash_idx; i < hash_idx + this->k; i++) {
    Counter *child_counter = &this->stages[d][s - 1][i];
    if (child_counter->overflow) {
      degree += this->lookup_degree_rec(i, d, s - 1);
    }
  }

  return degree;
}

// Lookup the degree of a tuple at depth d
uint32_t FCM_Sketches::lookup_degree(TUPLE tuple, uint32_t d) {
  uint32_t degree = 0;
  uint32_t init_idx = this->hashing(tuple, d);
  uint32_t hash_idx = init_idx;
  uint32_t val = this->lookup_sketch(tuple, d);

  for (size_t s = 0; s < this->n_stages; s++) {
    Counter *curr_counter = &this->stages[d][s][hash_idx];
    if (!curr_counter->overflow) {
      degree = std::max(degree, (uint32_t)1);
      /*std::cout << "Lookup degree at mid with tuple: " << tuple*/
      /*          << " degree: " << degree << " and value " << val <<
       * std::endl;*/
      return degree;
    }

    uint32_t base_idx = uint32_t(hash_idx / this->k) * this->k;
    for (size_t i = base_idx; i < base_idx + this->k; i++) {
      Counter *sibling_counter = &this->stages[d][s - 1][i];
      if (sibling_counter->overflow) {
        degree++;
      }
    }
  }

  degree = std::max(degree, (uint32_t)1);
  std::cout << "Lookup degree at end with tuple: " << tuple
            << " degree: " << degree << " and value " << val << std::endl;
  return degree;
}

uint32_t FCM_Sketches::subtract(TUPLE tuple, uint32_t count) {
  for (size_t d = 0; d < this->depth; d++) {
    uint32_t c = count;
    uint32_t hash_idx = this->hashing(tuple, d);
    if (hash_idx == 88705 && d == 0) {
      Counter *curr_counter = &this->stages[d][0][hash_idx];
      std::cout << "Found 88705 in subtracting " << count
                << " with tuple: " << tuple << std::endl;
      std::cout << 0 << " Has a value of " << curr_counter->count << std::endl;
    }
    vector<uint32_t> hash_idxes = {hash_idx, 0, 0};
    hash_idxes[1] = uint32_t(hash_idx / this->k);
    hash_idxes[2] = uint32_t(hash_idxes[1] / this->k);

    for (int s = 0; s < this->n_stages; s++) {
      Counter *curr_counter = &this->stages[d][s][hash_idxes[s]];
      if (!curr_counter->overflow) {
        // Subtract count and exit if no remainder
        if (hash_idxes[s] == uint32_t(88705 / this->k) && d == 0) {
          std::cout << s << " Has a value of " << curr_counter->count
                    << std::endl;
        }
        uint32_t remain = curr_counter->decrement(c);
        if (hash_idxes[s] == uint32_t(88705 / this->k) && d == 0) {
          std::cout << s << " Has a value of " << curr_counter->count
                    << std::endl;
        }
        if (remain == 0 || s == 0) {
          break;
        } else {
          for (int i = s - 1; i >= 0; i--) {
            c = remain;
            Counter *counter = &this->stages[d][i][hash_idxes[i]];
            if (hash_idxes[i] == uint32_t(88705 / this->k) && d == 0) {
              std::cout << i << " Has a value of " << counter->count
                        << std::endl;
            }
            remain = counter->decrement(c);
            if (hash_idxes[i] == uint32_t(88705 / this->k) && d == 0) {
              std::cout << i << " Has a value of " << counter->count
                        << std::endl;
            }
            if (remain == 0) {
              break;
            }
          }
          break;
        }
      }
    }
  }
  return 0;
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
    this->wmre = 0.0;

    if (this->estimator_org) {
      this->wmre = this->get_distribution(true_fsd);
    } else {
      this->wmre = this->get_distribution_Waterfall(true_fsd);
    }
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

  /* Making summary note for conversion algorithm (4-tuple summary)
                 Each dimension is for (tree, layer, width, tuple)
                 */
  vector<vector<vector<vector<uint32_t>>>> summary(DEPTH);

  /* To track how paths are merged along the layers */
  vector<vector<vector<vector<vector<uint32_t>>>>> track_thres(DEPTH);

  /*
          When tracking paths, save <a,b,c> in track_thres, where
          a : layer (where paths have met)
          b : number of path that have met (always less or equal than degree of
     virtual counter) c : accumulated max-count value of overflowed registers
     (e.g., 254, 65534+254, 65534+254+254, etc)
          */

  std::cout << "[FCMS] Start setting up summary and thresholds" << std::endl;
  for (int d = 0; d < DEPTH; ++d) {
    summary[d].resize(NUM_STAGES);
    track_thres[d].resize(NUM_STAGES);
    for (uint32_t i = 0; i < NUM_STAGES; ++i) {
      summary[d][i].resize(
          this->stages_sz[i],
          vector<uint32_t>(4, 0)); // { Total Degree, Previous Degrees, Local
                                   // Degree , count}
      track_thres[d][i].resize(this->stages_sz[i]); // initialize
      for (int w = 0; w < this->stages_sz[i]; ++w) {
        if (i == 0) { // stage 1
          summary[d][i][w][2] = 1;
          /*summary[d][i][w][2] =*/
          /*    std::max(init_degree[d][w], (uint32_t)1); // default*/
          summary[d][i][w][3] = std::min(this->stages[d][0][w].count,
                                         (uint32_t)OVERFLOW_LEVEL1); // depth 0

          if (!this->stages[d][0][w].overflow) { // not overflow
            summary[d][i][w][0] = summary[d][i][w][2];

          } else { // if counter is overflow
            track_thres[d][i][w].push_back(
                vector<uint32_t>{0, summary[d][i][w][2], summary[d][i][w][3]});
          }
        } else if (i == 1) { // stage 2
          summary[d][i][w][3] =
              std::min(this->stages[d][1][w].count, (uint32_t)OVERFLOW_LEVEL2);

          summary[d][i][w][2] = 0;
          for (int t = 0; t < K; ++t) {
            // if child is overflow, then accumulate both "value" and
            // "threshold"
            if (!this->stages[d][i - 1][K * w + t]
                     .overflow) { // if child is overflow
              continue;
            }
            summary[d][i][w][3] += summary[d][i - 1][K * w + t][3];
            track_thres[d][i][w].insert(
                track_thres[d][i][w].end(),
                track_thres[d][i - 1][K * w + t].begin(),
                track_thres[d][i - 1][K * w + t].end());

            summary[d][i][w][1] += summary[d][i - 1][K * w + t][0];
            summary[d][i][w][2] += 1;
          }

          if (!this->stages[d][1][w].overflow) // non-overflow, end of path
          {
            summary[d][i][w][0] = summary[d][i][w][2];
          } else {
            // if overflow, then push new threshold <layer, #path, value>
            track_thres[d][i][w].push_back(
                vector<uint32_t>{i, summary[d][i][w][2], summary[d][i][w][3]});
          }
        } else if (i == 2) { // stage 3
          summary[d][i][w][3] = this->stages[d][2][w].count;
          summary[d][i][w][2] = 0;

          for (int t = 0; t < K; ++t) {
            // if child is overflow, then accumulate both "value" and
            // "threshold"
            if (!this->stages[d][i - 1][K * w + t]
                     .overflow) { // if child is overflow
              continue;
            }
            summary[d][i][w][3] += summary[d][i - 1][K * w + t][3];
            track_thres[d][i][w].insert(
                track_thres[d][i][w].end(),
                track_thres[d][i - 1][K * w + t].begin(),
                track_thres[d][i - 1][K * w + t].end());

            summary[d][i][w][1] += summary[d][i - 1][K * w + t][0];
            summary[d][i][w][2] += 1;
          }

          if (!this->stages[d][i][w].overflow) // non-overflow, end of path
          {
            summary[d][i][w][0] = summary[d][i][w][1];
          } else {
            // if overflow, then push new threshold <layer, #path, value>
            track_thres[d][i][w].push_back(
                vector<uint32_t>{i, summary[d][i][w][2], summary[d][i][w][3]});
          }

        } else {
          printf("[ERROR] DEPTH(%d) is not mathcing with allocated counter "
                 "arrays...\n",
                 DEPTH);
          return -1;
        }
      }
    }
  }

  // make new sketch with specific degree, (depth, degree, value)
  vector<vector<vector<uint32_t>>> newsk(DEPTH);

  // (depth, degree, vector(layer)<vector(#.paths)<threshold>>>)
  vector<vector<vector<vector<vector<uint32_t>>>>> newsk_thres(DEPTH);

  std::cout << "[FCMS] Transform summary and thresh into newsk and "
               "newsk_thresh"
            << std::endl;

  for (int d = 0; d < DEPTH; ++d) {
    // size = all possible degree
    newsk[d].resize(std::pow(K, NUM_STAGES - 1) * 3 +
                    1); // maximum degree : k^(L-1) + 1 (except 0 index)
    newsk_thres[d].resize(std::pow(K, NUM_STAGES - 1) * 3 +
                          1); // maximum degree : k^(L-1) + 1 (except 0 index)
    for (int i = 0; i < NUM_STAGES; ++i) {
      for (int w = 0; w < this->stages_sz[i]; ++w) {
        if (i == 0) { // lowest level, degree 1
          if (summary[d][i][w][0] > 0 and
              summary[d][i][w][3] > 0) { // not full and nonzero

            if (summary[d][i][w][0] >= newsk[d].size()) {
              newsk[d].resize(summary[d][i][w][0] + 1);
              newsk_thres[d].resize(summary[d][i][w][0] + 1);
            }

            newsk[d][summary[d][i][w][0]].push_back(summary[d][i][w][3]);
            newsk_thres[d][summary[d][i][w][0]].push_back(track_thres[d][i][w]);
          }
        } else { // upper level
          if (summary[d][i][w][0] >
              0) { // the highest node that paths could reach

            if (summary[d][i][w][0] >= newsk[d].size()) {
              newsk[d].resize(summary[d][i][w][0] + 1);
              newsk_thres[d].resize(summary[d][i][w][0] + 1);
            }
            newsk[d][summary[d][i][w][0]].push_back(summary[d][i][w][3]);
            newsk_thres[d][summary[d][i][w][0]].push_back(track_thres[d][i][w]);
          }
        }
      }
    }
  }

  std::cout << "[FCMS] Finished setting up newsk and "
               "newsk_thresh"
            << std::endl;
  // just for debugging, 1 for print, 0 for not print.
  if (0) {
    int maximum_val = 0;
    for (int d = 0; d < DEPTH; ++d) {
      for (int i = 0; i < newsk[d].size(); ++i) {
        if (newsk[d][i].size() > 0) {
          printf("degree : %d - %lu\n", i, newsk[d][i].size());
          if (newsk_thres[d][i].size() != newsk[d][i].size()) {
            printf("[Error] newsk and newsk_thres sizes are different!!!\n\n");
            return -1;
          }
          for (int j = 0; j < newsk[d][i].size(); ++j) {
            printf("[Depth:%d, Degree:%d, index:%d] ==> ", d, i, j);
            printf("val:%d //", newsk[d][i][j]);
            for (int k = 0; k < newsk_thres[d][i][j].size(); ++k) {
              printf("<%d, %d, %d>, ", newsk_thres[d][i][j][k][0],
                     newsk_thres[d][i][j][k][1], newsk_thres[d][i][j][k][2]);
            }
            maximum_val =
                (maximum_val < newsk[d][i][j] ? newsk[d][i][j] : maximum_val);
            printf("\n");
          }
        }
      }
      printf("[Depth : %d] Maximum counter value : %d\n\n\n", d, maximum_val);
    }
  }

  EM_FCMS_org<DEPTH, W1, OVERFLOW_LEVEL1, OVERFLOW_LEVEL2> em_fsd_algo; // new

  /* now, make the distribution of each degree */
  em_fsd_algo.set_counters(newsk, newsk_thres); // new

  auto total_start = std::chrono::high_resolution_clock::now();
  std::cout << "Initialized EM_FSD, starting estimation..." << std::endl;
  double d_wmre = 0.0;
  for (size_t i = 0; i < this->em_iters; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    em_fsd_algo.next_epoch();
    auto stop = std::chrono::high_resolution_clock::now();
    auto time = chrono::duration_cast<chrono::milliseconds>(stop - start);
    auto total_time =
        chrono::duration_cast<chrono::milliseconds>(stop - total_start);

    vector<double> ns = em_fsd_algo.ns;
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
    std::cout << "[FCMS - EM FSD iter " << i << "] intermediary wmre " << wmre
              << " delta: " << wmre - d_wmre << std::endl;
    d_wmre = wmre;
    // Save data into csv
    char csv[300];
    sprintf(csv, "%zu,%.3ld,%.3ld,%.3f,%.3f", i, time.count(),
            total_time.count(), wmre, em_fsd_algo.n_new);
    this->fcsv_em << csv << std::endl;

    // Write NS FSD size and then the FSD as uint64_t
    uint32_t ns_size = ns.size();
    this->fcsv_ns.write((char *)&ns_size, sizeof(ns_size));
    for (uint32_t i = 0; i < ns.size(); i++) {
      if (ns[i] != 0) {
        this->fcsv_ns.write((char *)&i, sizeof(i));
        this->fcsv_ns.write((char *)&ns[i], sizeof(ns[i]));
      }
    }
  }

  return wmre;
}

double FCM_Sketches::get_distribution_Waterfall(vector<uint32_t> &true_fsd) {
  uint32_t max_counter_value = 0;
  vector<uint32_t> max_degree = {0, 0};
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
        if (stage == 0) {
          summary[d][stage][i][1] = 1;
        }
        // If overflown increase the minimal value for the collisions
        if (this->stages[d][stage][i].overflow) {
          summary[d][stage][i][0] = this->stages[d][stage][i].max_count;
          overflow_paths[d][stage][i][stage][1] =
              this->stages[d][stage][i].max_count;
        } else {

          summary[d][stage][i][0] = this->stages[d][stage][i].count;
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
          uint32_t degree = summary[d][stage][i][1];
          // Add entry to VC with its degree [1] and count [0]
          virtual_counters[d][degree].push_back(count);
          max_counter_value = std::max(max_counter_value, count);
          max_degree[d] = std::max(max_degree[d], degree);

          std::reverse(overflow_paths[d][stage][i].begin(),
                       overflow_paths[d][stage][i].end());
          thresholds[d][degree].push_back(overflow_paths[d][stage][i]);
        }
      }
    }
  }

  std::cout << "[EMS_FSD] ...done!" << std::endl;
  /*for (size_t d = 0; d < DEPTH; d++) {*/
  /*  for (size_t xi = 0; xi < thresholds[d].size(); xi++) {*/
  /*    if (thresholds[d][xi].size() == 0) {*/
  /*      continue;*/
  /*    }*/
  /*    std::cout << "Degree: " << xi << std::endl;*/
  /*    for (size_t i = 0; i < thresholds[d][xi].size(); i++) {*/
  /*      std::cout << "i " << i << ":";*/
  /*      for (size_t l = 0; l < thresholds[d][xi][i].size(); l++) {*/
  /*        std::cout << "\t" << l;*/
  /*        for (auto &col : thresholds[d][xi][i][l]) {*/
  /*          std::cout << " " << col;*/
  /*        }*/
  /*      }*/
  /*      std::cout << std::endl;*/
  /*    }*/
  /*  }*/
  /**/
  /*  std::cout << std::endl;*/
  /*}*/
  /**/
  /*for (size_t d = 0; d < DEPTH; d++) {*/
  /*  for (size_t st = 0; st < virtual_counters[d].size(); st++) {*/
  /*    if (virtual_counters[d][st].size() == 0) {*/
  /*      continue;*/
  /*    }*/
  /*    std::cout << "Degree: " << st << std::endl;*/
  /*    for (auto &val : virtual_counters[d][st]) {*/
  /*      std::cout << " " << val;*/
  /*    }*/
  /*    std::cout << std::endl;*/
  /*  }*/
  /*}*/
  std::cout << "Maximum degree is: " << max_degree[0] << ", " << max_degree[1]
            << std::endl;
  std::cout << "Maximum counter value is: " << max_counter_value << std::endl;

  std::cout << "[EMS_FSD] Initializing EMS_FSD..." << std::endl;
  EM_WFCM EM(thresholds, max_counter_value, max_degree, virtual_counters);
  std::cout << "[EMS_FSD] ...done!" << std::endl;
  auto total_start = std::chrono::high_resolution_clock::now();

  double d_wmre = 0.0;
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
    std::cout << "[FCMS - EM WFCM iter " << i << "] intermediary wmre " << wmre
              << " delta: " << wmre - d_wmre << std::endl;
    d_wmre = wmre;

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
