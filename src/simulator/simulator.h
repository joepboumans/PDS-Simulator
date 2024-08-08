#ifndef _SIMULATOR_H
#define _SIMULATOR_H

#include "../common.h"
#include "../pds.h"
#include <cstdint>
#include <unordered_map>

class Simulator {
public:
  Simulator(vector<PDS *> pds, int n_pds, float epoch_len) {
    this->n_pds = n_pds;
    this->pds = pds;
    this->epoch_len = epoch_len;
  }
  Simulator(Simulator &&) = default;
  Simulator(const Simulator &) = default;
  Simulator &operator=(Simulator &&) = default;
  Simulator &operator=(const Simulator &) = default;

  vector<std::unordered_map<string, uint32_t>> true_data_sets;

  int run(const TRACE &trace, unsigned int duration);

private:
  vector<PDS *> pds;
  int n_pds;
  float epoch_len;
  int insert(const TRACE &trace, int start, int end);
  int analyze();
  int store();
};

#endif // !_SIMULATOR_H
