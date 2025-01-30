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

void WaterfallFCM::set_estimate_fsd(bool onoff) {
  this->fcm_sketches.estimate_fsd = onoff;
}

void WaterfallFCM::analyze(int epoch) {
  this->waterfall.analyze(epoch);
  this->true_data = this->waterfall.true_data;
  this->fcm_sketches.analyze(epoch);
  uint32_t waterfall_card = this->waterfall.tuples.size();
  std::cout << "[CHT] Cardinality : " << waterfall_card << std::endl;

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

    /*uint32_t max_len = std::max(true_fsd.size(), em_fsd.size());*/
    /*true_fsd.resize(max_len);*/
    /*em_fsd.resize(max_len);*/
    // std::cout << "[EM_FSD] True FSD : ";
    // for (size_t i = 0; i < true_fsd.size(); i++) {
    //   if (true_fsd[i] != 0) {
    //
    //     std::cout << i << " = " << true_fsd[i] << "\t";
    //   }
    // }
    // std::cout << std::endl;
    // std::cout << "[EM_FSD] Estimated FSD : ";
    // for (size_t i = 0; i < em_fsd.size(); i++) {
    //   if (em_fsd[i] != 0) {
    //     std::cout << i << " = " << em_fsd[i] << "\t";
    //   }
    // }
    // std::cout << std::endl;

    /*for (size_t i = 0; i < max_len; i++) {*/
    /*  wmre_nom += std::abs(double(true_fsd[i]) - em_fsd[i]);*/
    /*  wmre_denom += double((double(true_fsd[i]) + em_fsd[i]) / 2);*/
    /*}*/
    /*this->wmre = wmre_nom / wmre_denom;*/
    auto stop = std::chrono::high_resolution_clock::now();
    auto time = duration_cast<std::chrono::milliseconds>(stop - start);
    em_time = time.count();
    printf("[EM_FSD] WMRE : %f\n", this->wmre);
    printf("[EM_FSD] Total time %li ms\n", em_time);
  } else {
    this->wmre = this->fcm_sketches.wmre;
  }
  // Save data into c  // Save data into csv
  char csv[300];
  this->insertions = this->waterfall.insertions;
  this->f1_member = this->waterfall.f1;
  this->average_absolute_error = this->fcm_sketches.average_absolute_error;
  this->average_relative_error = this->fcm_sketches.average_relative_error;
  this->f1_hh = this->fcm_sketches.f1;
  sprintf(csv, "%i,%.3f,%.3f,%.3f,%.3f,%li,%i,%i,%.3f", epoch,
          this->average_relative_error, this->average_absolute_error,
          this->wmre, this->f1_hh, em_time, this->em_iters,
          this->waterfall.insertions, this->waterfall.f1);
  this->fcsv << csv << std::endl;
  return;
}

vector<uint32_t> WaterfallFCM::peel_sketches(set<TUPLE> &tuples) {

  vector<vector<uint32_t>> init_degree(DEPTH, vector<uint32_t>(W1, 0));
  vector<vector<vector<TUPLE>>> coll_tuples(DEPTH, vector<vector<TUPLE>>(W1));

  uint32_t cht_max_degree = 0;
  for (auto &tuple : tuples) {
    for (size_t d = 0; d < DEPTH; d++) {
      uint32_t hash_idx = this->fcm_sketches.hashing(tuple, d);
      init_degree[d][hash_idx]++;
      cht_max_degree = std::max(init_degree[d][hash_idx], cht_max_degree);

      coll_tuples[d][hash_idx].push_back(tuple);
    }
  }

  std::cout << "[WaterfallFCM] ...done!" << std::endl;
  for (size_t d = 0; d < DEPTH; d++) {
    std::cout << "Depth " << d << std::endl;
    for (size_t i = 0; i < init_degree[d].size(); i++) {
      /*if (init_degree[d][i] > 1) {*/
      /*  std::cout << i << ":" << init_degree[d][i] << " ";*/
      /*}*/
    }
    std::cout << std::endl;
  }

  std::cout << "[WaterfallFCM] Start collecting stage 0 1 degree counters "
               "for peeling"
            << std::endl;

  vector<uint32_t> n_remain = {0, 0};
  for (size_t d = 0; d < DEPTH; d++) {
    for (size_t i = 0; i < coll_tuples[d].size(); i++) {
      if (coll_tuples[d][i].empty()) {
        continue;
      }
      n_remain[d]++;
    }
  }

  std::cout << "--- Starting flows in coll_tuples " << n_remain[0] << ", "
            << n_remain[1] << " ---" << std::endl;

  vector<uint32_t> init_fsd(DEPTH);
  uint32_t d_curr = 0;
  uint32_t d_next = 1;
  // First perform for "perfected" matched flows; Peel flows that are singular
  // in both sketches
  uint32_t found = 0;
  do {
    found = 0;
    uint32_t t_val =
        this->fcm_sketches.lookup_sketch(coll_tuples[0][88705][0], 0);
    std::cout << "TESTING D0 88705 "
              << this->fcm_sketches.stages[0][0][88705].count << " tval "
              << t_val << " " << init_degree[0][88705] << " "
              << coll_tuples[0][88705].size() << std::endl;
    for (auto &t : coll_tuples[0][88705]) {
      std::cout << t << " real val " << this->true_data[t] << "\t";
    }
    std::cout << std::endl;
    for (auto &t : coll_tuples[0][88705]) {
      std::cout << t << " val at d1 "
                << this->fcm_sketches
                       .stages[1][0][this->fcm_sketches.hashing(t, 1)]
                       .count
                << "\t";
    }
    std::cout << std::endl;
    std::cout << "TESTING D1 471672 "
              << this->fcm_sketches.stages[1][0][471672].count << " "
              << init_degree[1][471672] << " " << coll_tuples[1][471672].size()
              << std::endl;
    for (auto &t : coll_tuples[1][471672]) {
      std::cout << t << " real val " << this->true_data[t] << "\t";
    }
    std::cout << std::endl;
    for (size_t i = 0; i < coll_tuples[d_curr].size(); i++) {
      if (i == 88705 && d_curr == 0) {
        std::cout << "Found 88705 at d_curr with tuple size "
                  << coll_tuples[d_curr][i].size() << " val "
                  << this->fcm_sketches.stages[0][0][1210].count << std::endl;
      }

      if (coll_tuples[d_curr][i].size() != 1) {
        continue;
      }

      TUPLE tuple = coll_tuples[d_curr][i][0];
      uint32_t val = this->fcm_sketches.lookup_sketch(tuple, d_curr);
      if (val > 254) {
        continue;
      }

      uint32_t i_next = this->fcm_sketches.hashing(tuple, d_next);
      uint32_t val_next = this->fcm_sketches.lookup_sketch(tuple, d_next);
      uint32_t degree = this->fcm_sketches.lookup_degree(tuple, d_curr);
      if (degree != 1) {
        continue;
      }

      // Count needs to be smaller otherwise there is a fault
      if (val > val_next) {
        std::cerr
            << "[ERROR] Counts is bigger than counter value in other sketch"
            << std::endl;
        printf("D%d,i:%zu,val:%d > D%d,i:%d,val:%d\n", d_curr, i, val, d_next,
               i_next, val_next);
        printf("D%d sz:%zu | D%d sz:%zu\n", d_curr,
               coll_tuples[d_curr][i].size(), d_next,
               coll_tuples[d_next][i_next].size());
        std::cerr << tuple << std::endl;
        exit(1);
      }

      if (std::find(coll_tuples[d_next][i_next].begin(),
                    coll_tuples[d_next][i_next].end(),
                    tuple) == coll_tuples[d_next][i_next].end()) {
        std::cout << "Could not find tuple in other depth " << d_next
                  << " found at depth " << d_curr << " with tuple: " << tuple
                  << std::endl;
        exit(1);
      }
      init_fsd.push_back(val);

      coll_tuples[d_curr][i].clear();
      size_t in = coll_tuples[d_next][i_next].size();
      coll_tuples[d_next][i_next].erase(
          std::find(coll_tuples[d_next][i_next].begin(),
                    coll_tuples[d_next][i_next].end(), tuple));
      size_t out = coll_tuples[d_next][i_next].size();
      if (in == out) {
        std::cerr << "[ERROR] Did not delete tuple!" << std::endl;
        printf("D%d,i:%zu,val:%d > D%d,i:%d,val:%d\n", d_curr, i, val, d_next,
               i_next, val_next);
        printf("D%d sz:%zu | D%d sz:%zu\n", d_curr,
               coll_tuples[d_curr][i].size(), d_next,
               coll_tuples[d_next][i_next].size());
        exit(1);
      }

      this->fcm_sketches.subtract(tuple, val);

      found++;
    }
    std::cout << "Found " << found << " counters in depth " << d_curr
              << std::endl;
    std::cout << "Total found " << init_fsd.size() << " total flows"
              << std::endl;

    std::cout << "--- Switching depths to " << d_next << std::endl;
    uint32_t tmp = d_curr;
    d_curr = d_next;
    d_next = tmp;

  } while (found > 0);

  n_remain = {0, 0};
  for (size_t d = 0; d < DEPTH; d++) {
    for (size_t i = 0; i < coll_tuples[d].size(); i++) {
      if (coll_tuples[d][i].empty()) {
        continue;
      }
      n_remain[d]++;
    }
  }
  std::cout << "Remaining unsolved flows: " << n_remain[0] + n_remain[1]
            << "\tSolved flows " << init_fsd.size() << std::endl;

  return init_fsd;
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

  double d_wmre = 0.0;
  for (size_t i = 0; i < this->em_iters; i++) {
    auto start = std::chrono::high_resolution_clock::now();
    EM.next_epoch();
    this->fcm_sketches.ns = EM.ns;
    auto stop = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<chrono::milliseconds>(stop - start);
    auto total_time =
        std::chrono::duration_cast<chrono::milliseconds>(stop - total_start);
    uint32_t max_len = std::max(true_fsd.size(), this->fcm_sketches.ns.size());
    true_fsd.resize(max_len);
    this->fcm_sketches.ns.resize(max_len);

    double wmre_nom = 0.0;
    double wmre_denom = 0.0;
    for (size_t i = 0; i < max_len; i++) {
      wmre_nom += std::abs(double(true_fsd[i]) - this->fcm_sketches.ns[i]);
      wmre_denom +=
          double((double(true_fsd[i]) + this->fcm_sketches.ns[i]) / 2);
    }
    wmre = wmre_nom / wmre_denom;
    std::cout << "[FCMS - EM WFCM iter " << i << "] intermediary wmre " << wmre
              << " delta: " << wmre - d_wmre << std::endl;
    d_wmre = wmre;
    // entropy initialization
    double entropy_err = 0;
    double entropy_est = 0;

    double tot_est = 0;
    double entr_est = 0;

    for (int i = 1; i < this->fcm_sketches.ns.size(); ++i) {
      if (this->fcm_sketches.ns[i] == 0)
        continue;
      tot_est += i * this->fcm_sketches.ns[i];
      entr_est += i * this->fcm_sketches.ns[i] * log2(i);
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

#endif // !_WATERFALL_FCM_CPP
