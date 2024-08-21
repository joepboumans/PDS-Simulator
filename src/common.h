#ifndef _COMMON_H
#define _COMMON_H

#include "BOBHash32.h"
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
  unsigned char srcIp[4] = {0, 0, 0, 0};
  unsigned char dstIp[4] = {0, 0, 0, 0};
  unsigned char srcPort[2] = {0, 0};
  unsigned char dstPort[2] = {0, 0};
  unsigned char protocol = 0;

  friend std::ostream &operator<<(std::ostream &os, FIVE_TUPLE const &tuple) {
    char srcIp[16];
    sprintf(srcIp, "%i.%i.%i.%i", (int)tuple.srcIp[0], (int)tuple.srcIp[1],
            (int)tuple.srcIp[2], (int)tuple.srcIp[3]);
    char destIp[16];
    sprintf(destIp, "%i.%i.%i.%i", (int)tuple.dstIp[0], (int)tuple.dstIp[1],
            (int)tuple.dstIp[2], (int)tuple.dstIp[3]);
    char srcPort[6];
    sprintf(srcPort, "%i", (int)((tuple.srcPort[0] << 8) | tuple.srcPort[1]));
    char dstPort[6];
    sprintf(dstPort, "%i", (int)((tuple.dstPort[0] << 8) | tuple.dstPort[1]));
    char protocol[6];
    sprintf(protocol, "%i", (int)tuple.protocol);
    return os << srcIp << ":" << srcPort << "|" << destIp << ":" << dstPort
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
    for (auto &src : this->srcIp) {
      if (src >= std::numeric_limits<unsigned char>::max()) {
        src++;
        continue;
      }
      src++;
      return *this;
    }
    for (auto &dst : this->dstIp) {
      if (dst >= std::numeric_limits<unsigned char>::max()) {
        dst++;
        continue;
      }
      dst++;
      return *this;
    }
    for (auto &src : this->srcPort) {
      if (src >= std::numeric_limits<unsigned char>::max()) {
        src++;
        continue;
      }
      src++;
      return *this;
    }
    for (auto &dst : this->dstPort) {
      if (dst >= std::numeric_limits<unsigned char>::max()) {
        dst++;
        continue;
      }
      dst++;
      return *this;
    }
    if (this->protocol >= std::numeric_limits<unsigned char>::max()) {
      return *this;
    }
    this->protocol++;
    return *this;
  }

  FIVE_TUPLE operator++(int) {
    FIVE_TUPLE old = *this;
    operator++();
    return old;
  }

  FIVE_TUPLE operator+(int i) {
    FIVE_TUPLE new_tuple = *this;
    for (auto &src : new_tuple.srcIp) {
      if (src >= std::numeric_limits<unsigned char>::max()) {
        src += i;
        continue;
      }
      src += i;
      return new_tuple;
    }
    for (auto &dst : new_tuple.dstIp) {
      if (dst >= std::numeric_limits<unsigned char>::max()) {
        dst += i;
        continue;
      }
      dst += i;
      return new_tuple;
    }
    for (auto &src : new_tuple.srcPort) {
      if (src >= std::numeric_limits<unsigned char>::max()) {
        src += i;
        continue;
      }
      src += i;
      return new_tuple;
    }
    for (auto &dst : new_tuple.dstPort) {
      if (dst >= std::numeric_limits<unsigned char>::max()) {
        dst += i;
        continue;
      }
      dst += i;
      return new_tuple;
    }
    if (new_tuple.protocol >= std::numeric_limits<unsigned char>::max()) {
      new_tuple.protocol += i;
      return new_tuple;
    }
    new_tuple.protocol += i;
    return new_tuple;
  }

  FIVE_TUPLE operator^=(FIVE_TUPLE tup) {
    *this = *this ^ tup;
    return *this;
  }

  FIVE_TUPLE operator^(FIVE_TUPLE tup) {
    for (size_t i = 0; i < 4; i++) {
      this->srcIp[i] ^= tup.srcIp[i];
      this->dstIp[i] ^= tup.dstIp[i];
    }
    for (size_t i = 0; i < 2; i++) {
      this->srcPort[i] ^= tup.srcPort[i];
      this->dstPort[i] ^= tup.dstPort[i];
    }
    this->protocol ^= tup.protocol;
    return *this;
  }

  auto operator<=>(const FIVE_TUPLE &) const = default;
  bool operator==(const FIVE_TUPLE &rhs) const {

    if (this->protocol != rhs.protocol) {
      return false;
    }

    for (size_t i = 0; i < 4; i++) {
      if (this->srcIp[i] != rhs.srcIp[i]) {
        return false;
      }
      if (this->dstIp[i] != rhs.dstIp[i]) {
        return false;
      }
    }
    for (size_t i = 0; i < 2; i++) {
      if (this->srcPort[i] != rhs.srcPort[i]) {
        return false;
      }
      if (this->dstPort[i] != rhs.dstPort[i]) {
        return false;
      }
    }
    return true;
  }
};

struct tupleHash {
  std::size_t operator()(const FIVE_TUPLE &k) const {
    using std::size_t;
    BOBHash32 hasher(750);
    char c_ftuple[sizeof(FIVE_TUPLE)];
    memcpy(c_ftuple, &k, sizeof(FIVE_TUPLE));
    return hasher.run(c_ftuple, 4);
  }
};

typedef vector<FIVE_TUPLE> TRACE;
#endif
