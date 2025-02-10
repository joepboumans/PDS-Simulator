#include <arpa/inet.h> // For network-related functions
#include <iostream>
#include <netinet/ip.h> // For network-related functions
#include <pcap.h>

// Callback function to process each packet
void packetHandler(u_char *userData, const struct pcap_pkthdr *pkthdr,
                   const u_char *packet) {
  std::cout << "Packet length: " << pkthdr->len << " bytes" << std::endl;
  std::cout << "Captured length: " << pkthdr->caplen << " bytes" << std::endl;

  // Example: Print the source and destination IP addresses (for IPv4 packets)
  const struct ip *ipHeader =
      (struct ip *)(packet + 14); // Skip Ethernet header (14 bytes)
  char srcIP[INET_ADDRSTRLEN];
  char dstIP[INET_ADDRSTRLEN];

  inet_ntop(AF_INET, &(ipHeader->ip_src), srcIP, INET_ADDRSTRLEN);
  inet_ntop(AF_INET, &(ipHeader->ip_dst), dstIP, INET_ADDRSTRLEN);

  std::cout << "Source IP: " << srcIP << std::endl;
  std::cout << "Destination IP: " << dstIP << std::endl;

  std::cout << "-------------------------" << std::endl;
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <pcap_file>" << std::endl;
    return 1;
  }

  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t *pcapFile = pcap_open_offline(argv[1], errbuf);

  if (pcapFile == nullptr) {
    std::cerr << "Error opening file: " << errbuf << std::endl;
    return 1;
  }

  // Process packets
  if (pcap_loop(pcapFile, 0, packetHandler, nullptr) < 0) {
    std::cerr << "Error reading packets: " << pcap_geterr(pcapFile)
              << std::endl;
  }

  // Close the PCAP file
  pcap_close(pcapFile);

  return 0;
}
