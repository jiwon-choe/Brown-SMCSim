#!/usr/bin/python
# Generate Heat Map from DRAM accesses
import sys
import os

base = sys.argv[1]
step = int(sys.argv[2])   # Number of cycles to jump
VAULTS = int(os.environ['N_INIT_PORT'])
BANKS = int(os.environ['DRAM_BANKS_PER_VAULT'])
maxtime = 15000000 # This is the hard limit

print ("Dir:" + base)

banks = []

for i in xrange(BANKS*VAULTS):
    timelist = []
    banks.append(timelist)

for v in xrange(VAULTS):
    infile = open( base + "/ch_{0}.txt".format(v));
    print(v)
    for line in infile:
        tokens = line.split()
        b = int(tokens[1])  # Bank
        t = int(tokens[0])  # Time
        while (len(banks[v*BANKS + b]) <= t ):
            banks[v*BANKS + b].append(0)
        banks[v*BANKS + b][t] = 1

of = open( base + "/heat_map.txt", "w");

# Find maximum l
# extend all values by this maximum value
maxl = -1;
for bank in banks:
    if ( maxl == -1 or maxl < len(bank) ):
        maxl = len(bank)

if (maxl > maxtime ):
    print("Truncating data")
    maxl = maxtime

for bank in banks:
    while (len(bank) < maxl):
        bank.append(0);

b=0;
for bank in banks:
    print(b)
    index=0
    s=0
    for point in bank:
        s+=point
        if ( index % step == 0 ):
            of.write( str(s) + " ");
            s = 0;
        index+=1
        if ( index >= maxtime):
            break;
    of.write("\n")
    b+=1

of.close()

