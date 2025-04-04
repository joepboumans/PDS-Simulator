#ifndef _FCM_SKETCHES_CPP
#define _FCM_SKETCHES_CPP

#include "fcm-sketches.hpp"
#include "EM_FCMS_org.hpp"
#include "EM_GFCM.hpp"
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

  if (idx >= this->stages_sz[0]) {
    std::cout << "Given idx out of range" << std::endl;
    exit(1);
  }

  this->true_data[tuple]++;
  for (size_t d = 0; d < this->depth; d++) {
    uint32_t c = 0;
    uint32_t hash_idx = idx;
    for (size_t s = 0; s < n_stages; s++) {
      Counter *curr_counter = &this->stages[d][s][hash_idx];
      curr_counter->increment();
      c += curr_counter->count;
      if (curr_counter->overflow) {
        hash_idx = hash_idx / this->k;
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
    vector<uint32_t> hash_idxes = {hash_idx, 0, 0};
    hash_idxes[1] = uint32_t(hash_idx / this->k);
    hash_idxes[2] = uint32_t(hash_idxes[1] / this->k);

    for (int s = 0; s < this->n_stages; s++) {
      Counter *curr_counter = &this->stages[d][s][hash_idxes[s]];
      if (!curr_counter->overflow) {
        // Subtract count and exit if no remainder
        uint32_t remain = curr_counter->decrement(c);
        if (remain == 0 || s == 0) {
          break;
        } else {
          for (int i = s - 1; i >= 0; i--) {
            c = remain;
            Counter *counter = &this->stages[d][i][hash_idxes[i]];
            remain = counter->decrement(c);
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
      for (size_t i = 0; i < std::min(this->stages_sz[s], (uint32_t)300); i++) {
        std::cout << this->stages[d][s][i].count << " ";
      }
      std::cout << std::endl;
    }
  }
  std::cout << "[FCM] ------------------" << std::endl;
}

void FCM_Sketches::write2csv() {
  if (!this->store_results) {
    return;
  }
  // Save data into csv
  char csv[300];
  sprintf(csv, "%.5f,%.5f,%.4f,%.4f,%.4f", this->average_relative_error,
          this->average_absolute_error, this->recall, this->precision,
          this->f1);
  this->fcsv << csv << std::endl;
}

void FCM_Sketches::write2csv_em(uint32_t iter, size_t time, size_t total_time,
                                double card, double card_err, double true_sz,
                                double entropy) {
  if (not this->store_results) {
    return;
  }
  char csv[300];
  sprintf(csv, "%u,%ld,%ld,%.6f,%.1f,%.6f,%.1f,%.6f", iter, time, total_time,
          wmre, card, card_err, true_sz, entropy);
  this->fcsv_em << csv << std::endl;
}
void FCM_Sketches::write2csv_ns(vector<double> ns) {

  if (!this->store_results) {
    return;
  }
  uint32_t num_entries = 0;
  for (uint32_t i = 0; i < this->ns.size(); i++) {
    if (this->ns[i] != 0) {
      num_entries++;
    }
  }
  // Write NS FSD size and then the FSD as uint64_t
  this->fcsv_ns.write((char *)&num_entries, sizeof(num_entries));
  for (uint32_t i = 0; i < this->ns.size(); i++) {
    if (this->ns[i] != 0) {
      this->fcsv_ns.write((char *)&i, sizeof(i));
      this->fcsv_ns.write((char *)&this->ns[i], sizeof(this->ns[i]));
    }
  }
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
    std::cout << "[FCM Sketches] WMRE : " << this->wmre << std::endl;
  }

  this->write2csv();
  return;
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
          summary[d][i][w][3] = std::min(this->stages[d][0][w].count,
                                         (uint32_t)OVERFLOW_LEVEL1); // depth 0

          if (!this->stages[d][i][w].overflow) { // not overflow
            summary[d][i][w][0] = 1;

          } else { // if counter is overflow
            track_thres[d][i][w].push_back(
                vector<uint32_t>{0, 1, summary[d][i][w][3]});
          }
        } else if (i == 1) { // stage 2
          summary[d][i][w][2] = std::pow(K, i);
          for (int t = 0; t < K; ++t) {
            summary[d][i][w][1] += summary[d][i - 1][K * w + t][0] +
                                   summary[d][i - 1][K * w + t][1];
          }

          summary[d][i][w][3] =
              std::min(this->stages[d][i][w].count, (uint32_t)OVERFLOW_LEVEL2);

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
          }

          if (!this->stages[d][i][w].overflow) // non-overflow, end of path
          {
            summary[d][i][w][0] = summary[d][i][w][2] - summary[d][i][w][1];
          } else {
            // if overflow, then push new threshold <layer, #path, value>
            track_thres[d][i][w].push_back(
                vector<uint32_t>{i, summary[d][i][w][2] - summary[d][i][w][1],
                                 summary[d][i][w][3]});
          }
        } else if (i == 2) { // stage 3
          summary[d][i][w][3] = this->stages[d][i][w].count;
          summary[d][i][w][2] = std::pow(K, i);
          for (int t = 0; t < K; ++t) {
            summary[d][i][w][1] += summary[d][i - 1][K * w + t][0] +
                                   summary[d][i - 1][K * w + t][1];
          }

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
          }

          if (!this->stages[d][i][w].overflow) // non-overflow, end of path
          {
            summary[d][i][w][0] = summary[d][i][w][2] - summary[d][i][w][1];
          } else {
            // if overflow, then push new threshold <layer, #path, value>
            track_thres[d][i][w].push_back(
                vector<uint32_t>{i, summary[d][i][w][2] - summary[d][i][w][1],
                                 summary[d][i][w][3]});
            std::cerr << "[ERROR] Overflowing at L3 at " << i << std::endl;
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
    newsk[d].resize(std::pow(K, NUM_STAGES - 1) +
                    1); // maximum degree : k^(L-1) + 1 (except 0 index)
    newsk_thres[d].resize(std::pow(K, NUM_STAGES - 1) +
                          1); // maximum degree : k^(L-1) + 1 (except 0 index)
    for (int i = 0; i < NUM_STAGES; ++i) {
      for (int w = 0; w < this->stages_sz[i]; ++w) {
        if (i == 0) { // lowest level, degree 1
          if (summary[d][i][w][0] > 0 and
              summary[d][i][w][3] > 0) { // not full and nonzero

            newsk[d][summary[d][i][w][0]].push_back(summary[d][i][w][3]);
            newsk_thres[d][summary[d][i][w][0]].push_back(track_thres[d][i][w]);
          }
        } else { // upper level
          if (summary[d][i][w][0] >
              0) { // the highest node that paths could reach

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
  this->ns = em_fsd_algo.ns;
  uint32_t max_len = std::max(true_fsd.size(), this->ns.size());
  true_fsd.resize(max_len);
  this->ns.resize(max_len);

  double wmre_nom = 0.0;
  double wmre_denom = 0.0;
  for (size_t i = 0; i < max_len; i++) {
    wmre_nom += std::abs(double(true_fsd[i]) - this->ns[i]);
    wmre_denom += double((double(true_fsd[i]) + this->ns[i]) / 2);
  }
  this->wmre = wmre_nom / wmre_denom;
  std::cout << "[EM WFCM - Initial WMRE] wmre " << this->wmre
            << " delta: " << wmre - d_wmre << std::endl;
  d_wmre = wmre;
  // entropy initialization
  double entropy_err = 0;
  double entropy_est = 0;

  double tot_est = 0;
  double entr_est = 0;

  for (int i = 1; i < ns.size(); ++i) {
    if (ns[i] == 0)
      continue;
    tot_est += i * ns[i];
    entr_est += i * ns[i] * log2(i);
  }
  entropy_est = -entr_est / tot_est + log2(tot_est);

  double entropy_true = 0;
  double tot_true = 0;
  double entr_true = 0;
  for (int i = 0; i < true_fsd.size(); ++i) {
    if (true_fsd[i] == 0)
      continue;
    tot_true += i * true_fsd[i];
    entr_true += i * true_fsd[i] * log2(i);
  }
  entropy_true = -entr_true / tot_true + log2(tot_true);

  entropy_err = std::abs(entropy_est - entropy_true) / entropy_true;
  printf("[EM WFCM - Intitial Entropy Relative Error] (RE) = %f (true : %f, "
         "est : %f)\n",
         entropy_err, entropy_true, entropy_est);
  double err_cardinality =
      std::abs(em_fsd_algo.n_new - (int)this->true_data.size()) /
      (double)this->true_data.size();
  this->write2csv_em(0, 0, 0, em_fsd_algo.n_new, err_cardinality,
                     this->true_data.size(), entropy_err);

  for (size_t iter = 1; iter < this->em_iters + 1; iter++) {
    auto start = std::chrono::high_resolution_clock::now();
    em_fsd_algo.next_epoch();
    auto stop = std::chrono::high_resolution_clock::now();
    auto time = chrono::duration_cast<chrono::milliseconds>(stop - start);
    auto total_time =
        chrono::duration_cast<chrono::milliseconds>(stop - total_start);

    this->ns = em_fsd_algo.ns;
    this->write2csv_ns(ns);
    uint32_t max_len = std::max(true_fsd.size(), ns.size());
    true_fsd.resize(max_len);
    this->ns.resize(max_len);

    double wmre_nom = 0.0;
    double wmre_denom = 0.0;
    for (size_t i = 0; i < max_len; i++) {
      wmre_nom += std::abs(double(true_fsd[i]) - this->ns[i]);
      wmre_denom += double((double(true_fsd[i]) + this->ns[i]) / 2);
    }
    this->wmre = wmre_nom / wmre_denom;
    std::cout << "[FCMS - EM FSD iter " << iter << "] intermediary wmre "
              << this->wmre << " delta: " << wmre - d_wmre
              << " time : " << time.count() << std::endl;
    d_wmre = this->wmre;

    // entropy initialization
    double entropy_err = 0;
    double entropy_est = 0;

    double tot_est = 0;
    double entr_est = 0;

    for (int i = 1; i < ns.size(); ++i) {
      if (ns[i] == 0)
        continue;
      tot_est += i * ns[i];
      entr_est += i * ns[i] * log2(i);
    }
    entropy_est = -entr_est / tot_est + log2(tot_est);

    double entropy_true = 0;
    double tot_true = 0;
    double entr_true = 0;
    for (int i = 0; i < true_fsd.size(); ++i) {
      if (true_fsd[i] == 0)
        continue;
      tot_true += i * true_fsd[i];
      entr_true += i * true_fsd[i] * log2(i);
    }
    entropy_true = -entr_true / tot_true + log2(tot_true);

    entropy_err = std::abs(entropy_est - entropy_true) / entropy_true;
    printf("Entropy Relative Error (RE) = %f (true : %f, est : %f)\n",
           entropy_err, entropy_true, entropy_est);
    double err_cardinality =
        std::abs(em_fsd_algo.n_new - (int)this->true_data.size()) /
        (double)this->true_data.size();
    this->write2csv_em(iter, time.count(), total_time.count(),
                       em_fsd_algo.n_new, err_cardinality,
                       this->true_data.size(), entropy_err);
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

  std::cout << "[EM_GFCM] Setting up summary and overflow paths..."
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
  std::cout << "[EM_GFCM] ...done!" << std::endl;
  std::cout << "[EM_GFCM] Setting up virtual counters and thresholds..."
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

  std::cout << "[EM_GFCM] ...done!" << std::endl;
  std::cout << "[EM_GFCM] Load count from sketches into virtual counters and "
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
              // TODO: Adding thresholds like this does not account for
              // differences in layers: 254 + 254 + 65534 = 66042 is treated the
              // same as 254 + 65534 = 65788
              // Results in minor error max ~2.5%
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

  std::cout << "[EM_GFCM] ...done!" << std::endl;

  if (0) {
    // Print vc with thresholds
    for (size_t d = 0; d < DEPTH; d++) {
      for (size_t st = 0; st < virtual_counters[d].size(); st++) {
        if (virtual_counters[d][st].size() == 0) {
          continue;
        }
        for (size_t i = 0; i < virtual_counters[d][st].size(); i++) {
          printf("Depth %zu, Degree %zu, Index %zu ]= Val %d \tThresholds: ", d,
                 st, i, virtual_counters[d][st][i]);
          for (auto &t : thresholds[d][st][i]) {
            std::cout << "<";
            for (auto &x : t) {
              std::cout << x;
              if (&x != &t.back()) {
                std::cout << ", ";
              }
            }
            std::cout << ">";
            if (&t != &thresholds[d][st][i].back()) {
              std::cout << ", ";
            }
          }
          std::cout << std::endl;
        }
      }
    }
  }

  std::cout << "Maximum degree is: " << max_degree[0] << ", " << max_degree[1]
            << std::endl;
  std::cout << "Maximum counter value is: " << max_counter_value << std::endl;

  std::cout << "[EMS_FSD] Initializing EMS_FSD..." << std::endl;
  EM_GFCM EM(thresholds, max_counter_value, max_degree, virtual_counters);
  std::cout << "[EMS_FSD] ...done!" << std::endl;
  auto total_start = std::chrono::high_resolution_clock::now();

  double d_wmre = 0.0;
  this->ns = EM.ns;
  uint32_t max_len = std::max(true_fsd.size(), this->ns.size());
  true_fsd.resize(max_len);
  this->ns.resize(max_len);

  double wmre_nom = 0.0;
  double wmre_denom = 0.0;
  for (size_t i = 0; i < max_len; i++) {
    wmre_nom += std::abs(double(true_fsd[i]) - this->ns[i]);
    wmre_denom += double((double(true_fsd[i]) + this->ns[i]) / 2);
  }
  wmre = wmre_nom / wmre_denom;
  std::cout << "[EM WFCM - Initial WMRE] wmre " << wmre
            << " delta: " << wmre - d_wmre << std::endl;
  d_wmre = wmre;
  // entropy initialization
  double entropy_err = 0;
  double entropy_est = 0;

  double tot_est = 0;
  double entr_est = 0;

  for (int i = 1; i < ns.size(); ++i) {
    if (ns[i] == 0)
      continue;
    tot_est += i * ns[i];
    entr_est += i * ns[i] * log2(i);
  }
  entropy_est = -entr_est / tot_est + log2(tot_est);

  double entropy_true = 0;
  double tot_true = 0;
  double entr_true = 0;
  for (int i = 0; i < true_fsd.size(); ++i) {
    if (true_fsd[i] == 0)
      continue;
    tot_true += i * true_fsd[i];
    entr_true += i * true_fsd[i] * log2(i);
  }
  entropy_true = -entr_true / tot_true + log2(tot_true);

  entropy_err = std::abs(entropy_est - entropy_true) / entropy_true;
  printf("Intitial Entropy Relative Error (RE) = %f (true : %f, est : %f)\n",
         entropy_err, entropy_true, entropy_est);
  double err_cardinality = std::abs(EM.n_new - (int)this->true_data.size()) /
                           (double)this->true_data.size();
  this->write2csv_em(0, 0, 0, EM.n_new, err_cardinality, this->true_data.size(),
                     entropy_err);

  for (size_t iter = 1; iter < this->em_iters; iter++) {
    auto start = std::chrono::high_resolution_clock::now();
    EM.next_epoch();
    this->ns = EM.ns;
    this->write2csv_ns(ns);
    auto stop = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<chrono::milliseconds>(stop - start);
    auto total_time =
        std::chrono::duration_cast<chrono::milliseconds>(stop - total_start);
    uint32_t max_len = std::max(true_fsd.size(), ns.size());
    true_fsd.resize(max_len);
    this->ns.resize(max_len);

    double wmre_nom = 0.0;
    double wmre_denom = 0.0;
    for (size_t i = 0; i < max_len; i++) {
      wmre_nom += std::abs(double(true_fsd[i]) - this->ns[i]);
      wmre_denom += double((double(true_fsd[i]) + this->ns[i]) / 2);
    }
    wmre = wmre_nom / wmre_denom;
    std::cout << "[FCMS - EM GFCM iter " << iter << "] intermediary wmre "
              << wmre << " delta: " << wmre - d_wmre << std::endl;
    d_wmre = wmre;
    // entropy initialization
    double entropy_err = 0;
    double entropy_est = 0;

    double tot_est = 0;
    double entr_est = 0;

    for (int i = 1; i < ns.size(); ++i) {
      if (ns[i] == 0)
        continue;
      tot_est += i * ns[i];
      entr_est += i * ns[i] * log2(i);
    }
    entropy_est = -entr_est / tot_est + log2(tot_est);

    double entropy_true = 0;
    double tot_true = 0;
    double entr_true = 0;
    for (int i = 0; i < true_fsd.size(); ++i) {
      if (true_fsd[i] == 0)
        continue;
      tot_true += i * true_fsd[i];
      entr_true += i * true_fsd[i] * log2(i);
    }
    entropy_true = -entr_true / tot_true + log2(tot_true);

    entropy_err = std::abs(entropy_est - entropy_true) / entropy_true;
    printf("Entropy Relative Error (RE) = %f (true : %f, est : %f)\n",
           entropy_err, entropy_true, entropy_est);

    double err_cardinality = std::abs(EM.n_new - (int)this->true_data.size()) /
                             (double)this->true_data.size();
    this->write2csv_em(iter, time.count(), total_time.count(), EM.n_new,
                       err_cardinality, this->true_data.size(), entropy_err);
  }
  return wmre;
}
#endif // !_FCM_SKETCHES_CPP
