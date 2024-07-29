#ifndef _SIMULATOR_H
#define _SIMULATOR_H

#include "../common.h"
#include "../pds.h"
#include <unordered_map>

class Simulator {
public:
  Simulator(vector<PDS *> pds, int n_pds, float epoch) {
    this->n_pds = n_pds;
    this->pds = pds;
    this->epoch = epoch;
  }
  Simulator(Simulator &&) = default;
  Simulator(const Simulator &) = default;
  Simulator &operator=(Simulator &&) = default;
  Simulator &operator=(const Simulator &) = default;
  // ~Simulator() {
  //   for (auto p : pds) {
  //     delete p;
  //   }
  // }

  std::unordered_map<string, uint32_t> true_data;

  int run(const TRACE &trace, unsigned int duration);

private:
  vector<PDS *> pds;
  int n_pds;
  float epoch;
  int insert(const TRACE &trace, int start, int end);
  int analyze();
  int store();
};

#endif // !_SIMULATOR_H
