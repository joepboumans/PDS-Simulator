import argparse
import subprocess
import os
import time
from sys import byteorder
from scapy.all import *

def get_field_bytes(pkt, name):
    fld, val = pkt.getfield_and_val(name)
    return fld.i2m(pkt, val)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Downloads the specific dataset from CAIDA when given a valid login.\nProvide the data set name as passive-YYYY/equinix-XXXX/YYYYMMDD-HHMMSS.UTC")
    parser.add_argument('-u', '--user', dest="user", help='<user:password> for CAIDA', required=True)
    parser.add_argument('-i', '--input', dest="input", help='Input file with datasets', required=True)
    parser.add_argument('-d', '--download', dest="download", help='Only download the pcap files', action='store_true')
    args = parser.parse_args()
    
    # Read and parse input datasets
    with open(args.input) as f:
        lines = f.readlines()
        for dataset in lines:
            dataset = dataset.strip()
            sp_set = dataset.split("/")
            date = sp_set[2].split('-')[0]
            print(date)
            print(dataset)
            # Download data set
            file_name = f"{sp_set[1]}.{sp_set[2]}.pcap"
            url = f"https://data.caida.org/datasets/{sp_set[0]}/{sp_set[1]}/{date}-130000.UTC/{sp_set[1]}.dirA.{sp_set[2]}.anon.pcap.gz"
            download_file = f"{file_name}.gz"

            if (os.path.isfile(download_file) or os.path.isfile(file_name)):
                print(f"File was downloaded: {download_file}")
            else:
                print(f"Downloading {url}")
                proc = subprocess.Popen(["curl","-u", args.user, url, "-o", download_file], stdout=subprocess.PIPE)
                outs, errs = proc.communicate()
                if errs:
                    print(errs)
                    exit(1)


            #gzip dataset
            if (not os.path.isfile(file_name)):
                proc = subprocess.Popen(["gzip", download_file, "-d"], stdout=subprocess.PIPE)
                outs, errs = proc.communicate()
                print(outs)
                if errs:
                    print(errs)
                    exit(1)
            else:
                print("PCAP already present, skipping gzip...")

            if args.download:
                print("Only downloading files")
                continue
            
            # Parse into .dat file
            print("Parsing %s into 5-tuple..."% file_name)
            file_name.replace(".dirA", "")
            file_name.replace(".UTC", "")
            file_name.replace(".anon", "")
            with open(file_name.replace("pcap","dat"), 'wb') as fout:
                start = time.perf_counter_ns()
                for packet in PcapReader("equinix-chicago.20160121-130000.UTC.pcap"):
                    try:
                        if not UDP in packet and not TCP in packet or IPv6 in packet:
                            continue

                        out = get_field_bytes(packet, "src")
                        out += get_field_bytes(packet, "dst")
                        out += packet.sport.to_bytes(2, byteorder='big')
                        out += packet.dport.to_bytes(2, byteorder='big')
                        out += packet.proto.to_bytes(1, byteorder='big')
                        fout.write(out)
                    except:
                        packet.show()
                        exit(1)
                stop = time.perf_counter_ns()
                print(f"Parsed data set in {stop - start} ns")
            print("Finished parsing %s"% file_name)
