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
  uint32_t cuckoo_card = this->cuckoo.tuples.size();
  std::cout << "[CHT] Cardinality : " << cuckoo_card << std::endl;
  this->get_distribution(this->cuckoo.tuples);
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

void WaterfallFCM::get_distribution(set<FIVE_TUPLE> tuples) {
  // Setup initial degrees for each input counter (stage 0)
  vector<uint32_t> init_degree(this->fcm.stages_sz[0]);
  uint32_t max_degree = 0;
  for (auto &tuple : tuples) {
    uint32_t hash_idx = this->fcm.hashing(tuple, 0);
    init_degree[hash_idx]++;
    max_degree = std::max(max_degree, init_degree[hash_idx]);
  }
  for (size_t i = 0; i < init_degree.size(); i++) {
    std::cout << i << ":" << init_degree[i] << " ";
  }
  std::cout << std::endl;

  uint32_t max_counter_value = 0;
  // Summarize sketch and find collisions
  // stage, idx, (count, degree, min_value)
  vector<vector<vector<uint32_t>>> summary(this->n_stages);
  // stage, idx, layer, (collisions, min value)
  vector<vector<vector<vector<uint32_t>>>> overflow_paths(this->n_stages);

  // Setup sizes for summary and overflow_paths
  for (size_t stage = 0; stage < this->n_stages; stage++) {
    summary[stage].resize(this->fcm.stages_sz[stage], vector<uint32_t>(2, 0));
    overflow_paths[stage].resize(this->fcm.stages_sz[stage]);

    for (size_t i = 0; i < this->fcm.stages_sz[stage]; i++) {
      overflow_paths[stage][i].resize(stage + 1, vector<uint32_t>(2, 0));
    }
  }

  // Create virtual counters based on degree and count
  // degree, count value, n
  vector<vector<uint32_t>> virtual_counters(max_degree + 1);
  vector<vector<vector<vector<uint32_t>>>> thresholds(max_degree + 1);

  for (size_t stage = 0; stage < this->n_stages; stage++) {
    for (size_t i = 0; i < this->fcm.stages_sz[stage]; i++) {
      summary[stage][i][0] = this->fcm.stages[stage][i].count;
      if (stage == 0) {
        summary[stage][i][1] = init_degree[i];
      }
      // If overflown increase the minimal value for the collisions
      if (this->fcm.stages[stage][i].overflow) {
        summary[stage][i][0] = this->fcm.stages[stage][i].max_count;
        overflow_paths[stage][i][stage][1] =
            this->fcm.stages[stage][i].max_count;
      }

      // Start checking childeren from stage 1 and up
      if (stage > 0) {
        uint32_t overflown = 0;
        uint32_t imm_overflow = 0;
        bool child_overflown = false;
        // Loop over all childeren
        for (size_t k = 0; k < this->fcm.k; k++) {
          uint32_t child_idx = i * this->fcm.k + k;
          // Add childs count, degree and min_value to current counter
          if (this->fcm.stages[stage - 1][child_idx].overflow) {
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
      if (!this->fcm.stages[stage][i].overflow && summary[stage][i][0] > 0) {
        uint32_t count = summary[stage][i][0];
        uint32_t degree = summary[stage][i][1];
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
  for (size_t d = 0; d < thresholds.size(); d++) {
    if (thresholds[d].size() == 0) {
      continue;
    }
    std::cout << "Degree: " << d << '\t';
    for (size_t i = 0; i < thresholds[d].size(); i++) {
      std::cout << "i " << i << ":";
      for (size_t l = 0; l < thresholds[d][i].size(); l++) {
        std::cout << " " << l;
        for (auto &col : thresholds[d][i][l]) {
          std::cout << " " << col;
        }
      }
    }
    std::cout << std::endl;
  }

  std::cout << std::endl;
  for (size_t st = 0; st < virtual_counters.size(); st++) {
    if (virtual_counters[st].size() == 0) {
      continue;
    }
    std::cout << "Degree " << st << " : ";
    for (auto &val : virtual_counters[st]) {
      std::cout << " " << val;
    }
    std::cout << std::endl;
  }
  std::cout << "Maximum degree is: " << max_degree << std::endl;
  std::cout << "Maximum counter value is: " << max_counter_value << std::endl;
}

void WaterfallFCM::print_sketch() {
  this->cuckoo.print_sketch();
  this->fcm.print_sketch();
}

void WaterfallFCM::set_estimate_fsd(bool onoff) {
  this->fcm.estimate_fsd = onoff;
}
#endif // !_WATERFALL_FCM_CPP
