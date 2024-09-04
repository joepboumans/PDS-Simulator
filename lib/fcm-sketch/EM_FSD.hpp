#ifndef _EMALGORITHM_FCM_HPP
#define _EMALGORITHM_FCM_HPP

#include <cmath>
#include <cstdint>
#include <iostream>
#include <unistd.h>
#include <unordered_map>
#include <vector>

using std::unordered_map;
using std::vector;

// Degree = 1
class EMFSD_ld {
  uint32_t w;                        // width of counters
  vector<uint32_t> counter_dist;     // initial counter values
  vector<double> dist_old, dist_new; // for ratio \phi
public:
  vector<double> ns; // for integer n
  double n_sum;      // n_new
  double card_init;  // initial cardinality by MLE
  bool inited = false;

private:
  double n_old, n_new; // cardinality

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

  int factorial(int n) {
    if (n == 0 || n == 1)
      return 1;
    return factorial(n - 1) * n;
  };

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
      ret *= (std::pow(lambda_i, fi)) / factorial(fi);
    }

    return ret;
  }

public:
  EMFSD_ld() {}

  void set_counters(uint32_t max_counter_value,
                    vector<vector<uint32_t>> counters) {

    counter_dist.resize(max_counter_value + 1);
    std::fill(counter_dist.begin(), counter_dist.end(), 0);
    // Inital guess for # of flows
    // double n_new = 0.0; // # of flows (Cardinality)
    for (size_t d = 0; d < counters.size(); d++) {
      n_new += counters[d].size();
      for (size_t i = 0; i < counters[d].size(); i++) {
        counter_dist[counters[d][i]]++;
      }
    }
    w = n_new;

    // Inital guess for Flow Size Distribution (Phi)
    this->dist_new.resize(max_counter_value + 1);
    for (auto &degree : counters) {
      for (auto count : degree) {
        this->dist_new[count]++;
      }
    }
    ns.resize(counter_dist.size());
    for (uint32_t i = 1; i < counter_dist.size(); ++i) {
      this->dist_new[i] = counter_dist[i] / double(w - counter_dist[0]);
      ns[i] = counter_dist[i];
    }
  }

  void next_epoch() {
    dist_old = this->dist_new;
    n_old = n_new;

    double lambda = n_old / double(w);

    std::fill(ns.begin(), ns.end(), 0);

    for (uint32_t i = 1; i < counter_dist.size(); ++i) {
      // enum how to form val:i
      if (counter_dist[i] == 0) {
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
          ns[beta.now_result[j]] += counter_dist[i] * p / sum_p;
        }
      }
    }

    n_new = 0.0;
    for (uint32_t i = 1; i < counter_dist.size(); i++) {
      n_new += ns[i];
    }
    for (uint32_t i = 1; i < counter_dist.size(); i++) {
      dist_new[i] = ns[i] / n_new;
    }

    n_sum = n_new;
    // for (auto &x : ns) {
    //   std::cout << x << " ";
    // }
    // std::cout << std::endl;
  }
};

// class EMFSD_hd {
//   uint32_t w;                        // width of counters
//   vector<int> counter_dist;          // initial counter values
//   vector<double> dist_old, dist_new; // for ratio \phi
// public:
//   vector<double> ns; // for integer n
//   double n_sum;      // n_new
//   double card_init;  // initial cardinality by MLE
//   bool inited = false;
//
// private:
//   double n_old, n_new; // cardinality
//
//   struct BetaGenerator {
//     int sum;                            // value
//     int beta_degree;                    // degree
//     vector<vector<uint32_t>> thres;     // list of <. . .>
//     vector<uint32_t> counter_overflows; // List of overflow values per stage
//     int in_beta_degree;
//     int now_flow_num;
//     int flow_num_limit;
//     vector<int> now_result;
//
//     explicit BetaGenerator(int _sum, vector<vector<uint32_t>> _thres,
//                            int _degree)
//         : sum(_sum), beta_degree(_degree) {
//       // must initialize now_flow_num as 0
//       now_flow_num = 0;
//       in_beta_degree = 0;
//       thres = _thres; // interpretation of threshold
//
//       /**** For large dataset and cutoff ****/
//       if (sum > 1100)
//         flow_num_limit = beta_degree;
//       else if (sum > 550)
//         flow_num_limit = std::max(beta_degree, 3);
//       else
//         flow_num_limit = std::max(beta_degree, 4);
//
//       // Note that the counters of degree D (> 1) includes at least D flows.
//       // But, the combinatorial complexity also increases to O(N^D), which
//       takes
//       // a lot of time. To truncate the compexity, we introduce a simplified
//       // version that prohibits the number of flows less than D. As explained
//       in
//       // the paper, its effect is quite limited as the number of such
//       counters
//       // are only a few.
//
//       // "in_beta_degree" keeps the original beta_degree
//       // 15k, 15k, 7k
//       if (beta_degree >= 4 and sum < 10000) {
//         in_beta_degree = beta_degree;
//         flow_num_limit = 3;
//         beta_degree = 3;
//       } else if (beta_degree >= 4 and sum >= 10000) {
//         in_beta_degree = beta_degree;
//         flow_num_limit = 2;
//         beta_degree = 2;
//       } else if (beta_degree == 3 and sum > 5000) {
//         in_beta_degree = beta_degree;
//         flow_num_limit = 2;
//         beta_degree = 2;
//       }
//     }
//
//     bool get_new_comb() {
//       // input: now, always flow num >= 2..
//       for (int j = now_flow_num - 2; j >= 0; --j) {
//         int t = ++now_result[j];
//         for (int k = j + 1; k < now_flow_num - 1; ++k) {
//           now_result[k] = t;
//         }
//         int partial_sum = 0;
//         for (int k = 0; k < now_flow_num - 1; ++k) {
//           partial_sum += now_result[k];
//         }
//         int remain = sum - partial_sum;
//         if (remain >= now_result[now_flow_num - 2]) {
//           now_result[now_flow_num - 1] = remain;
//           return true;
//         }
//       }
//       return false;
//     }
//
//     bool get_next() {
//       // Need to test it is available combination or not, based on
//       newsk_thres
//       // (bayesian)
//       while (now_flow_num <= flow_num_limit) {
//         switch (now_flow_num) {
//         case 0:
//           now_flow_num = 1;
//           now_result.resize(1);
//           now_result[0] = sum;
//           /*****************************/
//           if (beta_degree == 1) // will never be occured
//             return true;
//           /*****************************/
//         case 1:
//           now_flow_num = 2;
//           now_result[0] = 0;
//         default: // fall through
//           now_result.resize(now_flow_num);
//           if (get_new_comb()) {
//             /******* condition_check *******/
//             if (in_beta_degree == 0) {
//               if (condition_check_fcm())
//                 return true;
//             } else if (in_beta_degree == 3) {
//               if (condition_check_fcm_simple2())
//                 return true;
//             } else if (beta_degree == 3) {
//               if (condition_check_fcm_simple4to3())
//                 return true;
//             } else if (beta_degree == 2) {
//               if (condition_check_fcm_simple4to2())
//                 return true;
//             }
//             /*****************************/
//           } else { // no more combination -> go to next flow number
//             now_flow_num++;
//             for (int i = 0; i < now_flow_num - 2; ++i) {
//               now_result[i] = 1;
//             }
//             now_result[now_flow_num - 2] = 0;
//           }
//         }
//       }
//       return false;
//     }
//
//     bool condition_check_fcm() {
//       // check number of flows should be more or equal than degree...
//       if (now_flow_num < beta_degree)
//         return false; // exit for next comb.
//
//       if (beta_degree == now_flow_num) {
//         // degree 2, num 2,, or degree 3, num 3
//         // layer 1
//         for (int i = 0; i < now_flow_num; ++i) {
//           if (now_result[i] <= this->counter_overflows[0])
//             return false;
//         }
//
//         // layer 2
//         int num_cond_l2 = thres.size() - beta_degree;
//         if (num_cond_l2 == 0) { // if no condition on layer 2
//           return true;
//         } else if (num_cond_l2 == 1) { // if one condition
//           if (thres[beta_degree][1] != beta_degree) {
//             printf("[ERROR] condition is not correct");
//           }
//
//           int val = 0;
//           for (int j = 0; j < thres[beta_degree][1]; ++j)
//             val += now_result[j];
//
//           if (val <= thres[beta_degree][2])
//             return false;
//         } else if (num_cond_l2 ==
//                    2) { // if (num_cond_l2 == 2){ // for two conditions
//           int thres_l2_1 = thres[beta_degree][2];
//           int thres_l2_2 = thres[beta_degree + 1][2];
//           if (beta_degree == 2) { // degree 2
//             if (now_result[0] <= thres_l2_1 or now_result[1] <= thres_l2_2) {
//               return false;
//             }
//           } else { // degree 3
//             int val_1 = 0;
//             int val_2 = 0;
//             for (int i = 0; i < thres[beta_degree][1]; ++i)
//               val_1 += now_result[i];
//             for (int i = thres[beta_degree][1];
//                  i < thres[beta_degree][1] + thres[beta_degree + 1][1]; ++i)
//               val_2 += now_result[i];
//
//             if (val_1 <= thres_l2_1 or val_2 <= thres_l2_2) {
//               return false;
//             }
//           }
//         } else { // only when degree 3
//           if (now_result[0] <= thres[beta_degree][2] or
//               now_result[1] <= thres[beta_degree + 1][2] or
//               now_result[2] <= thres[beta_degree + 2][2])
//             return false;
//         }
//         return true; // if no violation, it is a good permutation!
//       } else if (beta_degree + 1 == now_flow_num) {
//         // degree 2, num 3
//         // layer 1
//         if (now_result[0] + now_result[1] <= this->counter_overflows[0] or
//             now_result[2] <= this->counter_overflows[0])
//           return false;
//
//         // layer 2
//         int num_cond_l2 = thres.size() - beta_degree;
//         if (num_cond_l2 == 0) { // if no condition on layer 2
//           return true;
//         } else if (num_cond_l2 == 1) { // if one condition
//           int val = 0;
//           for (int j = 0; j < thres[beta_degree][1]; ++j)
//             val += now_result[j];
//           if (val <= thres[beta_degree][2])
//             return false;
//         } else { // if two conditions
//           if (now_result[0] + now_result[1] <= thres[beta_degree][2] or
//               now_result[2] <= thres[beta_degree + 1][2])
//             return false;
//         }
//       } else if (beta_degree + 2 == now_flow_num) {
//         // degree 2, num 4
//         // layer 1
//         if (now_result[3] > this->counter_overflows[0]) {
//           if (now_result[0] + now_result[1] + now_result[2] <=
//               this->counter_overflows[0])
//             return false;
//         } else {
//           if (now_result[0] + now_result[3] <= this->counter_overflows[0] or
//               now_result[1] + now_result[2] <= this->counter_overflows[0])
//             return false;
//         }
//
//         // layer 2
//         int num_cond_l2 = thres.size() - beta_degree;
//         if (num_cond_l2 == 0) { // if no condition
//           return true;
//         } else if (num_cond_l2 == 1) { // if one condition
//           int val = 0;
//           for (int j = 0; j < thres[beta_degree][1]; ++j)
//             val += now_result[j];
//           if (val <= thres[beta_degree][2])
//             return false;
//         } else { // if two conditions
//           int thres_l2_1 = thres[beta_degree][2];
//           int thres_l2_2 = thres[beta_degree + 1][2];
//
//           if (now_result[3] > thres_l2_2) {
//             if (now_result[0] + now_result[1] + now_result[2] <= thres_l2_1)
//               return false;
//           } else {
//             if (now_result[0] + now_result[3] <= thres_l2_1 or
//                 now_result[1] + now_result[2] <= thres_l2_2)
//               return false;
//           }
//         }
//       }
//       return true; // if no violation, it is a good permutation!
//     }
//
//     bool condition_check_fcm_simple4to3() {
//       // check number of flows should be more or equal than degree...
//       if (now_flow_num < beta_degree)
//         return false; // exit for next comb.
//
//       // layer 1 check
//       for (int i = 0; i < now_flow_num; ++i) {
//         if (now_result[i] <= this->counter_overflows[0])
//           return false;
//       }
//
//       // layer 2, degree 3
//       // In this setting, degree == 3, num_flow == 3
//       int num_cond_l2 = thres.size() - in_beta_degree;
//       if (num_cond_l2 == 0) { // if no layer 2 condition
//         return true;
//       } else if (num_cond_l2 == 1) { // if one condition
//         int val = 0;
//         for (int j = 0; j < now_flow_num; ++j)
//           val += now_result[j];
//         if (val <= this->counter_overflows[1] + 3 *
//         this->counter_overflows[0])
//           return false;
//       } else if (num_cond_l2 == 2) { // if two conditions
//         if (now_result[0] + now_result[1] <=
//                 this->counter_overflows[1] + 2 * this->counter_overflows[0]
//                 or
//             now_result[2] <=
//                 this->counter_overflows[1] + this->counter_overflows[0])
//           return false;
//       } else { // if num_cond_l2 == 3, 3 conditions
//         for (int i = 0; i < now_flow_num; ++i) {
//           if (now_result[i] <=
//               this->counter_overflows[1] + 3 * this->counter_overflows[0])
//             return false;
//         }
//       }
//       return true;
//     }
//
//     bool condition_check_fcm_simple4to2() {
//       // check number of flows should be more or equal than degree...
//       if (now_flow_num < beta_degree)
//         return false; // exit for next comb.
//
//       // layer 1 check
//       for (int i = 0; i < now_flow_num; ++i) {
//         if (now_result[i] <= this->counter_overflows[0])
//           return false;
//       }
//
//       // layer 2, degree 2
//       // In this setting, degree == 2 and num_flow == 2 (but regard the
//       // origianl flow num should be 3, to reduce overestimate)
//       int num_cond_l2 =
//           thres.size() - in_beta_degree; // here, simplified is origianl
//           degree
//       if (num_cond_l2 == 0) {            // if no layer 2 condition
//         return true;
//       } else if (num_cond_l2 == 1) { // if one condition
//         if (now_result[0] + now_result[1] <=
//             this->counter_overflows[1] + 3 * this->counter_overflows[0])
//           return false;
//       } else { // if two conditions
//         if (now_result[0] <=
//                 this->counter_overflows[1] + this->counter_overflows[0] or
//             now_result[1] <=
//                 this->counter_overflows[1] + 2 * this->counter_overflows[0])
//           return false;
//       }
//       return true;
//     }
//
//     bool condition_check_fcm_simple2() {
//       // check number of flows should be more or equal than degree...
//       if (now_flow_num < beta_degree)
//         return false; // exit for next comb.
//
//       // layer 1 check
//       for (int i = 0; i < now_flow_num; ++i) {
//         if (now_result[i] <= this->counter_overflows[0])
//           return false;
//       }
//
//       // layer 2
//       // if (beta_degree == 2){ // if degree 2
//       int num_cond_l2 =
//           thres.size() - in_beta_degree; // here, simplified is origianl
//           degree
//       if (num_cond_l2 == 0) {            // if no layer 2 condition
//         return true;
//       } else if (num_cond_l2 == 1) { // if one condition
//         if (now_result[0] + now_result[1] <=
//             this->counter_overflows[1] + 2 * this->counter_overflows[0])
//           return false;
//       } else { // if two conditions
//         if (now_result[0] <=
//                 this->counter_overflows[1] + this->counter_overflows[0] or
//             now_result[1] <=
//                 this->counter_overflows[1] + 2 * this->counter_overflows[0])
//           return false;
//       }
//       // }
//       return true;
//     }
//   };
//
//   int factorial(int n) {
//     if (n == 0 || n == 1)
//       return 1;
//     return factorial(n - 1) * n;
//   };
//
//   double get_p_from_beta(BetaGenerator &bt, double lambda,
//                          vector<double> &now_dist, double now_n) {
//     std::unordered_map<uint32_t, uint32_t> mp;
//     for (int i = 0; i < bt.now_flow_num; ++i) {
//       mp[bt.now_result[i]]++;
//     }
//
//     double ret = std::exp(-lambda);
//     for (auto &kv : mp) {
//       uint32_t fi = kv.second;
//       uint32_t si = kv.first;
//       double lambda_i = now_n * (now_dist[si]) / w;
//       ret *= (std::pow(lambda_i, fi)) / factorial(fi);
//     }
//
//     return ret;
//   }
//
//   int collect_counters(vector<uint32_t> counters) {
//     // collect counter information as the dist init
//     uint32_t max_counter_val = 0;
//     for (uint32_t i = 0; i < w; ++i) {
//       max_counter_val = std::max(max_counter_val, counters[i]);
//     }
//     counter_dist.resize(max_counter_val + 1);
//     std::fill(counter_dist.begin(), counter_dist.end(), 0);
//     for (uint32_t i = 0; i < w; ++i) {
//       counter_dist[counters[i]]++;
//     }
//     return max_counter_val;
//   }
//
//   int collect_counters(vector<uint16_t> counters) {
//     // collect counter information as the dist init
//     uint16_t max_counter_val = 0;
//     for (uint32_t i = 0; i < w; ++i) {
//       max_counter_val = std::max(max_counter_val, counters[i]);
//     }
//     counter_dist.resize(max_counter_val + 1);
//     std::fill(counter_dist.begin(), counter_dist.end(), 0);
//     for (uint32_t i = 0; i < w; ++i) {
//       counter_dist[counters[i]]++;
//     }
//     std::cout << w << std::endl;
//     std::cout << counter_dist[0] << std::endl;
//     std::cout << max_counter_val << std::endl;
//
//     return max_counter_val;
//   }
//
// public:
//   EMFSD_hd() {}
//
//   void set_counters(uint32_t _w, vector<uint32_t> counters) {
//     inited = true;
//     w = _w;
//     int max_cnt = collect_counters(counters);
//     n_new = w - counter_dist[0];
//     //        std::cout << "w: " << w << std::endl;
//     //        std::cout << "dist0: " << counter_dist[0] << std::endl;
//     dist_new.resize(counter_dist.size());
//     ns.resize(counter_dist.size());
//     for (uint32_t i = 1; i < counter_dist.size(); ++i) {
//       dist_new[i] = counter_dist[i] / double(w - counter_dist[0]);
//       ns[i] = counter_dist[i];
//     }
//   }
//
//   void set_counters(uint32_t _w, vector<uint16_t> counters) {
//     inited = true;
//     w = _w;
//     int max_cnt = collect_counters(counters);
//     n_new = w - counter_dist[0];
//     dist_new.resize(counter_dist.size());
//     ns.resize(counter_dist.size());
//     for (uint32_t i = 1; i < counter_dist.size(); ++i) {
//       dist_new[i] = counter_dist[i] / double(w - counter_dist[0]);
//       ns[i] = counter_dist[i];
//     }
//     card_init = (w * std::log(w / double(counter_dist[0])));
//   }
//
//   void next_epoch() {
//     dist_old = dist_new;
//     n_old = n_new;
//
//     double lambda = n_old / double(w);
//
//     std::fill(ns.begin(), ns.end(), 0);
//
//     for (uint32_t i = 1; i < counter_dist.size(); ++i) {
//       // enum how to form val:i
//       if (counter_dist[i] == 0) {
//         continue;
//       }
//       BetaGenerator bts1(i), bts2(i);
//       double sum_p = 0;
//       while (bts1.get_next()) {
//         double p = get_p_from_beta(bts1, lambda, dist_old, n_old);
//         sum_p += p;
//       }
//       while (bts2.get_next()) {
//         double p = get_p_from_beta(bts2, lambda, dist_old, n_old);
//         for (int j = 0; j < bts2.now_flow_num; ++j) {
//           ns[bts2.now_result[j]] += counter_dist[i] * p / sum_p;
//         }
//       }
//     }
//
//     n_new = 0.0;
//     for (uint32_t i = 1; i < counter_dist.size(); i++) {
//       n_new += ns[i];
//     }
//     for (uint32_t i = 1; i < counter_dist.size(); i++) {
//       dist_new[i] = ns[i] / n_new;
//     }
//
//     n_sum = n_new;
//   }
// };
#endif
