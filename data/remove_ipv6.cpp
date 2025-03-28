#include <chrono>
#include <cstring>
#include <glob.h>
#include <iostream>
#include <ostream>
#include <pcap/pcap.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

using std::string;
using std::vector;

int main(int argc, char *argv[]) {
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
    string outName = "ipv4-" + f;

    char errbuf[PCAP_ERRBUF_SIZE];
    // Open the input pcap file.
    pcap_t *in_handle = pcap_open_offline(f.data(), errbuf);
    if (!in_handle) {
      fprintf(stderr, "Error opening input file: %s\n", errbuf);
      return 1;
    }

    // Get the datalink type from the input file. For raw IP packets this might
    // be DLT_RAW.
    int dlt = pcap_datalink(in_handle);
    // Open a dead pcap handle with the same datalink type.
    pcap_t *dead_handle = pcap_open_dead(dlt, 65535);
    pcap_dumper_t *dumper = pcap_dump_open(dead_handle, outName.data());
    if (!dumper) {
      fprintf(stderr, "Error opening output file: %s\n",
              pcap_geterr(dead_handle));
      pcap_close(in_handle);
      return 1;
    }

    struct pcap_pkthdr *header;
    const u_char *data;
    int res;

    int packetCount = 0;
    auto start = std::chrono::high_resolution_clock::now();
    while ((res = pcap_next_ex(in_handle, &header, &data)) >= 0) {
      if (res == 0) { // Timeout for live captures; ignore.
        continue;
      }
      // Make sure there's at least one byte to inspect.
      if (header->caplen < 1)
        continue;

      // Check the IP version by examining the high nibble of the first byte.
      u_char version = data[0] >> 4;
      if (version == 6) {
        // Skip IPv6 packets.
        continue;
      }
      // Otherwise, it's IPv4 (or something unexpected) – write it to the output
      // file.
      pcap_dump((u_char *)dumper, header, data);

      if (packetCount % 10000 == 0) {
        std::cout << packetCount << '\r' << std::flush;
      }
      packetCount++;
    }

    // Clean up.
    pcap_dump_close(dumper);
    pcap_close(dead_handle);
    pcap_close(in_handle);
    auto stop = std::chrono::high_resolution_clock::now();
    auto time =
        std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
    std::cout << f << " parsed in " << time.count() << " with " << packetCount
              << " packets into " << outName << std::endl;
  }

  return 0;
}
