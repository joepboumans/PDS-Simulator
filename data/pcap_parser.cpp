#include <arpa/inet.h> // For network-related functions
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <glob.h>
#include <iostream>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <ostream>
#include <pcap.h>
#include <vector>

using std::string;
using std::vector;

std::ofstream fout;
size_t packet_count = 0;

// Callback function to process each packet
void packetHandler(u_char *userData, const struct pcap_pkthdr *pkthdr,
                   const u_char *packet) {
  // For a raw IP file, the first nibble of the first byte indicates the IP
  // version.
  u_char ipVersion = packet[0] >> 4;
  if (ipVersion == 6) {
    return;
  }

  const struct ip *ipHeader = (struct ip *)(packet);
  int protocol = ipHeader->ip_p;

  char srcIP[INET_ADDRSTRLEN];
  char dstIP[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(ipHeader->ip_src), srcIP, INET_ADDRSTRLEN);
  inet_ntop(AF_INET, &(ipHeader->ip_dst), dstIP, INET_ADDRSTRLEN);

  /*std::cout << "IPv4 Packet ";*/
  /*std::cout << "Source IP: " << srcIP << " ";*/
  /*std::cout << "Destination IP: " << dstIP << " ";*/
  /*std::cout << "Protocol: "*/
  /*          << (protocol == IPPROTO_TCP*/
  /*                  ? "TCP"*/
  /*                  : (protocol == IPPROTO_UDP ? "UDP" : "Other"))*/
  /*          << std::endl;*/
  /*std::cout << "Packet count: " << packetCount[0] << std::endl;*/

  char tuple[13];
  memcpy(&tuple[0], &ipHeader->ip_src, sizeof(ipHeader->ip_src));
  memcpy(&tuple[4], &ipHeader->ip_dst, sizeof(ipHeader->ip_dst));
  tuple[12] = protocol;

  int ipHeaderLength = ipHeader->ip_hl * 4; // ip_hl is in 4-byte words
  const u_char *transportHeader =
      packet + sizeof(struct ether_header) + ipHeaderLength;
  // Extract source and destination ports based on the protocol
  if (protocol == IPPROTO_TCP) {
    const struct tcphdr *tcpHeader = (struct tcphdr *)transportHeader;
    memcpy(&tuple[8], &tcpHeader->th_sport, sizeof(tcpHeader->th_sport));
    memcpy(&tuple[10], &tcpHeader->th_dport, sizeof(tcpHeader->th_dport));
  } else if (protocol == IPPROTO_UDP) {
    const struct udphdr *udpHeader = (struct udphdr *)transportHeader;
    memcpy(&tuple[8], &udpHeader->uh_sport, sizeof(udpHeader->uh_sport));
    memcpy(&tuple[10], &udpHeader->uh_dport, sizeof(udpHeader->uh_dport));
  } else {
    return;
  }
  int *packetCount = (int *)userData;
  if (*packetCount % 10000 == 0) {
    std::cout << *packetCount << '\r' << std::flush;
  }
  (*packetCount)++;

  fout.write(tuple, 13);
}

int main() {
  // Get data files
  glob_t *glob_res = new glob_t;
  if (memset(glob_res, 0, sizeof(glob_t)) == NULL) {
    std::cout << "Glob init failed" << std::endl;
    exit(1);
  }
  glob("*.pcap", GLOB_TILDE, NULL, glob_res);
  vector<string> filenames(glob_res->gl_pathc);
  for (size_t i = 0; i < glob_res->gl_pathc; ++i) {
    filenames[i] = string(glob_res->gl_pathv[i]);
  }
  globfree(glob_res);

  for (string &f : filenames) {
    std::cout << "Opening file " << f << std::endl;
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t *pcapFile = pcap_open_offline(f.data(), errbuf);

    if (pcapFile == nullptr) {
      std::cerr << "Error opening file: " << errbuf << std::endl;
      return 1;
    }

    // Setup .dat file
    string file = f.erase(f.find(".pcap"), sizeof(".pcap") - 1);
    char fname[300];
    sprintf(fname, "%s.dat", file.data());
    fout.open(fname, std::ios::out | std::ios::binary);

    int packetCount = 0;
    // Process packets
    auto start = std::chrono::high_resolution_clock::now();
    if (pcap_loop(pcapFile, 0, packetHandler, (u_char *)&packetCount) < 0) {
      std::cerr << "Error reading packets: " << pcap_geterr(pcapFile)
                << std::endl;
    }
    std::cout << std::endl;
    auto stop = std::chrono::high_resolution_clock::now();
    auto time =
        std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << f << " parsed in " << time.count() << " with " << packetCount
              << " packets" << std::endl;

    // Close the PCAP file
    pcap_close(pcapFile);
    fout.close();
  }

  return 0;
}
