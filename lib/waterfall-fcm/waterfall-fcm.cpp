#ifndef _WATERFALL_FCM_CPP
#define _WATERFALL_FCM_CPP

#include "waterfall-fcm.hpp"
#include "EM_WFCM.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <vector>

uint32_t WaterfallFCM::insert(TUPLE tuple) {
  this->waterfall.insert(tuple);
  this->fcm_sketches.insert(tuple);
  return 0;
}

uint32_t WaterfallFCM::insert(TUPLE tuple, uint32_t idx) {
  // Store initial degree with idx for debugging
  if (this->idx_degree.empty()) {
    this->idx_degree = vector<uint32_t>(this->fcm_sketches.stages_sz[0]);
  }
  TUPLE tmp;
  if (not this->waterfall.lookup(tuple)) {
    std::cout << tuple << " at idx " << idx << std::endl;
    this->idx_degree[idx]++;
  }

  this->waterfall.insert(tuple);
  this->fcm_sketches.insert(tuple, idx);

  return 0;
}

uint32_t WaterfallFCM::lookup(TUPLE tuple) {
  uint32_t member = this->waterfall.lookup(tuple);
  uint32_t count = this->fcm_sketches.lookup(tuple);
  if (member) {
    return count;
  }
  return 0;
}

void WaterfallFCM::reset() {
  this->waterfall.reset();
  this->fcm_sketches.reset();
}

void WaterfallFCM::print_sketch() {
  this->waterfall.print_sketch();
  this->fcm_sketches.print_sketch();
}

void WaterfallFCM::write2csv() {
  if (!this->store_results) {
    return;
  }
  // Save data into csv
  char csv[300];
  this->insertions = this->waterfall.insertions;
  this->f1_member = this->waterfall.f1;
  this->average_absolute_error = this->fcm_sketches.average_absolute_error;
  this->average_relative_error = this->fcm_sketches.average_relative_error;
  this->f1_hh = this->fcm_sketches.f1;
  sprintf(csv, "%.3f,%.3f,%.3f,%.3f,%i,%.3f", this->average_relative_error,
          this->average_absolute_error, this->wmre, this->f1_hh,
          this->waterfall.insertions, this->waterfall.f1);
  this->fcsv << csv << std::endl;
}

void WaterfallFCM::write2csv_em(uint32_t iter, size_t time, size_t total_time,
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

void WaterfallFCM::write2csv_ns() {
  if (!this->store_results) {
    return;
  }
  uint32_t num_entries = 0;
  for (uint32_t i = 0; i < this->fcm_sketches.ns.size(); i++) {
    if (this->fcm_sketches.ns[i] != 0) {
      num_entries++;
    }
  }
  // Write NS FSD size and then the FSD as uint64_t
  this->fcsv_ns.write((char *)&num_entries, sizeof(num_entries));
  for (uint32_t i = 0; i < this->fcm_sketches.ns.size(); i++) {
    if (this->fcm_sketches.ns[i] != 0) {
      this->fcsv_ns.write((char *)&i, sizeof(i));
      this->fcsv_ns.write((char *)&this->fcm_sketches.ns[i],
                          sizeof(this->fcm_sketches.ns[i]));
    }
  }
  std::cout << std::endl;
}

void WaterfallFCM::set_estimate_fsd(bool onoff) {
  this->fcm_sketches.estimate_fsd = onoff;
}

void WaterfallFCM::analyze(int epoch) {
  this->waterfall.analyze(epoch);
  this->true_data = this->waterfall.true_data;
  this->fcm_sketches.analyze(epoch);
  uint32_t waterfall_card = this->waterfall.tuples.size();
  double err_cardinality =
      std::abs((int)waterfall_card - (int)this->true_data.size()) /
      (double)this->true_data.size();
  std::cout << "[Waterfall] Cardinality : " << waterfall_card
            << " Card Error : " << err_cardinality << std::endl;

  long em_time = 0;
  // Flow Size Distribution (Weighted Mean Relative Error)
  if (this->estimate_fsd) {
    using pair_type = decltype(this->fcm_sketches.true_data)::value_type;
    auto max_count =
        std::max_element(this->fcm_sketches.true_data.begin(),
                         this->fcm_sketches.true_data.end(),
                         [](const pair_type &p1, const pair_type &p2) {
                           return p1.second < p2.second;
                         });

    vector<uint32_t> true_fsd(max_count->second + 1);

    for (const auto &[tuple, count] : this->fcm_sketches.true_data) {
      true_fsd[count]++;
    }

    this->wmre = 0.0;
    auto start = std::chrono::high_resolution_clock::now();
    this->wmre = this->get_distribution(this->waterfall.tuples, true_fsd);
    auto stop = std::chrono::high_resolution_clock::now();
    auto time = duration_cast<std::chrono::milliseconds>(stop - start);

    em_time = time.count();
    printf("[EM_FSD] WMRE : %f\n", this->wmre);
    printf("[EM_FSD] Total time %li ms\n", em_time);
  } else {
    this->wmre = this->fcm_sketches.wmre;
  }

  this->write2csv();
  return;
}

// Gets maps of flows to the first counter layer of FCM
vector<vector<uint32_t>> WaterfallFCM::get_initial_degrees(set<TUPLE> tuples) {
  if (not this->idx_degree.empty()) {
    std::cout << "[WaterfallFCM] Insertion happened with idx, use stored "
                 "values as initial degree"
              << std::endl;
    vector<vector<uint32_t>> init_degree(DEPTH, vector<uint32_t>(W1));
    for (size_t d = 0; d < init_degree.size(); d++) {
      init_degree[d] = this->idx_degree;
    }
    std::cout << "[WaterfallFCM] ...done!" << std::endl;
    for (size_t d = 0; d < DEPTH; d++) {
      std::cout << "Depth " << d << std::endl;
      for (size_t i = 0; i < init_degree.size(); i++) {
        if (init_degree[d][i] == 0) {
          continue;
        }
        std::cout << i << ":" << init_degree[d][i] << " ";
      }
      std::cout << std::endl;
    }
    return init_degree;
  }

  std::cout << "[WaterfallFCM] Calculate initial degrees from Waterfall..."
            << std::endl;
  vector<vector<uint32_t>> init_degree(DEPTH, vector<uint32_t>(W1));

  for (size_t d = 0; d < DEPTH; d++) {
    for (auto &tuple : tuples) {
      uint32_t hash_idx = this->fcm_sketches.hashing(tuple, d);
      init_degree[d][hash_idx]++;
    }
  }
  std::cout << "[WaterfallFCM] ...done!" << std::endl;
  for (size_t d = 0; d < DEPTH; d++) {
    std::cout << "Depth " << d << std::endl;
    for (size_t i = 0; i < init_degree.size(); i++) {
      if (init_degree[d][i] == 0) {
        continue;
      }
      std::cout << i << ":" << init_degree[d][i] << " ";
    }
    std::cout << std::endl;
  }
  return init_degree;
}

double WaterfallFCM::calculate_wmre(vector<double> &ns,
                                    vector<uint32_t> &true_fsd) {
  static uint32_t iter = 1;
  static double d_wmre = 0.0;
  double wmre_nom = 0.0;
  double wmre_denom = 0.0;
  uint32_t max_len = std::max(true_fsd.size(), ns.size());
  true_fsd.resize(max_len);
  ns.resize(max_len);

  for (size_t i = 0; i < max_len; i++) {
    wmre_nom += std::abs(double(true_fsd[i]) - ns[i]);
    wmre_denom += double((double(true_fsd[i]) + ns[i]) / 2);
  }
  wmre = wmre_nom / wmre_denom;
  std::cout << "[EM WFCM - iter " << iter << "] intermediary wmre " << wmre
            << " delta: " << wmre - d_wmre << std::endl;
  d_wmre = wmre;
  iter++;
  return wmre;
}

double WaterfallFCM::calculate_entropy(vector<double> &ns,
                                       vector<uint32_t> &true_fsd) {
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
  return entropy_err;
}

double WaterfallFCM::get_distribution(set<TUPLE> &tuples,
                                      vector<uint32_t> &true_fsd) {

  vector<vector<uint32_t>> init_degree = get_initial_degrees(tuples);
  uint32_t max_counter_value = 0;
  vector<uint32_t> max_degree = {0, 0};
  // Summarize sketch and find collisions
  // depth, stage, idx, (count, total degree, sketch degree)
  vector<vector<vector<vector<uint32_t>>>> summary(this->n_stages);
  // Create virtual counters based on degree and count
  // depth, degree, count, value, collisions
  vector<vector<vector<uint32_t>>> virtual_counters;
  // Base for EM
  vector<vector<vector<uint32_t>>> sketch_degrees;
  vector<vector<uint32_t>> init_fsd(DEPTH);
  // depth, stage, idx, layer, vector<stage, local degree, total degree, min
  // value>
  vector<vector<vector<vector<vector<uint32_t>>>>> overflow_paths(
      this->n_stages);
  // depth, degree, count, value, vector<stage, local collisions, total
  // collisions, min value>
  vector<vector<vector<vector<vector<uint32_t>>>>> thresholds;

  std::cout << "[EM_WFCM] Setting up summary and overflow paths..."
            << std::endl;
  // Setup sizes for summary and overflow_paths
  for (size_t d = 0; d < DEPTH; d++) {
    summary[d].resize(this->n_stages);
    overflow_paths[d].resize(this->n_stages);
    for (size_t stage = 0; stage < this->n_stages; stage++) {
      summary[d][stage].resize(this->fcm_sketches.stages_sz[stage],
                               vector<uint32_t>(3, 0));
      overflow_paths[d][stage].resize(this->fcm_sketches.stages_sz[stage]);
    }
  }
  std::cout << "[EM_WFCM] ...done!" << std::endl;
  std::cout << "[EM_WFCM] Setting up virtual counters and thresholds..."
            << std::endl;

  virtual_counters.resize(DEPTH);
  sketch_degrees.resize(DEPTH);
  thresholds.resize(DEPTH);

  std::cout << "[EM_WFCM] ...done!" << std::endl;
  std::cout << "[EM_WFCM] Load count from sketches into virtual counters and "
               "thresholds..."
            << std::endl;
  for (size_t d = 0; d < DEPTH; d++) {
    for (size_t stage = 0; stage < this->n_stages; stage++) {
      for (size_t i = 0; i < this->fcm_sketches.stages_sz[stage]; i++) {
        if (this->fcm_sketches.stages[d][stage][i].count <= 0) {
          continue;
        }

        // If overflown increase the minimal value for the collisions
        summary[d][stage][i][0] = this->fcm_sketches.stages[d][stage][i].count;
        if (this->fcm_sketches.stages[d][stage][i].overflow) {
          summary[d][stage][i][0] =
              this->fcm_sketches.stages[d][stage][i].max_count;
        }

        // Store local and initial degree
        if (stage == 0) {
          summary[d][stage][i][1] = 1;
          summary[d][stage][i][2] = init_degree[d][i];
        } else if (stage > 0) { // Start checking childeren from stage 1 and up
          for (size_t k = 0; k < K; k++) {
            uint32_t child_idx = i * K + k;

            // Add childs count, total and sketch degree to current counter
            if (this->fcm_sketches.stages[d][stage - 1][child_idx].overflow) {
              summary[d][stage][i][0] += summary[d][stage - 1][child_idx][0];
              summary[d][stage][i][1] += summary[d][stage - 1][child_idx][1];
              summary[d][stage][i][2] += summary[d][stage - 1][child_idx][2];

              // Add overflow path of child to my own path
              for (auto &path : overflow_paths[d][stage - 1][child_idx]) {
                overflow_paths[d][stage][i].push_back(path);
              }
            }
          }
        }

        // If not overflown and non-zero, we are at the end of the path
        if (!this->fcm_sketches.stages[d][stage][i].overflow &&
            summary[d][stage][i][0] > 0) {

          uint32_t count = summary[d][stage][i][0];
          uint32_t sketch_degree = summary[d][stage][i][1];
          uint32_t degree = summary[d][stage][i][2];
          max_counter_value = std::max(max_counter_value, count);
          max_degree[d] = std::max(max_degree[d], degree);

          if (max_counter_value >= init_fsd[d].size()) {
            init_fsd[d].resize(max_counter_value + 2);
          }

          if (degree == 1) {
            init_fsd[d][count]++;
          } else if (degree == count) {
            init_fsd[d][1] += count;
          } else if (degree + 1 == count) {
            init_fsd[d][1] += (count - 1);
            init_fsd[d][2] += 1;
          } else {
            if (degree >= virtual_counters[d].size()) {
              virtual_counters[d].resize(degree + 1);
              thresholds[d].resize(degree + 1);
              sketch_degrees[d].resize(degree + 1);
            }

            // Separate L1 VC's as these do not require thresholds to solve.
            // Store L1 VC in degree 0
            if (sketch_degree == 1) {
              sketch_degree = degree;
              degree = 1;
            }
            // Add entry to VC with its degree [1] and count [0]
            virtual_counters[d][degree].push_back(count);
            sketch_degrees[d][degree].push_back(sketch_degree);

            thresholds[d][degree].push_back(overflow_paths[d][stage][i]);
          }

        } else {
          uint32_t max_val = summary[d][stage][i][0];
          uint32_t local_degree = summary[d][stage][i][1];
          uint32_t total_degree = summary[d][stage][i][2];
          vector<uint32_t> local = {static_cast<uint32_t>(stage), local_degree,
                                    total_degree, max_val};
          overflow_paths[d][stage][i].push_back(local);
        }
      }
    }
  }

  std::cout << "[EM_WFCM] ...done!" << std::endl;

  if (0) {
    // Print vc with thresholds
    for (size_t d = 0; d < DEPTH; d++) {
      for (size_t st = 0; st < virtual_counters[d].size(); st++) {
        if (virtual_counters[d][st].size() == 0) {
          continue;
        }
        for (size_t i = 0; i < virtual_counters[d][st].size(); i++) {
          printf("Depth %zu, Degree %zu, Sketch Degree %u, Index %zu ]= Val "
                 "%d \tThresholds: ",
                 d, st, sketch_degrees[d][st][i], i,
                 virtual_counters[d][st][i]);
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
      for (size_t i = 0; i < init_fsd[d].size(); i++) {
        if (init_fsd[d][i] != 0) {
          printf("Depth %zu, Index %zu ]= Val %d\n", d, i, init_fsd[d][i]);
        }
      }
    }
  }

  std::cout << "Maximum degree is: " << max_degree[0] << ", " << max_degree[1]
            << std::endl;
  std::cout << "Maximum counter value is: " << max_counter_value << std::endl;

  std::cout << "[EMS_FSD] Initializing EMS_FSD..." << std::endl;
  EM_WFCM EM(thresholds, max_counter_value, max_degree, virtual_counters,
             sketch_degrees, init_fsd);
  std::cout << "[EMS_FSD] ...done!" << std::endl;
  auto total_start = std::chrono::high_resolution_clock::now();

  this->wmre = calculate_wmre(EM.ns, true_fsd);
  double entropy_err = calculate_entropy(EM.ns, true_fsd);
  double err_cardinality = std::abs(EM.n_new - (int)this->true_data.size()) /
                           (double)this->true_data.size();
  this->write2csv_em(0, 0, 0, EM.n_new, err_cardinality, this->true_data.size(),
                     entropy_err);

  for (size_t iter = 1; iter < this->em_iters + 1; iter++) {
    auto start = std::chrono::high_resolution_clock::now();
    EM.next_epoch();
    this->fcm_sketches.ns = EM.ns;
    this->write2csv_ns();
    auto stop = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<chrono::milliseconds>(stop - start);
    auto total_time =
        std::chrono::duration_cast<chrono::milliseconds>(stop - total_start);

    this->wmre = calculate_wmre(EM.ns, true_fsd);
    double entropy_err = calculate_entropy(EM.ns, true_fsd);
    err_cardinality = std::abs(EM.n_new - (int)this->true_data.size()) /
                      (double)this->true_data.size();
    write2csv_em(iter, time.count(), total_time.count(), EM.n_new,
                 err_cardinality, this->true_data.size(), entropy_err);
  }

  return wmre;
}

#endif // !_WATERFALL_FCM_CPP
