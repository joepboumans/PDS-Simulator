#ifndef _Q_WATERFALL_CPP
#define _Q_WATERFALL_CPP

#include "qwaterfall-fcm.hpp"
#include "EM_FSD_QWATER.hpp"
#include "EM_FSD_t.hpp"
#include "common.h"
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <sys/types.h>

template class qWaterfall_Fcm<FIVE_TUPLE, fiveTupleHash>;
template class qWaterfall_Fcm<FLOW_TUPLE, flowTupleHash>;

template <typename TUPLE, typename HASH>
uint32_t qWaterfall_Fcm<TUPLE, HASH>::insert(TUPLE tuple) {
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

    this->wmre = 0.0;
    double wmre_nom = 0.0;
    double wmre_denom = 0.0;
    auto start = std::chrono::high_resolution_clock::now();
    vector<double> em_fsd = this->get_distribution(this->qwaterfall.tuples);

    uint32_t max_len = std::max(true_fsd.size(), em_fsd.size());
    true_fsd.resize(max_len);
    em_fsd.resize(max_len);
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

    for (size_t i = 0; i < max_len; i++) {
      wmre_nom += std::abs(double(true_fsd[i]) - em_fsd[i]);
      wmre_denom += double((double(true_fsd[i]) + em_fsd[i]) / 2);
    }
    this->wmre = wmre_nom / wmre_denom;
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
vector<double>
qWaterfall_Fcm<TUPLE, HASH>::get_distribution(set<TUPLE> &tuples) {
  // Setup initial degrees for each input counter (stage 0)
  std::cout << "[qWaterfall_Fcm] Calculate initial degrees from qWaterfall..."
            << std::endl;
  vector<vector<uint32_t>> init_degree(DEPTH, vector<uint32_t>(W1));
  uint32_t cht_max_degree = 0;
  for (size_t d = 0; d < DEPTH; d++) {
    for (auto &tuple : tuples) {
      uint32_t hash_idx = this->fcm_sketches.hashing(tuple, 0, d);
      init_degree[d][hash_idx]++;
      cht_max_degree = std::max(init_degree[d][hash_idx], cht_max_degree);
    }
  }
  std::cout << "[qWaterfall_Fcm] ...done!" << std::endl;
  // for (size_t i = 0; i < init_degree.size(); i++) {
  //   std::cout << i << ":" << init_degree[i] << " ";
  // }
  // std::cout << std::endl;

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
  uint32_t max_degree = 0;
  for (size_t d = 0; d < DEPTH; d++) {
    std::cout << "Depth " << d << std::endl;
    for (size_t stage = 0; stage < NUM_STAGES; stage++) {
      std::cout << "Stage " << stage << std::endl;
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

          if (degree >= thresholds[d].size()) {
            thresholds[d].resize(degree + 1);
            virtual_counters[d].resize(degree + 1);
          }
          // Add entry to VC with its degree [1] and count [0]
          virtual_counters[d][degree].push_back(count);
          max_degree = std::max(max_degree, degree);
          if (stage > 1) {
            std::cout << "Stage 3 counts " << count << std::endl;
            std::cout << "End of counter of with degree " << degree
                      << " of index " << i << " at stage " << stage
                      << std::endl;
          }

          // Remove 1 collsions
          // for (size_t j = overflow_paths[d][stage][i].size() - 1; j > 0; --j)
          // {
          //   if (overflow_paths[d][stage][i][j][2] <= 1) {
          //     overflow_paths[d][stage][i].erase(overflow_paths[d][stage][i].begin()
          //     +
          //                                    j);
          //   }
          // }

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
    std::cout << "Depth " << d << " : ";
    for (size_t st = 0; st < virtual_counters[d].size(); st++) {
      for (auto &val : virtual_counters[d][st]) {
        /*std::cout << " " << val;*/
        max_counter_value = std::max(max_counter_value, val);
      }
    }
    std::cout << std::endl;
  }
  std::cout << "CHT maximum degree is: " << cht_max_degree << std::endl;
  std::cout << "Maximum degree is: " << max_degree << std::endl;
  std::cout << "Maximum counter value is: " << max_counter_value << std::endl;

  EM_FSD_QW_FCMS em_fsd(thresholds, max_counter_value, max_degree,
                        virtual_counters);

  std::cout << "Initialized EM_FSD, starting estimation..." << std::endl;
  for (size_t i = 0; i < this->em_iters; i++) {
    em_fsd.next_epoch();
    /*em_fsd->test();*/
  }
  vector<double> output = {1, 2}; // em_fsd.ns;
  std::cout << "...done!" << std::endl;
  return output;
}

#endif // !_Q_WATERFALL_CPP
