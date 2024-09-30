#ifndef _EMALGORITHM_WATERFALL_FCM_HPP
#define _EMALGORITHM_WATERFALL_FCM_HPP

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
#include <sstream>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

using std::unordered_map;
using std::vector;

class EMFSD {
  vector<vector<uint32_t>> counters;
  uint32_t w;                            // width of counters
  vector<double> dist_old, dist_new;     // for ratio \phi
  vector<vector<uint32_t>> counter_dist; // initial counter values
  vector<vector<vector<vector<uint32_t>>>> thresholds;
  vector<uint32_t> stage_sz;

public:
  vector<double> ns; // for integer n
  double n_sum;
  double card_init; // initial cardinality by MLE
  uint32_t iter = 0;
  bool inited = false;
  EMFSD(vector<uint32_t> szes, vector<vector<vector<vector<uint32_t>>>> thresh,
        uint32_t in_max_value, uint32_t max_degree, uint32_t card,
        vector<vector<uint32_t>> counters)
      : stage_sz(szes), max_degree(max_degree), max_counter_value(in_max_value),
        counters(counters) {

    // Setup distribution and the thresholds for it
    this->counter_dist = vector<vector<uint32_t>>(
        max_degree + 1, vector<uint32_t>(this->max_counter_value + 1, 0));
    this->thresholds.resize(counters.size());
    for (size_t d = 0; d < counters.size(); d++) {
      if (counters[d].size() == 0) {
        continue;
      }
      this->thresholds[d].resize(this->max_counter_value + 1);
    }
    // Inital guess for # of flows
    this->n_new = 0.0; // # of flows (Cardinality)
    for (size_t d = 0; d < counters.size(); d++) {
      this->n_new += counters[d].size();
      for (size_t i = 0; i < counters[d].size(); i++) {
        this->counter_dist[d][counters[d][i]]++;
        this->counters[d][counters[d][i]] = counters[d][i];
        this->thresholds[d][counters[d][i]] = thresh[d][i];
      }
    }
    this->w = this->stage_sz[0];

    // Inital guess for Flow Size Distribution (Phi)
    this->dist_new.resize(this->max_counter_value + 1);
    for (auto &degree : counters) {
      for (auto count : degree) {
        this->dist_new[count]++;
      }
    }
    this->ns.resize(this->max_counter_value + 1);
    for (size_t d = 0; d < this->counter_dist.size(); d++) {
      for (size_t i = 0; i < this->counter_dist[d].size(); i++) {
        if (this->counter_dist[d].size() == 0) {
          continue;
        }
        this->dist_new[i] += this->counter_dist[d][i];
        this->ns[i] += this->counter_dist[d][i];
      }
    }
    // Normalize over inital cardinality
    for (size_t i = 0; i < this->dist_new.size(); i++) {
      this->dist_new[i] /= this->n_new;
    }
    printf("[EM_WATERFALL_FCM] Initial Cardinality : %9.1f\n", this->n_new);
    printf("[EM_WATERFALL_FCM] Max Counter value : %d\n",
           this->max_counter_value);
    printf("[EM_WATERFALL_FCM] Max max degree : %d\n", this->max_degree);
  };

private:
  double n_old,
      n_new; // cardinality
  uint32_t max_counter_value, max_degree;
  struct BetaGenerator {
    int sum;
    int now_flow_num;
    int flow_num_limit;
    vector<int> now_result;

    explicit BetaGenerator(uint32_t _sum, uint32_t _in_degree)
        : sum(_sum), flow_num_limit(_in_degree) {
      now_flow_num = flow_num_limit;
      now_result.resize(_in_degree);
      now_result[0] = 1;

      // if (sum > 600) {
      //   flow_num_limit = 2;
      // } else if (sum > 250)
      //   flow_num_limit = 3;
      // else if (sum > 100)
      //   flow_num_limit = 4;
      // else if (sum > 50)
      //   flow_num_limit = 5;
      // else
      //   flow_num_limit = 6;
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
        now_result.resize(now_flow_num);
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
          for (int i = 0; i < now_flow_num - 2; ++i) {
            now_result[i] = 1;
          }
          now_result[now_flow_num - 2] = 0;
        }
      }

      return false;
    }

    bool check_condition() { return true; }
  };

  int factorial(int n) {
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
      if (lambda_i > 0) {
        for (auto &x : bt.now_result) {
          std::cout << x << " ";
        }
        std::cout << std::endl;
        std::cout << lambda_i << " " << now_dist[si] << std::endl;
        std::cout << si << " " << fi << std::endl;
        // std::cout << si << " " << fi << std::endl;
      }
      ret *= (std::pow(lambda_i, fi)) / factorial(fi);
    }

    return ret;
  }

  // double get_p_from_beta(BetaGenerator &bt, double lambda,
  //                        vector<double> now_dist, double now_n,
  //                        uint32_t degree) {
  //   std::unordered_map<uint32_t, uint32_t> mp;
  //   for (int i = 0; i < bt.now_flow_num; ++i) {
  //     mp[bt.now_result[i]]++;
  //   }
  //
  //   double ret = std::exp(-lambda);
  //   for (auto &kv : mp) {
  //     uint32_t fi = kv.second;
  //     uint32_t si = kv.first;
  //     double lambda_i = (now_n * now_dist[si] * degree) / w;
  //     ret *= (std::pow(lambda_i, fi)) / factorial(fi);
  //   }
  //
  //   return ret;
  // }

  void calculate_degree(vector<double> &nt, int d) {
    nt.resize(this->max_counter_value + 1);
    std::fill(nt.begin(), nt.end(), 0.0);

    printf("[EM_WATERFALL_FCM] ******** Running for degree %2d with a size of "
           "%12zu\t\t"
           "**********\n",
           d, counter_dist[d].size());

    double lambda = n_old * d / double(w);
    for (uint32_t i = 0; i < counter_dist[d].size(); i++) {
      // enum how to form val:i
      if (counter_dist[d][i] == 0) {
        continue;
      }
      // std::cout << i << std::endl;
      BetaGenerator alpha(i, d), beta(i, d);
      double sum_p = 0;
      uint32_t it = 0;
      while (alpha.get_next()) {
        double p = get_p_from_beta(alpha, lambda, dist_old, n_old, d);
        sum_p += p;
        it++;
      }

      if (sum_p == 0.0) {
        if (it > 0) {
          uint32_t temp_val = this->counters[d][i];
          vector<vector<uint32_t>> temp_thresh = this->thresholds[d][i];
          // Start from lowest layer to highest layer
          std::reverse(temp_thresh.begin(), temp_thresh.end());
          for (auto &t : temp_thresh) {
            if (temp_val < t[1] * (t[0] - 1)) {
              break;
            }
            temp_val -= t[1] * (t[0] - 1);
            nt[temp_val] += 1;
          }
        }
      } else {
        while (beta.get_next()) {
          double p = get_p_from_beta(beta, lambda, dist_old, n_old, d);
          for (size_t j = 0; j < beta.now_flow_num; ++j) {
            nt[beta.now_result[j]] += counter_dist[d][i] * p / sum_p;
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
    if (counter_dist[d].size() != 0)
      printf("[EM_WATERFALL_FCM] ******** degree %2d is "
             "finished...(accum:%10.1f #val:%8d)\t**********\n",
             d, accum, (int)this->counters[d].size());
  }

public:
  void next_epoch() {
    auto start = std::chrono::high_resolution_clock::now();
    double lambda = n_old / double(w);
    dist_old = dist_new;
    n_old = n_new;

    vector<vector<double>> nt;
    nt.resize(max_degree + 1);
    std::fill(ns.begin(), ns.end(), 0);

    // Always copy first degree as this is the already perfect estimation
    nt[1].resize(counter_dist[1].size());
    for (size_t i = 0; i < counter_dist[1].size(); i++) {
      nt[1][i] = counter_dist[1][i];
    }
    // Simple Multi thread
    std::thread threads[max_degree + 1];
    for (size_t t = 2; t <= max_degree; t++) {
      threads[t] =
          std::thread(&EMFSD::calculate_degree, *this, std::ref(nt[t]), t);
    }
    for (size_t t = 2; t <= max_degree; t++) {
      threads[t].join();
    }

    // Single threaded
    // for (size_t d = 0; d < nt.size(); d++) {
    //   if (d == 0) {
    //     this->calculate_single_degree(nt[d], d);
    //   } else {
    //     this->calculate_higher_degree(nt[d], d);
    //   }
    // }

    n_new = 0.0;
    for (size_t d = 0; d < nt.size(); d++) {
      if (nt[d].size() > 0) {
        std::cout << "Size of nt[" << d << "] " << nt[d].size() << std::endl;
      }
      for (uint32_t i = 0; i < nt[d].size(); i++) {
        ns[i] += nt[d][i];
        n_new += nt[d][i];
      }
    }
    for (uint32_t i = 0; i < ns.size(); i++) {
      dist_new[i] = ns[i] / n_new;
    }
    auto stop = std::chrono::high_resolution_clock::now();
    auto time = duration_cast<std::chrono::milliseconds>(stop - start);

    printf("[EM_WATERFALL_FCM - iter %2d] Compute time : %li\n", iter,
           time.count());
    printf("[EM_WATERFALL_FCM - iter %2d] Intermediate cardianlity : %9.1f\n\n",
           iter, n_new);
    iter++;
    std::cout << "ns : ";
    for (size_t i = 0; i < ns.size(); i++) {
      if (ns[i] != 0) {
        std::cout << i << " = " << ns[i] << "\t";
      }
    }
    std::cout << std::endl;
  }
};
#endif
