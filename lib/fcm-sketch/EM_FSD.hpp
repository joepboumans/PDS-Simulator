#ifndef _EMALGORITHM_FCM_HPP
#define _EMALGORITHM_FCM_HPP

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <numeric>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

using std::unordered_map;
using std::vector;

// Degree = 1
class EMFSD {
  uint32_t w;                            // width of counters
  vector<vector<uint32_t>> counter_dist; // initial counter values
  vector<double> dist_old, dist_new;     // for ratio \phi
  vector<uint32_t> stage_sz;
  vector<vector<vector<vector<uint32_t>>>> thresholds;

public:
  vector<double> ns; // for integer n
  double n_sum;      // n_new
  double card_init;  // initial cardinality by MLE
  bool inited = false;
  EMFSD(vector<uint32_t> szes, vector<vector<vector<vector<uint32_t>>>> thresh,
        uint32_t max_counter_value, uint32_t max_degree,
        vector<vector<uint32_t>> counters)
      : stage_sz(szes), max_counter_value(max_counter_value) {
    // Sizing
    this->max_degree = max_degree;

    // Setup distribution and the thresholds for it
    counter_dist.resize(max_degree + 1);
    thresholds.resize(max_degree + 1);
    for (size_t d = 0; d <= max_degree; d++) {
      counter_dist[d].resize(max_counter_value + 1);
      std::fill(counter_dist[d].begin(), counter_dist[d].end(), 0);
      thresholds[d].resize(max_counter_value + 1);
    }
    // Inital guess for # of flows
    // double n_new = 0.0; // # of flows (Cardinality)
    for (size_t d = 0; d <= max_degree; d++) {
      n_new += counters[d].size();
      for (size_t i = 0; i < counters[d].size(); i++) {
        counter_dist[d][counters[d][i]]++;
        thresholds[d][counters[d][i]] = thresh[d][i];
      }
    }
    w = this->stage_sz[0];

    // Inital guess for Flow Size Distribution (Phi)
    dist_new.resize(max_counter_value + 1);
    for (auto &degree : counters) {
      for (auto count : degree) {
        dist_new[count]++;
      }
    }
    ns.resize(max_counter_value + 1);
    for (size_t d = 0; d < max_degree; d++) {
      for (size_t i = 0; i < counter_dist.size(); i++) {
        dist_new[i] += counter_dist[d][i] / double(w - counter_dist[d][0]);
        ns[i] += counter_dist[d][i];
      }
    }
    double accumulate = std::accumulate(counter_dist[max_degree].begin(),
                                        counter_dist[max_degree].end(), 0.0);
    std::cout << "Max degree size " << accumulate << std::endl;
    accumulate =
        std::accumulate(counter_dist[3].begin(), counter_dist[3].end(), 0.0);
    std::cout << "Max degree size " << accumulate;
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

    explicit BetaGenerator(int _sum) : sum(_sum) {
      now_flow_num = 0;

      if (sum > 600) {
        flow_num_limit = 2;
      } else if (sum > 250)
        flow_num_limit = 3;
      else if (sum > 100)
        flow_num_limit = 4;
      else if (sum > 50)
        flow_num_limit = 5;
      else
        flow_num_limit = 6;
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
        switch (now_flow_num) {
        case 0:
          now_flow_num = 1;
          now_result.resize(1);
          now_result[0] = sum;
          return true;
        case 1:
          now_flow_num = 2;
          now_result[0] = 0;
          // fallthrough
        default:
          now_result.resize(now_flow_num);
          if (get_new_comb()) {
            return true;
          } else {
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
  };

  struct BetaGenerator_HD {
    int sum;                            // value
    int beta_degree;                    // degree
    vector<vector<uint32_t>> thres;     // list of <. . .>
    vector<uint32_t> counter_overflows; // List of overflow values per stage
    int in_beta_degree;
    bool use_reduction;
    int now_flow_num;
    int flow_num_limit;
    int correct_combinations = 0;
    vector<int> now_result;
    explicit BetaGenerator_HD(int _sum, vector<vector<uint32_t>> _thres,
                              vector<uint32_t> _overflows, int _degree)
        : sum(_sum), beta_degree(_degree), thres(_thres),
          counter_overflows(_overflows) {
      // must initialize now_flow_num as 0
      now_flow_num = 0;
      in_beta_degree = 0;
      use_reduction = false;

      if (beta_degree > 2) {
        std::cout << "Larger degree " << beta_degree << std::endl;
      }
      /**** For large dataset and cutoff ****/
      if (sum > 1100)
        flow_num_limit = beta_degree;
      else if (sum > 550)
        flow_num_limit = std::max(beta_degree, 3);
      else
        flow_num_limit = std::max(beta_degree, 4);

      // Note that the counters of degree D (> 1) includes at least D flows.
      // But, the combinatorial complexity also increases to O(N^D), which takes
      // a lot of time. To truncate the compexity, we introduce a simplified
      // version that prohibits the number of flows less than D. As
      // explained in the paper, its effect is quite limited as the number of
      // such counters are only a few.

      // "in_beta_degree" keeps the original beta_degree
      // 15k, 15k, 7k
      if (beta_degree >= 4 and sum < 10000) {
        use_reduction = true;
        in_beta_degree = beta_degree;
        flow_num_limit = 3;
        beta_degree = 3;
      } else if (beta_degree >= 4 and sum >= 10000) {
        use_reduction = true;
        in_beta_degree = beta_degree;
        flow_num_limit = 2;
        beta_degree = 2;
      } else if (beta_degree == 3 and sum > 5000) {
        use_reduction = true;
        in_beta_degree = beta_degree;
        flow_num_limit = 2;
        beta_degree = 2;
      }
    }

    bool get_new_comb() {
      // input: now, always flow num >= 2..
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
      // Need to test it is available combination or not, based on newsk_thres
      // (bayesian)
      while (now_flow_num <= flow_num_limit) {
        switch (now_flow_num) {
        case 0:
          now_flow_num = 1;
          now_result.resize(1);
          now_result[0] = sum;
          /*****************************/
          if (beta_degree == 0) // will never be occured
          {
            std::cout << "[ERROR] Beta degree is 0" << std::endl;
            return true;
          }
          /*****************************/
        case 1:
          now_flow_num = 2;
          now_result[0] = 0;
        default: // fall through
          now_result.resize(now_flow_num);
          if (get_new_comb()) {
            // Validate the combinations
            // There need to more or equal numbers to the degree of VC
            if (now_flow_num < beta_degree) {
              continue;
            }
            if (!use_reduction) {
              if (condition_check_fcm()) {
                correct_combinations++;
                return true;
              }
              // Some flows are very larges and/or have
            } else {
              if (condition_check_fcm_reduction()) {
                correct_combinations++;
                return true;
              }
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
      // std::cout << "Correct combinations " << correct_combinations <<
      // std::endl;
      return false;
    }

    bool condition_check_fcm() {
      if (beta_degree == now_flow_num) {
        for (auto &t : thres) {
          uint32_t colls = t[0];
          uint32_t min_val = t[1];
          uint32_t passes = 0;
          for (uint32_t i = 0; i < now_flow_num; ++i) {
            if (now_result[i] >= min_val) {
              passes++;
            }
          }
          // Combination has not large enough values to meet all conditions
          // E.g. it needs have 2 values large than the L2 threshold +
          // predecessor (min_value)
          if (passes < colls) {
            return false;
          }
        }
        // More flows than degrees, so the flows are split up and thus we need
        // to sum some of the values
      } else {
        for (auto &t : thres) {
          uint32_t colls = t[0];
          uint32_t min_val = t[1];
          uint32_t passes = 0;
          vector<bool> perms(now_flow_num);
          for (uint32_t i = 0; i < now_flow_num; ++i) {
            // std::cout << now_result[i] << " > " << min_val << "\t";
            if (now_result[i] > min_val) {
              passes++;
            }
          }
          // std::cout << std::endl;
          // Combination has not large enough values to meet all
          // conditions
          // E.g. it needs have 2 values large than the L2 threshold +
          // predecessor (min_value)
          if (passes < colls) {
            // std::cout << "Didn't have enough passes, " << passes << " < "
            //           << colls << std::endl;
            return false;
          }
        }
        return true;
      }
      return true; // if no violation, it is a good permutation!
    }

    bool condition_check_fcm_reduction() {
      // check number of flows should be more or equal than degree...
      if (now_flow_num < beta_degree)
        return false; // exit for next comb.

      // layer 1 check
      for (int i = 0; i < now_flow_num; ++i) {
        if (now_result[i] <= this->counter_overflows[0])
          return false;
      }
      // Run over threshold until more than 2 collisions are found
      for (auto &t : thres) {
        uint32_t colls = t[0];
        colls = std::min((int)colls, beta_degree);
        uint32_t min_val = t[1];
        uint32_t passes = 0;
        for (uint32_t i = 0; i < now_flow_num; ++i) {
          if (now_result[i] >= min_val) {
            passes++;
          }
        }
        // Combination has not large enough values to meet all conditions
        // E.g. it needs have 2 values large than the L2 threshold +
        // predecessor (min_value)
        if (passes < colls) {
          // std::cout << "Didn't have enough passes, " << passes << " < "
          //           << colls << std::endl;
          return false;
        }
        // Exit early
        if (colls > 1) {
          break;
        }
      }
      return true;
    }
  };

  int factorial(int n) {
    if (n == 0 || n == 1)
      return 1;
    return factorial(n - 1) * n;
  }

  double get_p_from_beta(BetaGenerator &bt, double lambda,
                         vector<double> now_dist, double now_n) {
    std::unordered_map<uint32_t, uint32_t> mp;
    for (int i = 0; i < bt.now_flow_num; ++i) {
      mp[bt.now_result[i]]++;
    }

    double ret = std::exp(-lambda);
    for (auto &kv : mp) {
      uint32_t fi = kv.second;
      uint32_t si = kv.first;
      double lambda_i = now_n * (now_dist[si]) / w;
      // printf("si %i, fi %i, now_n %f, now_dist %f,  lambda_i %f ", si, fi,
      //        now_n, now_dist[si], lambda_i);
      ret *= (std::pow(lambda_i, fi)) / factorial(fi);
    }
    // std::cout << std::endl;

    return ret;
  }

  double get_p_from_beta(BetaGenerator_HD &bt, double lambda,
                         vector<double> now_dist, double now_n) {
    std::unordered_map<uint32_t, uint32_t> mp;
    for (int i = 0; i < bt.now_flow_num; ++i) {
      mp[bt.now_result[i]]++;
    }

    double ret = std::exp(-lambda);
    for (auto &kv : mp) {
      uint32_t fi = kv.second;
      uint32_t si = kv.first;
      double lambda_i = now_n * (now_dist[si]) / w;
      // printf("si %i, fi %i, now_n %f, now_dist %f,  lambda_i %f ", si, fi,
      //        now_n, now_dist[si], lambda_i);
      ret *= (std::pow(lambda_i, fi)) / factorial(fi);
    }
    // std::cout << std::endl;

    return ret;
  }

  void calculate_single_degree(vector<double> &nt, int d) {
    nt.resize(this->max_counter_value + 1);
    std::fill(nt.begin(), nt.end(), 0.0);

    double lambda = n_old * d / double(w);
    for (uint32_t i = 0; i < counter_dist[d].size(); i++) {
      // enum how to form val:i
      if (counter_dist[d][i] == 0) {
        continue;
      }
      BetaGenerator alpha(i), beta(i);
      double sum_p = 0;
      while (alpha.get_next()) {
        double p = get_p_from_beta(alpha, lambda, dist_old, n_old);
        sum_p += p;
      }
      while (beta.get_next()) {
        double p = get_p_from_beta(beta, lambda, dist_old, n_old);
        for (int j = 0; j < beta.now_flow_num; ++j) {
          nt[beta.now_result[j]] += counter_dist[d][i] * p / sum_p;
        }
      }
    }

    double accum = std::accumulate(nt.begin(), nt.end(), 0.0);
    if (counter_dist[d].size() != 0)
      printf("[EM_FCM] ******** degree %2d is "
             "finished...(accum:%10.1f) **********\n",
             d, accum);
  }

  void calculate_higher_degree(vector<double> &nt, int d) {
    nt.resize(this->max_counter_value + 1);
    std::fill(nt.begin(), nt.end(), 0.0);

    printf("Running for degree %2d with a size of %zu\n", d,
           counter_dist[d].size());
    double lambda = n_old * d / double(w);
    for (uint32_t i = 0; i < counter_dist[d].size(); i++) {
      // enum how to form val:i
      if (counter_dist[d][i] == 0) {
        continue;
      }
      BetaGenerator_HD alpha(i, this->thresholds[d][i], this->stage_sz, d),
          beta(i, this->thresholds[d][i], this->stage_sz, d);
      double sum_p = 0;
      while (alpha.get_next()) {
        double p = get_p_from_beta(alpha, lambda, dist_old, n_old);
        sum_p += p;
      }
      if (sum_p == 0) {
        std::cout << "Sum is zero, stop calc" << std::endl;
        std::cout << "At " << i << " with degree " << d << std::endl;
        continue;
      }
      while (beta.get_next()) {
        double p = get_p_from_beta(beta, lambda, dist_old, n_old);
        for (int j = 0; j < beta.now_flow_num; ++j) {
          nt[beta.now_result[j]] += counter_dist[d][i] * p / sum_p;
        }
      }
    }
    double accum = std::accumulate(nt.begin(), nt.end(), 0.0);
    if (counter_dist[d].size() != 0)
      printf("[EM_FCM] ******** degree %2d is "
             "finished...(accum:%10.1f) **********\n",
             d, accum);
  }

public:
  void next_epoch() {
    double lambda = n_old / double(w);
    dist_old = dist_new;
    n_old = n_new;

    vector<vector<double>> nt;
    nt.resize(max_degree + 1);
    std::fill(ns.begin(), ns.end(), 0);

    // std::thread threads[max_degree];
    // for (size_t t = 0; t < max_degree; t++) {
    //   if (t == 0) {
    //     threads[t] = std::thread(&EMFSD::calculate_single_degree, *this,
    //                              std::ref(nt[t]), t);
    //   } else {
    //     threads[t] = std::thread(&EMFSD::calculate_higher_degree, *this,
    //                              std::ref(nt[t]), t);
    //   }
    // }
    // for (size_t t = 0; t < max_degree; t++) {
    //   threads[t].join();
    // }

    for (size_t d = 0; d <= max_degree; d++) {
      if (d == 0) {
        this->calculate_single_degree(nt[d], d);
      } else {
        this->calculate_higher_degree(nt[d], d);
      }
    }

    n_new = 0.0;
    for (size_t d = 0; d < max_degree; d++) {
      for (uint32_t i = 0; i < max_counter_value; i++) {
        ns[i] += nt[d][i];
        n_new += nt[d][i];
      }
    }
    for (uint32_t i = 0; i < max_counter_value; i++) {
      dist_new[i] = ns[i] / n_new;
    }
    for (auto &x : ns) {
      if (x != 0.0) {
        std::cout << x << " ";
      }
    }
    std::cout << std::endl;
  }
};
#endif
