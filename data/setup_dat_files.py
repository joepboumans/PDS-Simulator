import glob
import subprocess

# Transform .pcap files into text files
for file in glob.glob("*.pcap"):
    print("Parsing %s into 5-tuple..."% file)
    out =subprocess.run(["tshark", "-r", file, "-T", "fields", "-e", "ip.src","-e", "ip.dst","-e", "tcp.srcport","-e", "tcp.dstport","-e", "ip.proto"], capture_output=True, text=True)
    with open(file.replace("pcap","dat"), 'wb') as fout:
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
    print("Finished parsing %s"% file)
