#ifndef _COMMON_H
#define _COMMON_H

#include <cstdio>
#include <iostream>
#include <vector>

struct FIVE_TUPLE {
  unsigned char srcIp[4];
  unsigned char dstIp[4];
  unsigned char srcPort[2];
  unsigned char dstPort[2];
  unsigned char protocol;
};

inline void print_five_tuple(const FIVE_TUPLE tuple) {
  char srcIp[16];
  sprintf(srcIp, "%i.%i.%i.%i", (int) tuple.srcIp[0], (int) tuple.srcIp[1], (int) tuple.srcIp[2], (int) tuple.srcIp[3]);
  char destIp[16];
  sprintf(destIp, "%i.%i.%i.%i", (int) tuple.srcIp[0], (int) tuple.srcIp[1], (int) tuple.srcIp[2], (int) tuple.srcIp[3]);
  char srcPort[6];
  sprintf(srcPort, "%i", (int) ((tuple.srcPort[0] << 8) | tuple.srcPort[1]));
  char dstPort[6];
  sprintf(dstPort, "%i", (int) ((tuple.dstPort[0] << 8) | tuple.dstPort[1]));
  char protocol[6];
  sprintf(protocol, "%i", (int) tuple.protocol);
  std::cout << srcIp << ":" << srcPort << "|" << destIp << ":" << dstPort << "|" << protocol << std::endl;
}
using std::vector;
typedef vector<FIVE_TUPLE> TRACE;
#endif 
