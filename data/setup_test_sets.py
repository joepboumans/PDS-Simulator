import time
from collections import defaultdict
import mmap

from scapy.all import *

def get_field_bytes(pkt, name):
    fld, val = pkt.getfield_and_val(name)
    return fld.i2m(pkt, val)

def write2pcap(pkt, test_name):
    wrpcap(f"{test_name}.pcap", pkt, append=True)

def setup_dataset(data_name, test_name, length):
    print(f"[Dataset Loader] Get data from {data_name}")
    with open(f"{test_name}.dat", 'wb') as fout:
        start = time.perf_counter_ns()
        count = 0
        for packet in PcapReader(data_name):
            try:
                if not UDP in packet and not TCP in packet or IPv6 in packet:
                    continue

                out = get_field_bytes(packet, "src")
                out += get_field_bytes(packet, "dst")
                out += packet.sport.to_bytes(2, byteorder='big')
                out += packet.dport.to_bytes(2, byteorder='big')
                out += packet.proto.to_bytes(1, byteorder='big')
                fout.write(out)
                write2pcap(packet, test_name)
                count +=1
                if length <= 64:
                    print(out)
            except:
                packet.show()
                exit(1)
            if count >= length:
                print(f"Created data set with size of {count}")
                break
        stop = time.perf_counter_ns()
        print(f"Parsed data set in {stop - start} ns")
    print("Finished parsing %s"% data_name)

def setup_dataset_bursty(data_name, test_name, length, burst_length):
    print(f"[Dataset Loader] Get data from {data_name}")
    with open(f"{test_name}.dat", 'wb') as fout:
        start = time.perf_counter_ns()
        count = 0
        for packet in PcapReader(data_name):
            try:
                if not UDP in packet and not TCP in packet or IPv6 in packet:
                    continue

                out = get_field_bytes(packet, "src")
                out += get_field_bytes(packet, "dst")
                out += packet.sport.to_bytes(2, byteorder='big')
                out += packet.dport.to_bytes(2, byteorder='big')
                out += packet.proto.to_bytes(1, byteorder='big')
                for _ in range(burst_length):
                    fout.write(out)
                    write2pcap(packet, test_name)
                count +=1
                if length <= 64:
                    print(out)
            except:
                packet.show()
                exit(1)
            if count >= length:
                print(f"Created data set with size of {count}")
                break
        stop = time.perf_counter_ns()
        print(f"Parsed data set in {stop - start} ns")
    print("Finished parsing %s"% data_name)


def main():
   setup_dataset("equinix-chicago.20160121-130000.UTC.pcap", "small_test", 1024)
   setup_dataset("equinix-chicago.20160121-130000.UTC.pcap", "smaller_test", 32)
   setup_dataset("equinix-chicago.20160121-130000.UTC.pcap", "single_test", 1)
   setup_dataset_bursty("equinix-chicago.20160121-130000.UTC.pcap", "single_burst_test", 1, 32)
   setup_dataset_bursty("equinix-chicago.20160121-130000.UTC.pcap", "small_burst_test", 32, 32)

if __name__ == "__main__":
    main()
