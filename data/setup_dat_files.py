
with open('test.txt', 'r') as fin, open('test.out', 'wb') as fout:
    lines = fin.readlines()
    for l in lines:
        print(l)
        l = l.split()
        if len(l) < 4:
            continue
        out = [int(num).to_bytes(1, byteorder='big') for e in l[:2] for num in e.split('.')]
        out += [int(num).to_bytes(2, byteorder='big') for num in l[2:4]]
        out += [int(num).to_bytes(1, byteorder='big') for num in l[4]]
        print(out)
        for o in out:
            fout.write(o)
