#!/usr/bin/python
# Get the binary ELF file as input, and then dump a *.hex file starting from the START_ADDRESS up to SIZE bytes
# NOTICE: output file will be appended, and previous dumps will be kept in it
# 1 = binary file name
# 2 = dest.hex
# 3 = off in input binary file
# 4 = addr in memory
# 5 = size
import sys
##################
BLUE = '\033[94m'
GREEN = '\033[92m'
YELLOW = '\033[93m'
NOCOLOR = '\033[0m'
##################

import struct

src = sys.argv[1]
dst = sys.argv[2]
off = sys.argv[3]
addr = sys.argv[4]
size = sys.argv[5]

src_file = open( src, "rb" )
A = long(addr, 16);
S = long(size, 16);
O = long(off, 16);

print "  File: " + src + "  Offset: " + off + "  Addr: " + addr + "  Size: " + str(S) + " (Bytes)"

index = 0;
count = 0;
CODE = "";
try:
	b = src_file.read(1)
	while b != "":
		if ( index >= O ):
			CODE += b.encode("hex") + " "
			count += 1
		if ( index >= O+S-1 ):
			break;
		b = src_file.read(1)
		index += 1;
finally:
	src_file.close();
	
##print CODE
#if ( count % 2 != 0 ):
	#print "Error: the length of the binary code must be even! " + str(count)
	#exit(1)
	
if ( count != S ):
	print "Number of bytes read from the file does not match the ELF header! READ:" + str(count) + " EXPECTED:" + str(S)
	exit(1)

i=0;
dst_file = open( dst, "a" )
dst_file.write("@ADDR " + str(A) + "\n" )
dst_file.write("@SIZE " + str(S) + "\n" )
dst_file.write(CODE)
dst_file.write("@END" + "\n" )

src_file.close()
dst_file.close()
