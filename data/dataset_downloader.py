import argparse
import subprocess
import os


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Downloads the specific dataset from CAIDA when given a valid login.\nProvide the data set name as passive-YYYY/equinix-XXXX/YYYYMMDD-HHMMSS.UTCC")
    parser.add_argument('-u', '--user', dest="user", help='<user:password> for CAIDA', required=True)
    parser.add_argument('-i', '--input', dest="input",help='Input file with datasets', required=True)

    args = parser.parse_args()
    
    # Read and parse input datasets
    with open(args.input) as f:
        lines = f.readlines()
        for dataset in lines:
            dataset = dataset.strip()
            sp_set = dataset.split("/")
            print(dataset)
            # Download data set
            file_name = f"{sp_set[1]}.{sp_set[2]}.pcap"
            url = f"https://data.caida.org/datasets/{dataset}/{sp_set[1]}.dirA.{sp_set[2]}.anon.pcap.gz"
            download_file = f"{file_name}.gz"
            proc = subprocess.Popen(["curl","-u", args.user, url, "-o", download_file], stdout=subprocess.PIPE)
            outs, errs = proc.communicate()
            if errs:
                print(errs)
                exit(1)


            #gzip dataset
            if (os.path.isfile(download_file)):
                print("PCAP already present, skipping...")
                continue

            proc = subprocess.Popen(["gzip", download_file, "-d"], stdout=subprocess.PIPE)
            outs, errs = proc.communicate()
            print(outs)
            if errs:
                print(errs)
                exit(1)
            
            # Parse into .dat file
            print("Parsing %s into 5-tuple..."% file_name)
            out =subprocess.run(["tshark", "-r", file_name, "-T", "fields", "-e", "ip.src","-e", "ip.dst","-e", "tcp.srcport","-e", "tcp.dstport","-e", "ip.proto"], capture_output=True, text=True)
            file_name.replace(".dirA.", "")
            file_name.replace(".UTC.anon.", "")
            with open(file_name.replace("pcap","dat"), 'wb') as fout:
                lines = out.stdout.split('\n')
                for l in lines:
                    # print(l)
                    l = l.split()
                    if len(l) < 4:
                        continue
                    out = [int(num).to_bytes(1, byteorder='big') for e in l[:2] for num in e.split('.')]
                    out += [int(num).to_bytes(2, byteorder='big') for num in l[2:4]]
                    out += [int(num).to_bytes(1, byteorder='big') for num in l[4]]
                    # print(out)
                    for o in out:
                        fout.write(o)
            print("Finished parsing %s"% file_name)
