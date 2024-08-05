#ifndef _PDS_H
#define _PDS_H

#include "common.h"
#include <cstdint>
#include <unordered_map>

class PDS {
public:
  std::unordered_map<string, uint32_t> true_data;
  string name;
  string trace_name;

  PDS(string trace) { this->trace_name = trace; }
  ~PDS() = default;
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
  virtual void analyze(int epoch) {
    std::cout << "Not implemented!" << std::endl;
    return;
  }
  virtual void store_data(int epoch) {
    std::cout << "not implemented!" << std::endl;
    return;
  }
  virtual void setName(string in) { this->name = in; }
  virtual void print_sketch() { std::cout << "Not implemented!" << std::endl; }
};

#endif // !_PDS_H
