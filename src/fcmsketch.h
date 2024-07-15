#ifndef _FCM_SKETCH_H
#define _FCM_SKETCH_H

#include "common.h"
#include "counter.h"
#include "pds.h"
#include <cstdint>
#include <vector>
class FCM_Sketch : public PDS {
public:
  std::vector<vector<Counter>> stages;
  FCM_Sketch(uint32_t n_roots, uint32_t n_stages = 3, uint32_t k = 8) {
    // Check if structure is possible, max counter is 32bit

    for (size_t i = 0; i < n_stages; i++) {
      int j = 0;
    }
  }
  ~FCM_Sketch() {
    for (auto v : this->stages) {
      for (auto c : v) {
        int j = 0;
      }
    }
  }
  int insert(FIVE_TUPLE tuple) { return 0; }
  int remove(FIVE_TUPLE tuple);
  int lookup(FIVE_TUPLE tuple);

private:
};

#endif // !_FCM_SKETCH_H
