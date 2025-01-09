#ifndef _Q_WATERFALL_CPP
#define _Q_WATERFALL_CPP

#include "qwaterfall-fcm.hpp"
#include "EM_FSD_QWATER.hpp"
#include "common.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sys/types.h>

template class qWaterfall_Fcm<FIVE_TUPLE, fiveTupleHash>;
template class qWaterfall_Fcm<FLOW_TUPLE, flowTupleHash>;

template <typename TUPLE, typename HASH>
uint32_t qWaterfall_Fcm<TUPLE, HASH>::insert(TUPLE tuple) {
  TUPLE empty;
  if (tuple == empty) {
    return 1;
  }
  this->qwaterfall.insert(tuple);
  this->fcm_sketches.insert(tuple);
  return 0;
}

template <typename TUPLE, typename HASH>
uint32_t qWaterfall_Fcm<TUPLE, HASH>::lookup(TUPLE tuple) {
  uint32_t member = this->qwaterfall.lookup(tuple);
  uint32_t count = this->fcm_sketches.lookup(tuple);
  if (member) {
    return count;
  }
  return 0;
}

template <typename TUPLE, typename HASH>
void qWaterfall_Fcm<TUPLE, HASH>::reset() {
  this->qwaterfall.reset();
  this->fcm_sketches.reset();
}

template <typename TUPLE, typename HASH>
void qWaterfall_Fcm<TUPLE, HASH>::set_estimate_fsd(bool onoff) {
  this->fcm_sketches.estimate_fsd = onoff;
}

template <typename TUPLE, typename HASH>
void qWaterfall_Fcm<TUPLE, HASH>::print_sketch() {
  this->qwaterfall.print_sketch();
  this->fcm_sketches.print_sketch();
}

template <typename TUPLE, typename HASH>
void qWaterfall_Fcm<TUPLE, HASH>::analyze(int epoch) {
  this->qwaterfall.analyze(epoch);
  this->fcm_sketches.analyze(epoch);

  long em_time = 0;
  // Flow Size Distribution (Weighted Mean Relative Error)
  if (this->estimate_fsd) {
    std::cout << "[qWaterfall_Fcm] Start FSD estimation..." << std::endl;
    using pair_type = decltype(this->fcm_sketches.true_data)::value_type;
    auto max_count =
        std::max_element(this->fcm_sketches.true_data.begin(),
                         this->fcm_sketches.true_data.end(),
                         [](const pair_type &p1, const pair_type &p2) {
                           return p1.second < p2.second;
                         });

    vector<uint32_t> true_fsd(max_count->second + 1);
    double wmre = 0.0;

    for (const auto &[tuple, count] : this->fcm_sketches.true_data) {
      true_fsd[count]++;
    }

    uint32_t true_max = *std::max_element(true_fsd.begin(), true_fsd.end());
    std::cout << "[qWaterfall_Fcm] True maximum counter values: " << true_max
              << std::endl;

    /*auto start = std::chrono::high_resolution_clock::now();*/
    /*this->wmre = this->calculate_fsd(this->qwaterfall.tuples, true_fsd);*/
    /*auto stop = std::chrono::high_resolution_clock::now();*/
    /*auto time = duration_cast<std::chrono::milliseconds>(stop - start);*/
    /**/
    /*em_time = time.count();*/
    /*printf("[EM_FSD] WMRE : %f\n", this->wmre);*/
    /*printf("[EM_FSD] Total time %li ms\n", em_time);*/

    auto start = std::chrono::high_resolution_clock::now();
    this->wmre = this->calculate_fsd_peeling(this->qwaterfall.tuples, true_fsd);
    auto stop = std::chrono::high_resolution_clock::now();
    auto time = duration_cast<std::chrono::milliseconds>(stop - start);

    em_time = time.count();
    printf("[EM_FSD] Peeling WMRE : %f\n", this->wmre);
    printf("[EM_FSD] Peeling time %li ms\n", em_time);
  } else {
    this->wmre = this->fcm_sketches.wmre;
  }
  // Save data into c  // Save data into csv
  char csv[300];
  this->insertions = this->qwaterfall.insertions;
  this->f1_member = this->qwaterfall.f1;
  this->average_absolute_error = this->fcm_sketches.average_absolute_error;
  this->average_relative_error = this->fcm_sketches.average_relative_error;
  this->f1_hh = this->fcm_sketches.f1;
  sprintf(csv, "%i,%.3f,%.3f,%.3f,%.3f,%li,%i,%i,%.3f", epoch,
          this->average_relative_error, this->average_absolute_error,
          this->wmre, this->f1_hh, em_time, this->em_iters,
          this->qwaterfall.insertions, this->qwaterfall.f1);
  this->fcsv << csv << std::endl;
  return;
}

template <typename TUPLE, typename HASH>
double qWaterfall_Fcm<TUPLE, HASH>::calculate_fsd(set<TUPLE> &tuples,
                                                  vector<uint32_t> &true_fsd) {
  // Setup initial degrees for each input counter (stage 0)
  std::cout << "[qWaterfall_Fcm] Calculate initial degrees from qWaterfall..."
            << std::endl;
  vector<vector<uint32_t>> init_degree(DEPTH, vector<uint32_t>(W1));
  uint32_t cht_max_degree = 0;
  for (size_t d = 0; d < DEPTH; d++) {
    for (auto &tuple : tuples) {
      uint32_t hash_idx = this->fcm_sketches.hashing(tuple, d);
      init_degree[d][hash_idx]++;
      cht_max_degree = std::max(init_degree[d][hash_idx], cht_max_degree);
    }
  }
  std::cout << "[qWaterfall_Fcm] ...done!" << std::endl;
  for (size_t d = 0; d < DEPTH; d++) {
    std::cout << "Depth " << d << std::endl;
    for (size_t i = 0; i < init_degree.size(); i++) {
      std::cout << i << ":" << init_degree[d][i] << " ";
    }
    std::cout << std::endl;
  }

  uint32_t max_counter_value = 0;
  // Summarize sketch and find collisions
  // depth, stage, idx, (count, degree, min_value)
  vector<vector<vector<vector<uint32_t>>>> summary(DEPTH);
  // depth, stage, idx, layer, (stage, total degree/collisions, local
  // collisions, min value)
  vector<vector<vector<vector<vector<uint32_t>>>>> overflow_paths(DEPTH);

  // Setup sizes for summary and overflow_paths
  for (size_t d = 0; d < DEPTH; d++) {
    summary[d].resize(NUM_STAGES);
    overflow_paths[d].resize(NUM_STAGES);
    for (size_t stage = 0; stage < NUM_STAGES; stage++) {
      summary[d][stage].resize(this->fcm_sketches.stages_sz[stage],
                               vector<uint32_t>(3, 0));
      overflow_paths[d][stage].resize(this->fcm_sketches.stages_sz[stage]);
    }
  }

  // Create virtual counters based on degree and count
  // degree, count value, n
  vector<vector<vector<uint32_t>>> virtual_counters(
      DEPTH, vector<vector<uint32_t>>(cht_max_degree + 1));
  vector<vector<vector<vector<vector<uint32_t>>>>> thresholds(
      DEPTH, vector<vector<vector<vector<uint32_t>>>>(cht_max_degree + 1));

  std::cout << "[qWaterfall_Fcm] Setup virtual counters and thresholds..."
            << std::endl;
  vector<uint32_t> max_degree(DEPTH, 0);
  for (size_t d = 0; d < DEPTH; d++) {
    std::cout << "Depth " << d << std::endl;
    for (size_t stage = 0; stage < NUM_STAGES; stage++) {
      std::cout << "\tStage " << stage << std::endl;
      for (size_t i = 0; i < this->fcm_sketches.stages_sz[stage]; i++) {
        summary[d][stage][i][0] = this->fcm_sketches.stages[d][stage][i].count;
        // If overflown increase the minimal value for the collisions
        if (this->fcm_sketches.stages[d][stage][i].overflow) {
          summary[d][stage][i][0] =
              this->fcm_sketches.stages[d][stage][i].max_count;
        }

        if (stage == 0) {
          summary[d][stage][i][1] = init_degree[d][i];
          if (summary[d][stage][i][0] > 0 && init_degree[d][i] < 1) {
            summary[d][stage][i][1] = 1;
          }
          overflow_paths[d][stage][i].push_back(
              {(uint32_t)stage, init_degree[d][i], 1, summary[d][stage][i][0]});
        }
        // Start checking childeren from stage 1 and up
        else {
          uint32_t overflown = 0;
          // Loop over all childeren
          for (size_t k = 0; k < this->fcm_sketches.k; k++) {
            uint32_t child_idx = i * this->fcm_sketches.k + k;
            // Add childs count, degree and min_value to current counter
            if (this->fcm_sketches.stages[d][stage - 1][child_idx].overflow) {
              summary[d][stage][i][0] += summary[d][stage - 1][child_idx][0];
              summary[d][stage][i][1] += summary[d][stage - 1][child_idx][1];
              // If any of my predecessors have overflown, add them to my
              // overflown paths
              overflown++;
              for (size_t j = 0;
                   j < overflow_paths[d][stage - 1][child_idx].size(); j++) {
                overflow_paths[d][stage][i].push_back(
                    overflow_paths[d][stage - 1][child_idx][j]);
              }
            }
          }
          // If any of my childeren have overflown, add me to the overflow path
          if (overflown > 0) {
            vector<uint32_t> imm_overflow = {(uint32_t)stage,
                                             summary[d][stage][i][1], overflown,
                                             summary[d][stage][i][0]};
            overflow_paths[d][stage][i].push_back(imm_overflow);
          }
        }

        // If not overflown and non-zero, we are at the end of the path
        if (!this->fcm_sketches.stages[d][stage][i].overflow &&
            summary[d][stage][i][0] > 0) {
          uint32_t count = summary[d][stage][i][0];
          uint32_t degree = summary[d][stage][i][1];
          if (overflow_paths[d][stage][i].empty()) {
            std::cout << "[ERROR] OVerflow path is empty" << std::endl;
            printf("d:%zu, s:%zu, i:%zu, count:%i, degree:%i\n", d, stage, i,
                   count, degree);
          }

          if (count >= 18000000) {
            std::cout << "Found large number at ";
            printf("d:%zu, s:%zu, i:%zu, count:%i, degree:%i\n", d, stage, i,
                   count, degree);
          }

          if (degree >= thresholds[d].size()) {
            thresholds[d].resize(degree + 1);
            virtual_counters[d].resize(degree + 1);
          }
          max_degree[d] = std::max(max_degree[d], degree);
          // Add entry to VC with its degree [1] and count [0], and add it to
          // the thresholds
          virtual_counters[d][degree].push_back(count);
          thresholds[d][degree].push_back(overflow_paths[d][stage][i]);
        }
      }
    }
  }

  std::cout << "[qWaterfall_Fcm] ...done!" << std::endl;
  /*for (size_t d = 0; d < thresholds.size(); d++) {*/
  /*  if (thresholds[d].size() == 0) {*/
  /*    continue;*/
  /*  }*/
  /*  std::cout << "Depth: " << d << std::endl;*/
  /*  for (size_t xi = 0; xi < thresholds[d].size(); xi++) {*/
  /**/
  /*    for (size_t i = 0; i < thresholds[d][xi].size(); i++) {*/
  /*      std::cout << "i " << i << ":";*/
  /*      for (size_t l = 0; l < thresholds[d][xi][i].size(); l++) {*/
  /*        std::cout << " <";*/
  /*        for (auto &col : thresholds[d][xi][i][l]) {*/
  /*          std::cout << col;*/
  /*          if (&col != &thresholds[d][xi][i][l].back()) {*/
  /*            std::cout << ", ";*/
  /*          }*/
  /*        }*/
  /*        std::cout << "> ";*/
  /*      }*/
  /*      std::cout << std::endl;*/
  /*    }*/
  /*  }*/
  /*}*/

  std::cout << std::endl;
  for (size_t d = 0; d < virtual_counters.size(); d++) {
    if (virtual_counters[d].size() == 0) {
      continue;
    }
    std::cout << "Depth " << d << " : " << std::endl;
    for (size_t xi = 0; xi < virtual_counters[d].size(); xi++) {
      std::cout << "Degree " << xi
                << "\tThreshold size: " << thresholds[d][xi].size()
                << "\tVC size: " << virtual_counters[d][xi].size();
      for (auto &val : virtual_counters[d][xi]) {
        /*if (xi > 2) {*/
        /*  std::cout << " " << val << " with " << thresholds[d][xi].size()*/
        /*            << " ";*/
        /*}*/
        max_counter_value = std::max(max_counter_value, val);
      }
      std::cout << "\tMaximum value: " << max_counter_value << std::endl;
    }
    std::cout << std::endl;
  }
  std::cout << "CHT maximum degree is: " << cht_max_degree << std::endl;
  std::cout << "Maximum degree is: " << max_degree[0] << ", " << max_degree[1]
            << std::endl;
  std::cout << "Maximum counter value is: " << max_counter_value << std::endl;

  EM_FSD_QW_FCMS em_fsd(thresholds, max_counter_value, max_degree,
                        virtual_counters);

  std::cout << "Initialized EM_FSD, starting estimation..." << std::endl;
  double wmre = 0.0;
  double d_wmre = 0.0;
  for (size_t i = 0; i < this->em_iters; i++) {
    em_fsd.next_epoch();
    vector<double> ns = em_fsd.ns;

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
    std::cout << "[qWaterfall_Fcm - EM FSD iter " << i << "] intermediary wmre "
              << wmre << " delta: " << wmre - d_wmre << std::endl;
    d_wmre = wmre;
  }

  uint32_t card_waterfall = tuples.size();
  uint32_t card_em = em_fsd.n_new;
  if (card_em < card_waterfall) {
    uint32_t d_card = card_waterfall - card_em;
    std::cout << "[qWaterfall_Fcm - Card Padding] Still missing " << d_card
              << " number of flows as EM " << card_em << " with qWaterfall has "
              << card_waterfall << std::endl;

    vector<double> ns = em_fsd.ns;
    // TODO: Use zipf distribution for calculation diff
    // These values are from FlowLiDAR as flows are split up like this. With
    // remaining flows being >3
    ns[1] += (double)0.60 * d_card;
    ns[2] += (double)0.10 * d_card;
    /*ns[3] += 0.10 * d_card;*/

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
    std::cout << "[qWaterfall_Fcm - Card Padding] wmre " << wmre
              << " delta: " << wmre - d_wmre << std::endl;
    d_wmre = wmre;
  }

  std::cout << "...done!" << std::endl;
  return wmre;
}

template <typename TUPLE, typename HASH>
vector<vector<uint32_t>> qWaterfall_Fcm<TUPLE, HASH>::peel_sketches(
    vector<vector<uint32_t>> counts,
    vector<vector<vector<TUPLE>>> coll_tuples) {

  vector<uint32_t> n_remain = {0, 0};
  for (size_t d = 0; d < DEPTH; d++) {
    for (size_t i = 0; i < coll_tuples[d].size(); i++) {
      if (coll_tuples[d][i].empty()) {
        continue;
      }
      n_remain[d]++;
    }
  }

  std::cout << "Starting flows in coll_tuples " << n_remain[0] << ", "
            << n_remain[1] << std::endl;

  vector<vector<uint32_t>> init_fsd(DEPTH);
  vector<uint32_t> solved_counters;
  uint32_t d_curr = 0;
  uint32_t d_next = 1;
  // First perform for "perfected" matched flows; Peel flows that are singular
  // in both sketches
  do {

    // Clear previous solved counters and find next batch
    solved_counters.clear();
    for (size_t i = 0; i < coll_tuples[d_curr].size(); i++) {
      // Only pass for single inital degree counters
      if (coll_tuples[d_curr][i].size() != 1) {
        continue;
      }

      TUPLE tuple = coll_tuples[d_curr][i][0];
      uint32_t hash_idx = this->fcm_sketches.hashing(tuple, d_next);
      // Only pass if the in the other sketch there is only one tuple
      if (coll_tuples[d_next][hash_idx].size() != 1) {
        continue;
      }
      // Has to be equal
      if (coll_tuples[d_next][hash_idx][0] != tuple) {
        continue;
      }

      uint32_t degree = this->fcm_sketches.lookup_degree(tuple, d_curr);
      if (degree == 1) {
        solved_counters.push_back(i);

        coll_tuples[d_curr][i].clear();
      }
    }
    std::cout << "Solved " << solved_counters.size()
              << " counters in Stage 0 of Depth " << d_curr << std::endl;

    // Remove found single degree small flow counters from other FCM Sketch
    for (auto &i : solved_counters) {
      TUPLE sub_tuple = coll_tuples[d_curr][i][0];
      uint32_t sub_count = counts[d_curr][i];
      uint32_t hash_idx = this->fcm_sketches.hashing(sub_tuple, d_next);

      if (std::find(coll_tuples[d_next][hash_idx].begin(),
                    coll_tuples[d_next][hash_idx].end(),
                    sub_tuple) == coll_tuples[d_next][hash_idx].end()) {
        std::cout << "Could not find tuple in other depth " << d_next
                  << " found at depth " << d_curr
                  << " with tuple: " << sub_tuple << std::endl;
        std::cout << "D_n has following tuples: ";
        for (auto &t : coll_tuples[d_next][hash_idx]) {
          std::cout << t << std::endl;
        }
        std::cout << std::endl;
        exit(1);
      }

      if (sub_count > counts[d_next][hash_idx]) {
        std::cout << "Subcount mismatch at " << hash_idx
                  << " with tuple: " << sub_tuple << " and sub count "
                  << sub_count << ", counts " << counts[d_next][hash_idx]
                  << std::endl;
        uint32_t diff = sub_count - counts[d_next][hash_idx];
        init_fsd[d_curr].push_back(counts[d_next][hash_idx]);
        init_fsd[d_next].push_back(diff);
        counts[d_next][hash_idx] = 0;
      } else {
        counts[d_next][hash_idx] -= sub_count;
        init_fsd[d_curr].push_back(counts[d_curr][i]);
      }

      // Subtract if from the local records and from the sketch
      this->fcm_sketches.subtract(sub_tuple, sub_count);
      coll_tuples[d_next][hash_idx].erase(
          std::find(coll_tuples[d_next][hash_idx].begin(),
                    coll_tuples[d_next][hash_idx].end(), sub_tuple));
    }

    // Switch around the sketches
    uint32_t t = d_curr;
    d_curr = d_next;
    d_next = t;

    std::cout << "Found " << init_fsd.size() << " total flows" << std::endl;
  } while (solved_counters.size() > 0);

  n_remain = {0, 0};
  for (size_t d = 0; d < DEPTH; d++) {
    for (size_t i = 0; i < coll_tuples[d].size(); i++) {
      if (coll_tuples[d][i].empty()) {
        continue;
      }
      n_remain[d]++;
    }
  }

  d_curr = 0;
  d_next = 1;
  // Now match imperfect matches
  do {

    // Clear previous solved counters and find next batch
    solved_counters.clear();
    for (size_t i = 0; i < coll_tuples[d_curr].size(); i++) {
      // Degree 1 for curr depth
      if (coll_tuples[d_curr][i].size() != 1) {
        continue;
      }

      // If other sketchs has less count, then this is not actually a degree 1
      // counter
      TUPLE tuple = coll_tuples[d_curr][i][0];
      uint32_t sub_count = counts[d_curr][i];
      uint32_t hash_idx = this->fcm_sketches.hashing(tuple, d_next);
      if (sub_count > counts[d_next][hash_idx]) {
        continue;
      }

      solved_counters.push_back(i);
      coll_tuples[d_curr][i].clear();
    }
    std::cout << "Solved " << solved_counters.size()
              << " counters in Stage 0 of Depth " << d_curr << std::endl;

    // Remove found single degree small flow counters from other FCM Sketch
    for (auto &i : solved_counters) {
      TUPLE sub_tuple = coll_tuples[d_curr][i][0];
      uint32_t sub_count = counts[d_curr][i];
      uint32_t hash_idx = this->fcm_sketches.hashing(sub_tuple, d_next);

      if (std::find(coll_tuples[d_next][hash_idx].begin(),
                    coll_tuples[d_next][hash_idx].end(),
                    sub_tuple) == coll_tuples[d_next][hash_idx].end()) {
        std::cout << "Could not find tuple in other depth " << d_next
                  << " found at depth " << d_curr
                  << " with tuple: " << sub_tuple << std::endl;
        exit(1);
      }

      if (sub_count > counts[d_next][hash_idx]) {
        std::cout << "Subcount mismatch at " << hash_idx
                  << " with tuple: " << sub_tuple << " and sub count "
                  << sub_count << ", counts " << counts[d_next][hash_idx]
                  << std::endl;
        uint32_t diff = sub_count - counts[d_next][hash_idx];
        init_fsd[d_curr].push_back(counts[d_next][hash_idx]);
        init_fsd[d_next].push_back(diff);
        counts[d_next][hash_idx] = 0;
      } else {
        counts[d_next][hash_idx] -= sub_count;
        init_fsd[d_curr].push_back(counts[d_curr][i]);
      }

      // Subtract if from the local records and from the sketch
      this->fcm_sketches.subtract(sub_tuple, sub_count);
      coll_tuples[d_next][hash_idx].erase(
          std::find(coll_tuples[d_next][hash_idx].begin(),
                    coll_tuples[d_next][hash_idx].end(), sub_tuple));
    }

    // Switch around the sketches
    uint32_t t = d_curr;
    d_curr = d_next;
    d_next = t;

    std::cout << "Found " << init_fsd.size() << " total flows" << std::endl;
  } while (solved_counters.size() > 0);

  std::cout << "Found a total of " << init_fsd[0].size() + init_fsd[1].size()
            << " flows of degree 1, count <= 254" << std::endl;
  std::cout << "Remaining flows in coll_tuples " << n_remain[0] << ", "
            << n_remain[1] << std::endl;

  return init_fsd;
}

template <typename TUPLE, typename HASH>
double
qWaterfall_Fcm<TUPLE, HASH>::calculate_fsd_peeling(set<TUPLE> &tuples,
                                                   vector<uint32_t> &true_fsd) {
  // Setup initial degrees for each input counter (stage 0)
  std::cout << "[qWaterfall_Fcm] Calculate initial degrees from qWaterfall..."
            << std::endl;
  vector<vector<uint32_t>> init_degree(DEPTH, vector<uint32_t>(W1));
  vector<vector<uint32_t>> init_count(DEPTH, vector<uint32_t>(W1));
  vector<vector<vector<TUPLE>>> coll_tuples(DEPTH, vector<vector<TUPLE>>(W1));

  uint32_t cht_max_degree = 0;
  for (size_t d = 0; d < DEPTH; d++) {
    for (auto &tuple : tuples) {
      uint32_t hash_idx = this->fcm_sketches.hashing(tuple, d);
      init_degree[d][hash_idx]++;
      cht_max_degree = std::max(init_degree[d][hash_idx], cht_max_degree);

      init_count[d][hash_idx] = this->fcm_sketches.lookup_sketch(tuple, d);
      /*init_count[d][hash_idx] =
       * this->fcm_sketches.stages[d][0][hash_idx].count;*/
      coll_tuples[d][hash_idx].push_back(tuple);
    }
  }

  std::cout << "[qWaterfall_Fcm] ...done!" << std::endl;
  for (size_t d = 0; d < DEPTH; d++) {
    std::cout << "Depth " << d << std::endl;
    for (size_t i = 0; i < init_degree[d].size(); i++) {
      /*if (init_degree[d][i] > 1) {*/
      /*  std::cout << i << ":" << init_degree[d][i] << " ";*/
      /*}*/
    }
    std::cout << std::endl;
  }

  std::cout << "[qWaterfall_Fcm] Start collecting stage 0 1 degree counters "
               "for peeling"
            << std::endl;

  vector<vector<uint32_t>> init_fsd =
      this->peel_sketches(init_count, coll_tuples);

  // ********************************* //
  // Start setting up summary and VC's //
  // ********************************* //

  uint32_t max_counter_value = 0;
  // Summarize sketch and find collisions
  // depth, stage, idx, (count, degree, min_value)
  vector<vector<vector<vector<uint32_t>>>> summary(DEPTH);
  // depth, stage, idx, layer, (stage, total degree/collisions, local
  // collisions, min value)
  vector<vector<vector<vector<vector<uint32_t>>>>> overflow_paths(DEPTH);

  // Setup sizes for summary and overflow_paths
  for (size_t d = 0; d < DEPTH; d++) {
    summary[d].resize(NUM_STAGES);
    overflow_paths[d].resize(NUM_STAGES);
    for (size_t stage = 0; stage < NUM_STAGES; stage++) {
      summary[d][stage].resize(this->fcm_sketches.stages_sz[stage],
                               vector<uint32_t>(3, 0));
      overflow_paths[d][stage].resize(this->fcm_sketches.stages_sz[stage]);
    }
  }

  // Create virtual counters based on degree and count
  // degree, count value, n
  vector<vector<vector<uint32_t>>> virtual_counters(
      DEPTH, vector<vector<uint32_t>>(cht_max_degree + 1));

  vector<uint32_t> missing_fsd;

  vector<vector<vector<vector<vector<uint32_t>>>>> thresholds(
      DEPTH, vector<vector<vector<vector<uint32_t>>>>(cht_max_degree + 1));

  std::cout << "[qWaterfall_Fcm] Setup virtual counters and thresholds..."
            << std::endl;
  vector<uint32_t> max_degree(DEPTH, 0);
  for (size_t d = 0; d < DEPTH; d++) {
    std::cout << "Depth " << d << std::endl;
    for (size_t stage = 0; stage < NUM_STAGES; stage++) {
      std::cout << "\tStage " << stage << std::endl;
      for (size_t i = 0; i < this->fcm_sketches.stages_sz[stage]; i++) {

        summary[d][stage][i][0] = this->fcm_sketches.stages[d][stage][i].count;
        if (summary[d][stage][i][0] == 0) {
          continue;
        }
        // If overflown increase the minimal value for the collisions
        if (this->fcm_sketches.stages[d][stage][i].overflow) {
          summary[d][stage][i][0] =
              this->fcm_sketches.stages[d][stage][i].max_count;
        }

        if (stage == 0) {
          summary[d][stage][i][1] = init_degree[d][i];
          if (summary[d][stage][i][0] > 0 && init_degree[d][i] < 1) {
            std::cout << "Found count with no initial degree: ";
            printf("d:%zu, s:%zu, i:%zu, count:%d \n", d, stage, i,
                   summary[d][stage][i][0]);
            if (this->fcm_sketches.stages[d][stage][i].overflow) {
              std::cout << "It has overflown so handle it as a regular flow"
                        << std::endl;
              summary[d][stage][i][1] = 1;
            } else {
              missing_fsd.push_back(summary[d][stage][i][0]);
              continue;
            }
          }
          overflow_paths[d][stage][i].push_back(
              {(uint32_t)stage, init_degree[d][i], 1, summary[d][stage][i][0]});
        }
        // Start checking childeren from stage 1 and up
        else {
          uint32_t overflown = 0;
          // Loop over all childeren
          for (size_t k = 0; k < this->fcm_sketches.k; k++) {
            uint32_t child_idx = i * this->fcm_sketches.k + k;
            // Add childs count, degree and min_value to current counter
            if (this->fcm_sketches.stages[d][stage - 1][child_idx].overflow) {
              summary[d][stage][i][0] += summary[d][stage - 1][child_idx][0];
              summary[d][stage][i][1] += summary[d][stage - 1][child_idx][1];
              // If any of my predecessors have overflown, add them to my
              // overflown paths
              overflown++;
              for (size_t j = 0;
                   j < overflow_paths[d][stage - 1][child_idx].size(); j++) {
                overflow_paths[d][stage][i].push_back(
                    overflow_paths[d][stage - 1][child_idx][j]);
              }
            }
          }
          // If any of my childeren have overflown, add me to the overflow
          // path
          vector<uint32_t> imm_overflow = {(uint32_t)stage,
                                           summary[d][stage][i][1], overflown,
                                           summary[d][stage][i][0]};
          overflow_paths[d][stage][i].push_back(imm_overflow);
        }

        // If not overflown and non-zero, we are at the end of the path
        if (!this->fcm_sketches.stages[d][stage][i].overflow &&
            summary[d][stage][i][0] > 0) {
          uint32_t count = summary[d][stage][i][0];
          uint32_t degree = summary[d][stage][i][1];
          if (overflow_paths[d][stage][i].empty()) {
            std::cout << "[ERROR] OVerflow path is empty" << std::endl;
            printf("d:%zu, s:%zu, i:%zu, count:%i, degree:%i\n", d, stage, i,
                   count, degree);
          }
          if (count >= 18000000) {
            std::cout << "Found large number at ";
            printf("d:%zu, s:%zu, i:%zu, count:%i, degree:%i\n", d, stage, i,
                   count, degree);
            exit(1);
          }

          if (degree >= thresholds[d].size()) {
            thresholds[d].resize(degree + 1);
            virtual_counters[d].resize(degree + 1);
          }
          max_degree[d] = std::max(max_degree[d], degree);
          // Add entry to VC with its degree [1] and count [0], and add it to
          // the thresholds
          virtual_counters[d][degree].push_back(count);
          thresholds[d][degree].push_back(overflow_paths[d][stage][i]);
        }
      }
    }
  }

  // Add inital FSD to VC's
  for (size_t d = 0; d < DEPTH; d++) {
    for (auto &flow_size : init_fsd[d]) {
      virtual_counters[d][1].push_back(flow_size);
    }
  }

  std::cout << "[qWaterfall_Fcm] ...done!" << std::endl;
  /*for (size_t d = 0; d < thresholds.size(); d++) {*/
  /*  if (thresholds[d].size() == 0) {*/
  /*    continue;*/
  /*  }*/
  /*  std::cout << "Depth: " << d << std::endl;*/
  /*  for (size_t xi = 0; xi < thresholds[d].size(); xi++) {*/
  /**/
  /*    for (size_t i = 0; i < thresholds[d][xi].size(); i++) {*/
  /*      std::cout << "i " << i << ":";*/
  /*      for (size_t l = 0; l < thresholds[d][xi][i].size(); l++) {*/
  /*        std::cout << " <";*/
  /*        for (auto &col : thresholds[d][xi][i][l]) {*/
  /*          std::cout << col;*/
  /*          if (&col != &thresholds[d][xi][i][l].back()) {*/
  /*            std::cout << ", ";*/
  /*          }*/
  /*        }*/
  /*        std::cout << "> ";*/
  /*      }*/
  /*      std::cout << std::endl;*/
  /*    }*/
  /*  }*/
  /*}*/

  std::cout << std::endl;
  for (size_t d = 0; d < virtual_counters.size(); d++) {
    if (virtual_counters[d].size() == 0) {
      continue;
    }
    std::cout << "Depth " << d << " : " << std::endl;
    for (size_t xi = 0; xi < virtual_counters[d].size(); xi++) {
      std::cout << "Degree " << xi
                << "\tThreshold size: " << thresholds[d][xi].size()
                << "\tVC size: " << virtual_counters[d][xi].size();
      uint32_t im_max_val = 0;
      for (auto &val : virtual_counters[d][xi]) {
        /*if (xi > 2) {*/
        /*  std::cout << " " << val << " with " << thresholds[d][xi].size()*/
        /*            << " ";*/
        /*}*/
        im_max_val = std::max(im_max_val, val);
      }
      std::cout << "\tMaximum value: " << im_max_val << std::endl;
      max_counter_value = std::max(max_counter_value, im_max_val);
    }
    std::cout << std::endl;
  }
  std::cout << "CHT maximum degree is: " << cht_max_degree << std::endl;
  std::cout << "Maximum degree is: " << max_degree[0] << ", " << max_degree[1]
            << std::endl;
  std::cout << "Maximum counter value is: " << max_counter_value << std::endl;

  EM_FSD_QW_FCMS em_fsd(thresholds, max_counter_value, max_degree,
                        virtual_counters);

  std::cout << "Initialized EM_FSD, starting estimation..." << std::endl;
  double wmre = 0.0;
  double d_wmre = 0.0;
  for (size_t i = 0; i < this->em_iters; i++) {
    em_fsd.next_epoch();
    vector<double> ns = em_fsd.ns;

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
    std::cout << "[qWaterfall_Fcm - EM FSD iter " << i << "] intermediary wmre "
              << wmre << " delta: " << wmre - d_wmre << std::endl;
    d_wmre = wmre;
  }

  uint32_t card_waterfall = tuples.size();
  uint32_t card_em = em_fsd.n_new;
  if (card_em < card_waterfall) {
    uint32_t d_card = card_waterfall - card_em;
    std::cout << "[qWaterfall_Fcm - Card Padding] Still missing " << d_card
              << " number of flows as EM " << card_em << " with qWaterfall has "
              << card_waterfall << std::endl;

    vector<double> ns = em_fsd.ns;
    // TODO: Use zipf distribution for calculation diff
    // These values are from FlowLiDAR as flows are split up like this. With
    // remaining flows being >3
    ns[1] += (double)0.60 * d_card;
    ns[2] += (double)0.10 * d_card;
    /*ns[3] += 0.10 * d_card;*/

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
    std::cout << "[qWaterfall_Fcm - Card Padding] wmre " << wmre
              << " delta: " << wmre - d_wmre << std::endl;
    d_wmre = wmre;
  }

  std::cout << "...done!" << std::endl;
  return wmre;
}
#endif // !_Q_WATERFALL_CPP
