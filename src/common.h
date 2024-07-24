#ifndef _COMMON_H
#define _COMMON_H

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iterator>
#include <limits>
#include <ostream>
#include <vector>

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
    sprintf(destIp, "%i.%i.%i.%i", (int)tuple.srcIp[0], (int)tuple.srcIp[1],
            (int)tuple.srcIp[2], (int)tuple.srcIp[3]);
    char srcPort[6];
    sprintf(srcPort, "%i", (int)((tuple.srcPort[0] << 8) | tuple.srcPort[1]));
    char dstPort[6];
    sprintf(dstPort, "%i", (int)((tuple.dstPort[0] << 8) | tuple.dstPort[1]));
    char protocol[6];
    sprintf(protocol, "%i", (int)tuple.protocol);
    return os << srcIp << ":" << srcPort << "|" << destIp << ":" << dstPort
              << "|" << protocol;
  }

  friend FIVE_TUPLE operator+(FIVE_TUPLE &tuple, uint32_t i) {
    for (auto &src : tuple.srcIp) {
      if (src >= std::numeric_limits<char>::max()) {
        continue;
      }
      src += i;
      std::cout << "Change srcIp" << std::endl;
      std::cout << tuple << std::endl;
      return tuple;
    }
    std::cout << "Changing dstIp" << std::endl;
    for (auto &dst : tuple.dstIp) {
      if (dst >= std::numeric_limits<char>::max()) {
        continue;
      }
      dst++;
      return tuple;
    }
    for (auto &src : tuple.srcPort) {
      if (src >= std::numeric_limits<char>::max()) {
        continue;
      }
      src++;
      return tuple;
    }
    for (auto &dst : tuple.dstPort) {
      if (dst >= std::numeric_limits<char>::max()) {
        continue;
      }
      dst++;
      return tuple;
    }
    if (tuple.protocol >= std::numeric_limits<char>::max()) {
      return tuple;
    }
    tuple.protocol++;
    return tuple;
  }
};

using std::vector;
typedef vector<FIVE_TUPLE> TRACE;
#endif
