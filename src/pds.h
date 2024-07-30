#ifndef _PDS_H
#define _PDS_H

#include "common.h"

class PDS {
public:
  virtual ~PDS() = default;
  virtual int insert(FIVE_TUPLE tuple) {
    std::cout << "Not implemented!" << std::endl;
    return -1;
  };
  virtual int remove(FIVE_TUPLE tuple) {
    std::cout << "Not implemented!" << std::endl;
    return -1;
  }
  virtual int lookup(FIVE_TUPLE tuple) {
    std::cout << "Not implemented!" << std::endl;
    return -1;
  }
  virtual void reset() {
    std::cout << "Not implemented!" << std::endl;
    return;
  }
  virtual void analyze(std::unordered_map<string, uint32_t> true_data) {
    std::cout << "Not implemented!" << std::endl;
    return;
  }
  virtual void store_data(int epoch) {
    std::cout << "Not implemented!" << std::endl;
    return;
  }
  virtual void print_sketch() { std::cout << "Not implemented!" << std::endl; }
};

#endif // !_PDS_H
