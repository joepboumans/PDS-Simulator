import argparse
import subprocess
import os
from sys import byteorder
from scapy.all import *


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
                for packet in PcapReader("equinix-chicago.20160121-130000.UTC.pcap"):
                    packet.show()
                    print (f"{packet.src, packet.dst, packet.sport, packet.dport, packet.proto}")
                    out = [int(n).to_bytes(1, byteorder='big') for n in packet.src.split(".")]
                    out += [int(n).to_bytes(1, byteorder='big') for n in packet.dst.split(".")]
                    out += [packet.sport.to_bytes(2, byteorder='big')]
                    out += [packet.dport.to_bytes(2, byteorder='big')]
                    out += [packet.proto.to_bytes(1, byteorder='big')]
                    for o in out:
                        fout.write(o)
            print("Finished parsing %s"% file_name)
