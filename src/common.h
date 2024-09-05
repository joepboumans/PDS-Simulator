#ifndef _COMMON_H
#define _COMMON_H

#include "BOBHash32.h"
#include "xxhash.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <glob.h>
#include <iostream>
#include <iterator>
#include <limits>
#include <ostream>
#include <set>
#include <unordered_map>
#include <vector>
using std::string;
using std::vector;

struct FIVE_TUPLE {
  uint8_t num_array[13] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

  friend std::ostream &operator<<(std::ostream &os, FIVE_TUPLE const &tuple) {
    uint32_t srcPort = ((tuple.num_array[8] << 8) | tuple.num_array[9]);
    uint32_t dstPort = ((tuple.num_array[10] << 8) | tuple.num_array[11]);
    uint32_t protocol = tuple.num_array[12];
    char srcIp[16];
    sprintf(srcIp, "%i.%i.%i.%i", tuple.num_array[0], tuple.num_array[1],
            tuple.num_array[2], tuple.num_array[3]);
    char dstIp[16];
    sprintf(dstIp, "%i.%i.%i.%i", tuple.num_array[4], tuple.num_array[5],
            tuple.num_array[6], tuple.num_array[7]);

    return os << srcIp << ":" << srcPort << "|" << dstIp << ":" << dstPort
              << "|" << protocol;
  }

  operator string() {
    char c_ftuple[sizeof(FIVE_TUPLE)];
    memcpy(c_ftuple, this, sizeof(FIVE_TUPLE));
    string s_ftuple;
    s_ftuple.assign(c_ftuple, sizeof(FIVE_TUPLE));
    return s_ftuple;
  }

  FIVE_TUPLE() {}
  FIVE_TUPLE(string s_tuple) {
    const char *c_tuple = s_tuple.c_str();
    memcpy(this, c_tuple, sizeof(FIVE_TUPLE));
  }

  FIVE_TUPLE &operator++() {
    for (size_t i = 0; i < 13; i++) {
      if (this->num_array[i] >= std::numeric_limits<unsigned char>::max()) {
        this->num_array[i]++;
        continue;
      }
      this->num_array[i]++;
      return *this;
    }
    return *this;
  }

  FIVE_TUPLE operator++(int) {
    FIVE_TUPLE old = *this;
    operator++();
    return old;
  }

  FIVE_TUPLE operator+(int i) {
    FIVE_TUPLE new_tuple = *this;
    for (size_t i = 0; i < 13; i++) {
      if (new_tuple.num_array[i] >= std::numeric_limits<unsigned char>::max()) {
        new_tuple.num_array[i] += i;
        continue;
      }
      new_tuple.num_array[i] += i;
      return new_tuple;
    }
    return new_tuple;
  }

  FIVE_TUPLE operator^=(FIVE_TUPLE tup) {
    *this = *this ^ tup;
    return *this;
  }

  FIVE_TUPLE operator^(FIVE_TUPLE tup) {
    for (size_t i = 0; i < 13; i++) {
      this->num_array[i] ^= tup.num_array[i];
    }
    return *this;
  }

  auto operator<=>(const FIVE_TUPLE &) const = default;
  bool operator==(const FIVE_TUPLE &rhs) const {
    for (size_t i = 0; i < 13; i++) {
      if (this->num_array[i] != rhs.num_array[i]) {
        return false;
      }
    }
    return true;
  }
};

struct tupleHash {
  std::size_t operator()(const FIVE_TUPLE &k) const {
    // static BOBHash32 hasher(750);

    return XXH32(k.num_array, sizeof(FIVE_TUPLE), 0);
    // return hasher.run(c_ftuple, sizeof(FIVE_TUPLE));
  }
};

typedef vector<FIVE_TUPLE> TRACE;
#endif
