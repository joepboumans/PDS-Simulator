#ifndef _EM_GENERIC_FCM_HPP
#define _EM_GENERIC_FCM_HPP

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

class EM_GFCM {
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
  vector<uint32_t> max_degree;
  bool inited = false;
  double n_old,
      n_new; // cardinality
  double n_sum;
  double card_init; // initial cardinality by MLE
  uint32_t iter = 0;
  EM_GFCM(vector<vector<vector<vector<vector<uint32_t>>>>> &thresh,
          uint32_t in_max_value, vector<uint32_t> max_degree,
          vector<vector<vector<uint32_t>>> &counters)
      : thresholds(thresh), counters(counters), counter_dist(DEPTH),
        max_counter_value(in_max_value), max_degree(max_degree) {

    std::cout << "[EM_GFCM] Init EMFSD" << std::endl;

    // Setup counters and counters_distribution for estimation, counter_dist is
    // Depth, Degree, Count
    for (size_t d = 0; d < DEPTH; d++) {
      this->thresholds[d][1].resize(this->max_counter_value + 1);
      this->counter_dist[d].resize(this->max_degree[d] + 1);

      for (size_t xi = 0; xi < this->counter_dist[d].size(); xi++) {
        this->counter_dist[d][xi].resize(this->max_counter_value + 1);
      }
    }
    std::cout << "[EM_GFCM] Finished setting up counter_dist and "
                 "thresholds"
              << std::endl;
    // Inital guess for # of flows, sum total number of counters
    this->n_new = 0.0; // # of flows (Cardinality)
    for (size_t d = 0; d < DEPTH; d++) {
      for (size_t xi = 0; xi < counters[d].size(); xi++) {
        this->n_new += counters[d][xi].size();

        for (size_t i = 0; i < counters[d][xi].size(); i++) {
          this->counter_dist[d][xi][counters[d][xi][i]]++;
        }
      }
    }
    // Divide by number of sketches
    this->n_new = this->n_new / double(DEPTH);

    std::cout << "[EM_GFCM] Initial cardinality guess : " << this->n_new
              << std::endl;

    // Inital guess for Flow Size Distribution (Phi)
    this->dist_new.resize(this->max_counter_value + 1);
    this->dist_old.resize(this->max_counter_value + 1);

    this->ns.resize(this->max_counter_value + 1);
    for (size_t d = 0; d < DEPTH; d++) {
      for (size_t xi = 0; xi < this->counter_dist[d].size(); xi++) {
        for (size_t i = 0; i < this->counter_dist[d][xi].size(); i++) {
          this->dist_new[i] += this->counter_dist[d][xi][i];
          this->ns[i] += this->counter_dist[d][xi][i];
        }
      }
    }

    std::cout << "[EM_GFCM] Initial Flow Size Distribution guess" << std::endl;
    /*for (size_t i = 0; i < this->dist_new.size(); i++) {*/
    /*  if (this->dist_new[i] != 0) {*/
    /*    std::cout << i << ":" << this->dist_new[i] << " ";*/
    /*  }*/
    /*}*/
    /*std::cout << std::endl;*/

    std::cout << "[EM_GFCM] Normalize guesses" << std::endl;
    // Normalize over inital cardinality
    for (size_t i = 0; i < this->dist_new.size(); i++) {
      this->dist_new[i] /= (static_cast<double>(DEPTH) * this->n_new);
      this->ns[i] /= double(DEPTH);
    }

    /*for (size_t i = 0; i < this->dist_new.size(); i++) {*/
    /*  if (this->dist_new[i] != 0) {*/
    /*    std::cout << i << ":" << this->dist_new[i] << " ";*/
    /*  }*/
    /*}*/
    /*std::cout << std::endl;*/

    printf("[EM_GFCM] Initial Cardinality : %9.1f\n", this->n_new);
    printf("[EM_GFCM] Max Counter value : %d\n", this->max_counter_value);
    printf("[EM_GFCM] Max degree : %d, %d\n", this->max_degree[0],
           this->max_degree[1]);
  }

private:
  struct BetaGenerator {
    int sum;
    int now_flow_num;
    int flow_num_limit;
    int start_pos;
    uint32_t in_degree;
    uint32_t total_combi = 0;
    vector<int> now_result;
    vector<vector<uint32_t>> thresh;
    uint32_t min_val = -1;
    uint32_t min_count = 0;
    uint32_t max_val = 0;
    uint32_t max_count = 0;

    explicit BetaGenerator(uint32_t _sum, uint32_t _in_degree,
                           vector<vector<uint32_t>> _thresh)
        : sum(_sum), flow_num_limit(_in_degree), in_degree(_in_degree),
          thresh(_thresh) {

      now_flow_num = 0;

      // Setup limit for degree 1
      if (in_degree <= 1) {
        flow_num_limit = 6;
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

      } else { // Setup Multi degree limits
        if (sum > 1100)
          flow_num_limit = in_degree;
        else if (sum > 550)
          flow_num_limit = std::max(in_degree, (uint32_t)3);
        else
          flow_num_limit = std::max(in_degree, (uint32_t)4);

        // Note that the counters of degree D (> 1) includes at least D flows.
        // But, the combinatorial complexity also increases to O(N^D), which
        // takes a lot of time. To truncate the compexity, we introduce a
        // simplified version that prohibits the number of flows less than D. As
        // explained in the paper, its effect is quite limited as the number of
        // such counters are only a few.

        // "simplified" keeps the original in_degree
        // 15k, 15k, 7k
        if (in_degree >= 4 and sum < 10000) {
          flow_num_limit = 3;
          in_degree = 3;
        } else if (in_degree >= 4 and sum >= 10000) {
          flow_num_limit = 2;
          in_degree = 2;
        } else if (in_degree == 3 and sum > 5000) {
          flow_num_limit = 2;
          in_degree = 2;
        } else if (in_degree == 2 and sum > ADD_LEVEL2) {
          thresh[1][1] = 1;
        }
        // Setup limits for thresholds
        for (auto &t : thresh) {
          if (t[3] == max_val) {
            max_count++;
          } else if (t[3] > max_val) {
            max_count = 1;
            max_val = t[3];
          }
          if (t[3] == min_val) {
            min_count++;
          } else if (t[3] < min_val) {
            min_count = 1;
            min_val = t[3];
          }
        }
      }

      start_pos = 0;
      /*printf("Setup gen with sum:%d, in_degree:%d, flow_num_limit:%d\n",
       * sum,*/
      /*       in_degree, flow_num_limit);*/
      /*print_thresholds();*/
    }

    bool get_new_comb() {
      for (int j = now_flow_num - 2; j >= start_pos; --j) {
        int t = ++now_result[j];
        for (int k = j + 1; k < now_flow_num - 1; ++k) {
          now_result[k] = t;
        }
        int partial_sum = 0;
        for (int k = start_pos; k < now_flow_num - 1; ++k) {
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
        switch (now_flow_num) {
        case 0:
          now_flow_num = 1;
          now_result.resize(now_flow_num);
          now_result[0] = sum;
          if (in_degree == 1) {
            return true;
          }
        case 1:
          now_flow_num = 2;
          now_result[0] = 0;
        default:
          now_result.resize(now_flow_num);
          if (get_new_comb() and now_flow_num >= in_degree) {
            if (in_degree == 1) {
              total_combi++;
              return true;
            }
            if (check_condition()) {
              total_combi++;
              return true;
            }
          } else { // no more combination -> go to next flow number
            now_flow_num++;
            for (int i = 0; i < now_flow_num - 2; ++i) {
              now_result[i] = 1;
            }
            now_result[now_flow_num - 2] = 0;
          }
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

      for (auto &t : thresh) {
        uint32_t colls = t[0];

        if (colls <= 1) {
          continue;
        }

        if (&t == &thresh.front()) {
          continue;
        }
        /*print_thresholds();*/
        /*print_now_result();*/

        uint32_t min_val = t[1];
        uint32_t last_group_sz = 1;
        uint32_t passes = 0;

        uint32_t n_spread = 1;
        if (now_flow_num > in_degree) {
          n_spread = now_flow_num - in_degree;
        }

        uint32_t last_val = now_result.back();
        /*std::cout << "n spread " << n_spread << " last_group_sz "*/
        /*          << last_group_sz << " last group val " << last_val*/
        /*          << " min val " << min_val << std::endl;*/

        if (now_flow_num == in_degree and last_val <= min_val) {
          return false;
        }

        if (last_val > min_val) {
          passes++;
          // First group takes whole spread
          uint32_t first_val = std::accumulate(
              now_result.begin(), now_result.begin() + n_spread, 0);
          if (first_val <= min_val) {
            return false;
          }
          passes++;
          // Next groups are singles
          for (size_t i = 0; i < in_degree - 2; i++) {
            if (now_result[i + n_spread] > min_val) {
              passes++;
            }
          }
        } else {
          // Shift group to include first entry
          last_val += now_result[0];

          /*std::cout << "Shifting group" << std::endl;*/
          /*std::cout << "n spread " << n_spread << " last_group_sz "*/
          /*          << last_group_sz << " last group val " << last_val*/
          /*          << " min val " << min_val << std::endl;*/

          if (last_val <= min_val) {
            return false;
          }
          passes++;

          if (n_spread > 1) {
            // First group takes whole spread
            uint32_t first_val = std::accumulate(
                now_result.begin() + 1, now_result.begin() + 1 + n_spread, 0);
            if (first_val <= min_val) {
              return false;
            }
            passes++;
            // Next groups are singles
            for (size_t i = 0; i < in_degree - 2; i++) {
              if (now_result[i + 1 + n_spread] > min_val) {
                passes++;
              }
            }
          } else {
            // Next groups are singles
            for (size_t i = 0; i < in_degree - 1; i++) {
              if (now_result[i + 1] > min_val) {
                passes++;
              }
            }
          }
        }
        // Combination has not large enough values to meet all conditions
        // E.g. it needs have 2 values large than the L2 threshold +
        // predecessor (min_value)
        if (passes < colls and passes < flow_num_limit) {
          return false;
        }
      }
      return true;
    }
  };

  static constexpr int factorial(int n) {
    if (n == 0 || n == 1)
      return 1;
    return factorial(n - 1) * n;
  }

  double get_p_from_beta(BetaGenerator &bt, double lambda,
                         vector<double> &now_dist, double now_n,
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
    std::fill(nt.begin(), nt.end(), 0.0);
    double lambda = this->n_old * xi / static_cast<double>(w);

    printf("[EM_GFCM] ******** Running for degree %2d with a size of "
           "%12zu\t\t"
           "**********\n",
           xi, this->counters[d][xi].size());

    // FCM 1 degree does not care about thresholds and thus can be calculated
    // on distribution instead of individual counters
    if (xi == 1) {
      for (uint32_t i = 0; i < this->counter_dist[d][xi].size(); i++) {
        if (this->counter_dist[d][xi][i] == 0) {
          continue;
        }

        BetaGenerator alpha(i, xi, this->thresholds[d][xi][i]),
            beta(i, xi, this->thresholds[d][xi][i]);
        double sum_p = 0.0;

        // Sum over first combinations
        while (alpha.get_next()) {
          double p =
              get_p_from_beta(alpha, lambda, this->dist_old, this->n_old, xi);
          sum_p += p;
        }

        if (sum_p == 0.0) {
          continue;
        } else {
          while (beta.get_next()) {
            double p =
                get_p_from_beta(beta, lambda, this->dist_old, this->n_old, xi);
            for (size_t j = 0; j < beta.now_flow_num; ++j) {
              nt[beta.now_result[j]] +=
                  this->counter_dist[d][xi][i] * p / sum_p;
            }
          }
        }
      }
    } else {
      for (uint32_t i = 0; i < this->counters[d][xi].size(); i++) {
        if (this->counters[d][xi][i] == 0) {
          continue;
        }

        if (this->thresholds[d][xi].size() <= i) {
          std::cout << "Out of bound for thresholds" << std::endl;
          std::cout << this->thresholds[d][xi].size() << std::endl;
          std::cout << "At " << xi << " " << i << " with count "
                    << this->counters[d][xi][i] << " ";
          exit(1);
        }
        BetaGenerator alpha(this->counters[d][xi][i], xi,
                            this->thresholds[d][xi][i]),
            beta(this->counters[d][xi][i], xi, this->thresholds[d][xi][i]);
        double sum_p = 0.0;
        uint32_t it = 0;

        // Sum over first combinations
        /*std::cout << "Found val " << this->counters[d][xi][i] <<
         * std::endl;*/
        while (alpha.get_next()) {
          double p =
              get_p_from_beta(alpha, lambda, this->dist_old, this->n_old, xi);
          sum_p += p;
          it++;
        }

        std::cout << "Val " << this->counters[d][xi][i] << " found sum_p "
                  << sum_p << " with " << alpha.total_combi << " combinations"
                  << std::endl;
        for (auto &t : this->thresholds[d][xi][i]) {
          std::cout << "<";
          for (auto &x : t) {
            std::cout << x;
            if (&x != &t.back()) {
              std::cout << ", ";
            }
          }
          std::cout << "> ";
        }
        std::cout << std::endl;

        // If there where valid combinations, but value of combinations where
        // not found in measured data. We
        if (sum_p == 0.0) {
          if (it > 0) {
            uint32_t temp_val = this->counters[d][xi][i];
            vector<vector<uint32_t>> temp_thresh = this->thresholds[d][xi][i];

            /*std::cout << "adjust value at " << i << " with val " <<
             * temp_val*/
            /*          << std::endl;*/
            /*for (auto &t : temp_thresh) {*/
            /*  std::cout << "<";*/
            /*  for (auto &x : t) {*/
            /*    std::cout << x;*/
            /*    if (&x != &t.back()) {*/
            /*      std::cout << ", ";*/
            /*    }*/
            /*  }*/
            /*  std::cout << "> ";*/
            /*}*/
            /*std::cout << std::endl;*/

            // Remove l1 collisions, keep one flow
            temp_val -= temp_thresh.back()[1] * (xi - 1);
            if (temp_thresh.size() == 3) {
              temp_val -= temp_thresh[1][1] * (temp_thresh[1][0] - 1);
            }

            /*std::cout << "Storing 1 at " << temp_val << std::endl;*/
            nt[temp_val] += 1;
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
    }

    double accum = std::accumulate(nt.begin(), nt.end(), 0.0);
    if (0) {
      for (size_t i = 0; i < nt.size(); i++) {
        if (nt[i] != 0) {
          std::cout << i << ":" << nt[i] << " ";
        }
      }
      std::cout << std::endl;
    }
    if (this->counters[d][xi].size() != 0)
      printf("[EM_GFCM] ******** depth %d degree %2d is "
             "finished...(accum:%10.1f #val:%8d)\t**********\n",
             d, xi, accum, (int)this->counters[d][xi].size());
  }

public:
  void next_epoch() {
    std::cout << "[EM_GFCM] Start next epoch" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    this->n_old = this->n_new;
    this->dist_old = this->dist_new;

    std::cout << "[EM_GFCM] Copy first degree distribution" << std::endl;

    // Always copy first degree as this is can be considered a perfect
    // estimation. qWaterfall is not perfect, but assumed to be
    vector<vector<vector<double>>> nt(DEPTH);
    for (size_t d = 0; d < DEPTH; d++) {
      nt[d].resize(this->max_degree[d] + 1);

      /*nt[d][1].resize(this->counter_dist[d][1].size());*/
      /*for (size_t i = 0; i < this->counter_dist[d][1].size(); i++) {*/
      /*  nt[d][1][i] = this->counter_dist[d][1][i];*/
      /*}*/
    }

    std::fill(this->ns.begin(), this->ns.end(), 0);
    std::cout << "[EM_GFCM] Init first degree" << std::endl;
    // Simple Multi thread
    vector<vector<std::thread>> threads(DEPTH);
    for (size_t d = 0; d < threads.size(); d++) {
      threads[d].resize(this->max_degree[d] + 1);
    }

    uint32_t total_degree = this->max_degree[0] + this->max_degree[1] + 1;
    std::cout << "[EM_GFCM] Created " << total_degree << " threads"
              << std::endl;

    if (0) {
      for (size_t d = 0; d < DEPTH; d++) {
        for (size_t t = 1; t < threads[d].size(); t++) {
          std::cout << "[EM_GFCM] Start thread " << t << " at depth " << d
                    << std::endl;
          threads[d][t] = std::thread(&EM_GFCM::calculate_degree, *this,
                                      std::ref(nt[d][t]), d, t);
        }
      }

      std::cout << "[EM_GFCM] Started all threads, wait for them to finish..."
                << std::endl;

      for (size_t d = 0; d < DEPTH; d++) {
        for (size_t t = 1; t < threads[d].size(); t++) {
          threads[d][t].join();
        }
      }
    } else {
      // Single threaded
      for (size_t d = 0; d < DEPTH; d++) {
        for (size_t xi = 1; xi <= this->max_degree[d]; xi++) {
          this->calculate_degree(nt[d][xi], d, xi);
        }
      }
    }

    std::cout << "[EM_GFCM] Finished calculating nt per degree" << std::endl;

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
    std::cout << std::endl;

    auto stop = std::chrono::high_resolution_clock::now();
    auto time =
        std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);

    printf("[EM_GFCM - iter %2d] Compute time : %li\n", iter, time.count());
    printf("[EM_GFCM - iter %2d] Intermediate cardianlity : "
           "%9.1f\n\n",
           iter, this->n_new);
    iter++;

    /*print_stats();*/
  }

  void print_stats() {
    std::cout << "ns : " << std::endl;
    for (size_t i = 0; i < this->ns.size(); i++) {
      if (this->ns[i] != 0) {
        std::cout << i << ":" << this->ns[i] << " ";
      }
    }
    std::cout << std::endl;

    std::cout << "Dist_new: " << std::endl;
    for (uint32_t i = 0; i < this->dist_new.size(); i++) {
      if (this->dist_new[i] != 0) {
        std::cout << i << ":" << this->dist_new[i] << " ";
      }
    }
    std::cout << std::endl;
  }
};

#endif
