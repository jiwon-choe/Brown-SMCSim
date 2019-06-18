#!/usr/bin/python
# Convert the CSV format to GNU Plot format
import sys
##################
BLUE = '\033[94m'
GREEN = '\033[92m'
YELLOW = '\033[93m'
NOCOLOR = '\033[0m'
##################
#print BLUE + "Reading the stats file ..." + NOCOLOR
statname = sys.argv[1]
substatname = sys.argv[2]
infile = open( sys.argv[3], "r" )
outfile = open( sys.argv[4], "w" )

assert statname != "";
assert substatname != "";

def is_number(s):
    try:
        float(s)
        return True
    except ValueError:
        return False

#########################################
stats = []
title = ""
index = 0;
for line in infile:
    line = line.replace(',', ' ')
    line = line.replace(')', ' ')
    line = line.replace('(', ' ')
    if ( len(title) > 0 and line.find("###########") >= 0 ):
        break;
    if ( line.find( statname ) >= 0 ):
        title=statname
        continue
    if ( len(line) <= 1 ):
        continue;
    if ( len(title) > 0 ):    # column index is used to identify the Y-Axis
        if ( is_number(substatname) ):
            column_index = int(substatname)
            Lrep = line.replace("  ", " ");
            while ( Lrep != line ):
                line = Lrep
                Lrep = line.replace("  ", " ");
            tokens = line.split(' ');
            
            line = "\""
            seen = False
            count = 0
            i=0;
            for to in tokens:
                i += 1
                if ( to == "$$$" ):
                    seen=True
                    line += "\""
                if ( seen ):
                    if ( count == column_index ):
                        line += " " + tokens[i];
                        break;
                    count += 1
                else:
                    line += to + " "
            line = line.strip();
            line = line.replace("m5out", "");
            outfile.write(str(index) + " " + line + "\n" );
                    
        else:    # sub-stat name is used to identify the Y-Axis
            tokens = line.split(substatname);
            
            line = tokens[0].strip()
            tokens = line.split(' ')
            #line = line.replace(tokens[len(tokens)-1], "")
            tokens2 = line.split("$$$");
            line = tokens2[0];
            line = line.strip();
            outfile.write(str(index) + " \"" + line + "\" " + tokens[len(tokens)-1] + "\n" );

        index += 1
infile.close()
outfile.close();
