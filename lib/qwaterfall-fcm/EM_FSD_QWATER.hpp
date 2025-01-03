#ifndef _EMALGORITHM_WATERFALL_FCM_HPP
#define _EMALGORITHM_WATERFALL_FCM_HPP

#include "common.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <numeric>
#include <ostream>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

using std::unordered_map;
using std::vector;

class EM_FSD_QW_FCMS {
private:
  vector<vector<vector<vector<vector<uint32_t>>>>>
      thresholds; // depth, degree, count, < stage, total coll, local
                  // colll, min_value >
  vector<vector<vector<uint32_t>>> counters;
  vector<vector<vector<uint32_t>>> counter_dist; // initial counter values
  vector<double> dist_old, dist_new;             // for ratio \phi
  uint32_t w = W1;                               // width of counters
public:
  vector<double> ns; // for integer n
  uint32_t max_counter_value = 0;
  uint32_t max_degree = 0;
  bool inited = false;
  double n_old,
      n_new; // cardinality
  double n_sum;
  double card_init; // initial cardinality by MLE
  uint32_t iter = 0;
  EM_FSD_QW_FCMS(vector<vector<vector<vector<vector<uint32_t>>>>> &thresh,
                 uint32_t in_max_value, uint32_t max_degree,
                 vector<vector<vector<uint32_t>>> &counters)
      : max_degree(max_degree), max_counter_value(in_max_value),
        counter_dist(DEPTH), thresholds(DEPTH), counters(counters) {

    std::cout << "[EM_QWATERFALL_FCM] Init EMFSD" << std::endl;

    // Setup counters and counters_distribution for estimation, counter_dist is
    // Depth, Degree, Count
    for (size_t d = 0; d < DEPTH; d++) {
      this->counter_dist[d].resize(this->max_degree + 1);
      this->thresholds[d].resize(this->max_degree + 1);

      for (size_t xi = 0; xi < this->counter_dist[d].size(); xi++) {
        this->counter_dist[d][xi].resize(this->max_counter_value + 1);
        this->thresholds[d][xi].resize(this->max_counter_value + 1);
      }
    }
    std::cout << "[EM_WATERFALL_FCM] Finished setting up counter_dist and "
                 "thresholds"
              << std::endl;
    // Inital guess for # of flows, sum total number of counters
    this->n_new = 0.0; // # of flows (Cardinality)
    for (size_t d = 0; d < DEPTH; d++) {
      for (size_t xi = 0; xi < counters[d].size(); xi++) {

        this->n_new += counters[d][xi].size();

        for (size_t i = 0; i < counters[d][xi].size(); i++) {
          this->counter_dist[d][xi][counters[d][xi][i]]++;
          this->thresholds[d][xi][counters[d][xi][i]] = thresh[d][xi][i];
        }
      }
    }
    // Divide by number of sketches
    this->n_new = this->n_new / double(DEPTH);

    std::cout << "[EM_WATERFALL_FCM] Initial cardinality guess : "
              << this->n_new << std::endl;

    // Inital guess for Flow Size Distribution (Phi)
    this->dist_new.resize(this->max_counter_value + 1);
    this->dist_old.resize(this->max_counter_value + 1);
    for (auto &counter : counters) {
      for (auto &degree : counter) {
        for (auto count : degree) {
          this->dist_new[count]++;
        }
      }
    }
    std::cout << "[EM_WATERFALL_FCM] Initial Flow Size Distribution guess"
              << std::endl;
    /*for (auto &x : this->dist_new) {*/
    /*  if (x != 0) {*/
    /*    std::cout << x << " ";*/
    /*  }*/
    /*}*/
    /*std::cout << std::endl;*/

    this->ns.resize(this->max_counter_value + 1);
    for (size_t d = 0; d < DEPTH; d++) {
      for (size_t xi = 0; xi < this->counter_dist[d].size(); xi++) {
        for (size_t i = 0; i < this->counter_dist[d][xi].size(); i++) {
          if (this->counter_dist[d][xi].size() == 0) {
            continue;
          }
          this->dist_new[i] += this->counter_dist[d][xi][i];
          this->ns[i] += this->counter_dist[d][xi][i];
        }
      }
    }
    std::cout << "[EM_WATERFALL_FCM] Summed Flow Size Distribution"
              << std::endl;
    /*for (auto &x : this->dist_new) {*/
    /*  if (x != 0) {*/
    /*    std::cout << x << " ";*/
    /*  }*/
    /*}*/
    /*std::cout << std::endl;*/

    std::cout << "[EM_WATERFALL_FCM] Normalize guesses" << std::endl;
    // Normalize over inital cardinality
    for (size_t i = 0; i < this->dist_new.size(); i++) {
      this->dist_new[i] /= (static_cast<double>(DEPTH) * this->n_new);
      this->ns[i] /= double(DEPTH);
    }
    /*for (auto &x : this->dist_new) {*/
    /*  if (x != 0) {*/
    /*    std::cout << x << " ";*/
    /*  }*/
    /*}*/
    /*std::cout << std::endl;*/

    printf("[EM_WATERFALL_FCM] Initial Cardinality : %9.1f\n", this->n_new);
    printf("[EM_WATERFALL_FCM] Max Counter value : %d\n",
           this->max_counter_value);
    printf("[EM_WATERFALL_FCM] Max degree : %d\n", this->max_degree);
  }

private:
  struct BetaGenerator {
    int sum;
    int now_flow_num;
    int flow_num_limit;
    uint32_t in_degree;
    vector<int> now_result;
    vector<vector<uint32_t>> thresh;

    explicit BetaGenerator(uint32_t _sum, uint32_t _in_degree,
                           vector<vector<uint32_t>> _thresh)
        : sum(_sum), flow_num_limit(_in_degree), in_degree(_in_degree),
          thresh(_thresh) {
      std::reverse(thresh.begin(), thresh.end());
      now_flow_num = flow_num_limit;
      now_result.resize(_in_degree);
      for (size_t i = 0; i < now_result.size() - 1; i++) {
        now_result[i] = 1;
      }

      if (sum > 600) {
        flow_num_limit = std::min(2, flow_num_limit);
      } else if (sum > 250)
        flow_num_limit = std::min(3, flow_num_limit);
      else if (sum > 100)
        flow_num_limit = std::min(4, flow_num_limit);
      else if (sum > 50)
        flow_num_limit = std::min(5, flow_num_limit);
      else
        flow_num_limit = std::min(6, flow_num_limit);
    }

    bool get_new_comb() {
      for (int j = now_flow_num - 2; j >= 0; --j) {
        int t = ++now_result[j];
        for (int k = j + 1; k < now_flow_num - 1; ++k) {
          now_result[k] = t;
        }
        int partial_sum = 0;
        for (int k = 0; k < now_flow_num - 1; ++k) {
          partial_sum += now_result[k];
        }
        int remain = sum - partial_sum;
        if (remain >= now_result[now_flow_num - 2]) {
          now_result[now_flow_num - 1] = remain;
          return true;
        }
      }

      return false;
    }

    bool get_next() {
      while (now_flow_num <= flow_num_limit) {
        if (get_new_comb()) {
          if (check_condition()) {
            // std::ostringstream oss;
            // oss << "Current combi : ";
            // for (auto &x : now_result) {
            //   oss << x << " ";
            // }
            // std::cout << oss.str().c_str() << std::endl;
            return true;
          }
        } else {
          // Extend current combination, e.g. 1 0 to 1 1 0
          now_flow_num++;
          now_result.resize(now_flow_num);
          for (int i = 0; i < now_flow_num - 2; ++i) {
            now_result[i] = 1;
          }
          now_result[now_flow_num - 2] = 0;
        }
      }

      return false;
    }

    void print_now_result() {
      std::cout << "Now result[" << now_result.size() << "] ";
      for (auto &x : now_result) {
        std::cout << x << " ";
      }
      std::cout << std::endl;
    }

    void print_thresholds() {
      std::cout << "Threshold[" << thresh.size() << "] ";
      for (auto &t : thresh) {
        std::cout << " <";
        for (auto &x : t) {
          std::cout << x;
          if (&x != &t.back()) {
            std::cout << ", ";
          }
        }
        std::cout << "> ";
      }
      std::cout << std::endl;
    }

    void print_thresh(auto t) {
      std::cout << "Thresh ";
      std::cout << " <";
      for (auto &x : t) {
        std::cout << x;
        if (&x != &t.back()) {
          std::cout << ", ";
        }
      }
      std::cout << "> ";
      std::cout << std::endl;
    }

    bool check_condition() {
      bool allOne = true;

      for (auto &t : thresh) {
        uint32_t tot_colls = t[1];
        uint32_t colls = t[2];

        if (colls <= 1 && tot_colls <= 1) {
          continue;
        }

        allOne = false;
        if (&t == &thresh.front()) {
          continue;
        }

        /*print_thresholds();*/
        /*print_now_result();*/

        uint32_t min_val = t[3];
        uint32_t group_sz = std::floor(now_flow_num / tot_colls);
        uint32_t last_group_sz = std::ceil(now_flow_num / tot_colls);
        uint32_t passes = 0;

        uint32_t val_last_group = std::accumulate(
            now_result.end() - last_group_sz, now_result.end(), 0);
        /*std::cout << "group sz " << group_sz << " last_group_sz "*/
        /*          << last_group_sz << " last group val " << val_last_group*/
        /*          << " min val " << min_val << std::endl;*/
        if (val_last_group >= min_val) {
          /*std::cout << "Passes min val" << std::endl;*/
          passes++;
          for (size_t i = 0; i < tot_colls - 1; i++) {
            uint32_t accum =
                std::accumulate(now_result.begin() + i * group_sz,
                                now_result.begin() + (i + 1) * group_sz, 0);
            if (accum >= min_val) {
              passes++;
            }
          }
        } else {
          /*std::cout << "Shifting group" << std::endl;*/
          // Shift group to include first entry
          val_last_group = std::accumulate(now_result.end() - last_group_sz + 1,
                                           now_result.end(), 0) +
                           now_result[0];
          /*std::cout << "group sz " << group_sz << " last_group_sz "*/
          /*          << last_group_sz << " last group val " << val_last_group*/
          /*          << " min val " << min_val << std::endl;*/
          if (val_last_group < min_val) {
            /*std::cout << "Invalid permutation of sum " << sum << std::endl;*/
            /*print_thresh(t);*/
            /*print_now_result();*/
            return false;
          }
          passes++;
          for (size_t i = 0; i < tot_colls - 1; i++) {
            uint32_t accum = 0;
            if (1 + (i + 1) * group_sz >= now_result.size()) {
              accum =
                  std::accumulate(now_result.begin() + 1 + i * group_sz,
                                  now_result.begin() + (i + 1) * group_sz, 0);
            } else {
              accum = std::accumulate(
                  now_result.begin() + 1 + i * group_sz,
                  now_result.begin() + 1 + (i + 1) * group_sz, 0);
            }
            if (accum >= min_val) {
              passes++;
            }
          }
        }
        // Combination has not large enough values to meet all conditions
        // E.g. it needs have 2 values large than the L2 threshold +
        // predecessor (min_value)
        if (passes < colls) {
          /*std::cout << "Invalid permutation of sum " << sum << std::endl;*/
          /*print_thresh(t);*/
          /*print_now_result();*/
          return false;
        }
      }
      /*if (!allOne) {*/
      /*  std::cout << "Not all 1, in_degree " << in_degree << std::endl;*/
      /*  print_now_result();*/
      /*  print_thresholds();*/
      /*  std::cout << "Valid permutation" << std::endl;*/
      /*}*/
      return true;
    }
  };

  static constexpr int factorial(int n) {
    if (n == 0 || n == 1)
      return 1;
    return factorial(n - 1) * n;
  }

  double get_p_from_beta(BetaGenerator &bt, double lambda,
                         vector<double> now_dist, double now_n,
                         uint32_t degree) {
    std::unordered_map<uint32_t, uint32_t> mp;
    for (int i = 0; i < bt.now_flow_num; ++i) {
      mp[bt.now_result[i]]++;
    }

    double ret = std::exp(-lambda);
    for (auto &kv : mp) {
      uint32_t fi = kv.second;
      uint32_t si = kv.first;
      double lambda_i = now_n * (now_dist[si] * degree) / w;
      ret *= (std::pow(lambda_i, fi)) / factorial(fi);
    }

    return ret;
  }

  void calculate_degree(vector<double> &nt, int d, int xi) {
    nt.resize(this->max_counter_value + 1);
    std::fill(this->ns.begin(), this->ns.end(), 0.0);

    printf("[EM_WATERFALL_FCM] ******** Running for degree %2d with a size of "
           "%12zu\t\t"
           "**********\n",
           xi, this->counter_dist[d][xi].size());

    for (uint32_t i = 0; i < this->counters[d][xi].size(); i++) {
      if (this->counters[d][xi][i] == 0) {
        continue;
      }

      BetaGenerator alpha(this->counters[d][xi][i], xi,
                          this->thresholds[d][xi][i]),
          beta(this->counters[d][xi][i], xi, this->thresholds[d][xi][i]);
      double sum_p = 0.0;
      uint32_t it = 0;

      double lambda = this->n_old * xi / double(W1);
      // Sum over first combinations
      while (alpha.get_next()) {
        double p =
            get_p_from_beta(alpha, lambda, this->dist_old, this->n_old, xi);
        sum_p += p;
        it++;
      }

      // If there where valid combinations, but value of combinations where not
      // found in measured data. We
      if (sum_p == 0.0) {
        if (it > 0) {
          uint32_t temp_val = this->counters[d][xi][i];
          vector<vector<uint32_t>> temp_thresh = this->thresholds[d][xi][i];
          for (auto &t : temp_thresh) {
            if (temp_val < t[3] * (t[2] - 1)) {
              continue;
            }
            temp_val -= t[3] * (t[2] - 1);
          }
          if (temp_val >= 0 and temp_val <= this->max_counter_value) {
            nt[temp_val] += 1;
          }
        }
      } else {
        while (beta.get_next()) {
          double p =
              get_p_from_beta(beta, lambda, this->dist_old, this->n_old, xi);
          for (size_t j = 0; j < beta.now_flow_num; ++j) {
            nt[beta.now_result[j]] += p / sum_p;
          }
        }
      }
    }

    double accum = std::accumulate(nt.begin(), nt.end(), 0.0);
    if (accum != accum) {
      std::cout << "Accum is Nan" << std::endl;
      for (auto &x : nt) {
        std::cout << x << " ";
      }
      std::cout << std::endl;
    }
    if (this->counter_dist[d][xi].size() != 0)
      printf("[EM_WATERFALL_FCM] ******** depth %d degree %2d is "
             "finished...(accum:%10.1f #val:%8d)\t**********\n",
             d, xi, accum, (int)this->counter_dist[d][xi].size());
  }

public:
  void next_epoch() {
    std::cout << "[EM_WATERFALL_FCM] Start next epoch" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    this->n_old = this->n_new;
    this->dist_old = this->dist_new;

    std::cout << "[EM_WATERFALL_FCM] Copy first degree distribution"
              << std::endl;

    // Always copy first degree as this is can be considered a perfect
    // estimation. qWaterfall is not perfect, but assumed to be
    vector<vector<vector<double>>> nt(DEPTH);
    for (size_t d = 0; d < DEPTH; d++) {
      nt[d].resize(this->max_degree + 1);

      nt[d][1].resize(this->counter_dist[d][1].size());
      for (size_t i = 0; i < this->counter_dist[d][1].size(); i++) {
        nt[d][1][i] += this->counter_dist[d][1][i];
      }
    }

    std::fill(this->ns.begin(), this->ns.end(), 0);
    std::cout << "[EM_WATERFALL_FCM] Init first degree" << std::endl;
    // Simple Multi thread
    vector<vector<std::thread>> threads(DEPTH);
    for (size_t d = 0; d < threads.size(); d++) {
      threads[d].resize(this->max_degree + 1);
    }

    uint32_t total_degree = this->max_degree * 2 + 1;
    std::cout << "[EM_WATERFALL_FCM] Created " << total_degree << " threads"
              << std::endl;

    for (size_t d = 0; d < DEPTH; d++) {
      for (size_t t = 2; t < threads[d].size(); t++) {
        std::cout << "[EM_WATERFALL_FCM] Start thread " << t << " at depth "
                  << d << std::endl;
        threads[d][t] = std::thread(&EM_FSD_QW_FCMS::calculate_degree, *this,
                                    std::ref(nt[d][t]), d, t);
      }
    }

    std::cout
        << "[EM_WATERFALL_FCM] Started all threads, wait for them to finish..."
        << std::endl;

    for (size_t d = 0; d < DEPTH; d++) {
      for (size_t t = 2; t < threads[d].size(); t++) {
        threads[d][t].join();
      }
    }

    // Single threaded
    /*for (size_t d = 0; d < DEPTH; d++) {*/
    /*  for (size_t xi = 2; xi <= this->max_degree; xi++) {*/
    /*    this->calculate_degree(nt[d][xi], d, xi);*/
    /*  }*/
    /*}*/

    std::cout << "[EM_WATERFALL_FCM] Finished calculating nt per degree"
              << std::endl;

    for (size_t d = 0; d < DEPTH; d++) {
      for (size_t xi = 0; xi < nt[d].size(); xi++) {
        for (uint32_t i = 0; i < nt[d][xi].size(); i++) {
          this->ns[i] += nt[d][xi][i];
        }
      }
    }

    this->n_new = 0.0;
    for (size_t i = 0; i < this->ns.size(); i++) {
      if (this->ns[i] != 0) {
        this->ns[i] /= static_cast<double>(DEPTH);
        this->n_new += this->ns[i];
      }
    }
    for (uint32_t i = 0; i < this->ns.size(); i++) {
      this->dist_new[i] = this->ns[i] / this->n_new;
    }
    auto stop = std::chrono::high_resolution_clock::now();
    auto time =
        std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

    printf("[EM_WATERFALL_FCM - iter %2d] Compute time : %li\n", iter,
           time.count());
    printf("[EM_WATERFALL_FCM - iter %2d] Intermediate cardianlity : "
           "%9.1f\n\n",
           iter, this->n_new);
    iter++;
  }
};

#endif
