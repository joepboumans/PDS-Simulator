#ifndef _WATERFALL_FCM_CPP
#define _WATERFALL_FCM_CPP

#include "waterfall-fcm.hpp"
#include "EM_FCM_org.h"
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
    double wmre = 0.0;

    for (const auto &[tuple, count] : this->fcm_sketches.true_data) {
      true_fsd[count]++;
    }

    this->wmre = 0.0;
    double wmre_nom = 0.0;
    double wmre_denom = 0.0;
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

double WaterfallFCM::get_distribution(set<TUPLE> &tuples,
                                      vector<uint32_t> &true_fsd) {

  /*vector<uint32_t> init_fsd = this->peel_sketches(tuples);*/
  /**/
  /*uint32_t max_len = std::max(true_fsd.size(), init_fsd.size());*/
  /*true_fsd.resize(max_len);*/
  /*init_fsd.resize(max_len);*/
  /**/
  /*double wmre = 0.0;*/
  /*double wmre_nom = 0.0;*/
  /*double wmre_denom = 0.0;*/
  /*for (size_t i = 0; i < max_len; i++) {*/
  /*  wmre_nom += std::abs(double(true_fsd[i]) - init_fsd[i]);*/
  /*  wmre_denom += double((double(true_fsd[i]) + init_fsd[i]) / 2);*/
  /*}*/
  /*wmre = wmre_nom / wmre_denom;*/
  /*std::cout << "[WaterfallFCM - Peeling] intermediary wmre " << wmre*/
  /*          << std::endl;*/

  /* Making summary note for conversion algorithm (4-tuple summary)
                 Each dimension is for (tree, layer, width, tuple)
                 */
  vector<int> get_width{W1, W2, W3};
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

  std::cout << "[WaterfallFCM] Start setting up summary and thresholds"
            << std::endl;
  for (int d = 0; d < DEPTH; ++d) {
    summary[d].resize(NUM_STAGES);
    track_thres[d].resize(NUM_STAGES);
    for (uint32_t i = 0; i < NUM_STAGES; ++i) {
      summary[d][i].resize(
          get_width[i],
          vector<uint32_t>(4, 0)); // { Total Degree, Previous Degrees, Local
                                   // Degree , count}
      track_thres[d][i].resize(get_width[i]); // initialize
      for (int w = 0; w < get_width[i]; ++w) {
        if (i == 0) { // stage 1
          summary[d][i][w][2] = 1;
          /*summary[d][i][w][2] =*/
          /*    std::max(init_degree[d][w], (uint32_t)1); // default*/
          summary[d][i][w][3] =
              std::min(this->fcm_sketches.stages[d][0][w].count,
                       (uint32_t)OVERFLOW_LEVEL1); // depth 0

          if (!this->fcm_sketches.stages[d][0][w].overflow) { // not overflow
            summary[d][i][w][0] = summary[d][i][w][2];

          } else { // if counter is overflow
            track_thres[d][i][w].push_back(
                vector<uint32_t>{0, summary[d][i][w][2], summary[d][i][w][3]});
          }
        } else if (i == 1) { // stage 2
          summary[d][i][w][3] =
              std::min(this->fcm_sketches.stages[d][1][w].count,
                       (uint32_t)OVERFLOW_LEVEL2);

          summary[d][i][w][2] = 0;
          for (int t = 0; t < K; ++t) {
            // if child is overflow, then accumulate both "value" and
            // "threshold"
            if (!this->fcm_sketches.stages[d][i - 1][K * w + t]
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

          if (!this->fcm_sketches.stages[d][1][w]
                   .overflow) // non-overflow, end of path
          {
            summary[d][i][w][0] = summary[d][i][w][2];
          } else {
            // if overflow, then push new threshold <layer, #path, value>
            track_thres[d][i][w].push_back(
                vector<uint32_t>{i, summary[d][i][w][2], summary[d][i][w][3]});
          }
        } else if (i == 2) { // stage 3
          summary[d][i][w][3] = this->fcm_sketches.stages[d][2][w].count;
          summary[d][i][w][2] = 0;

          for (int t = 0; t < K; ++t) {
            // if child is overflow, then accumulate both "value" and
            // "threshold"
            if (!this->fcm_sketches.stages[d][i - 1][K * w + t]
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

          if (!this->fcm_sketches.stages[d][i][w]
                   .overflow) // non-overflow, end of path
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

  std::cout << "[WaterfallFCM] Transform summary and thresh into newsk and "
               "newsk_thresh"
            << std::endl;

  for (int d = 0; d < DEPTH; ++d) {
    // size = all possible degree
    newsk[d].resize(std::pow(K, NUM_STAGES - 1) * 3 +
                    1); // maximum degree : k^(L-1) + 1 (except 0 index)
    newsk_thres[d].resize(std::pow(K, NUM_STAGES - 1) * 3 +
                          1); // maximum degree : k^(L-1) + 1 (except 0 index)
    for (int i = 0; i < NUM_STAGES; ++i) {
      for (int w = 0; w < get_width[i]; ++w) {
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

  std::cout << "[WaterfallFCM] Finished setting up newsk and "
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

  EM_FCM_org<DEPTH, W1, OVERFLOW_LEVEL1, OVERFLOW_LEVEL2> em_fsd_algo; // new

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
    std::cout << "[WaterfallFCM - EM FSD iter " << i << "] intermediary wmre "
              << wmre << " delta: " << wmre - d_wmre << std::endl;
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

  uint32_t card_waterfall = tuples.size();
  uint32_t card_em = em_fsd_algo.n_new;
  if (card_em < card_waterfall) {
    auto start = std::chrono::high_resolution_clock::now();
    em_fsd_algo.next_epoch();
    uint32_t d_card = card_waterfall - card_em;
    std::cout << "[WaterfallFCM - Card Padding] Still missing " << d_card
              << " number of flows as EM " << card_em << " with Waterfall has "
              << card_waterfall << std::endl;

    vector<double> ns = em_fsd_algo.ns;
    // These values are from FlowLiDAR as flows are split up like this. With
    // remaining flows being >3
    for (size_t i = 0; i < 5; i++) {
      double val = (double)(ns[i] / card_em) * d_card;
      ns[i] += val;
      card_em += val;
      d_card = card_waterfall - card_em;
    }
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
    std::cout << "[WaterfallFCM - Card Padding] wmre " << wmre
              << " delta: " << wmre - d_wmre << std::endl;
    d_wmre = wmre;
    char csv[300];
    sprintf(csv, "%u,%.3ld,%.3ld,%.3f,%.3f", this->em_iters, time.count(),
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

#endif // !_WATERFALL_FCM_CPP
