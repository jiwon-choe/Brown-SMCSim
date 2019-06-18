#!/usr/bin/python
# Convert the statistics.txt result of the simulation to CSV format
import sys
##################
BLUE = '\033[94m'
GREEN = '\033[92m'
YELLOW = '\033[93m'
NOCOLOR = '\033[0m'
##################
#print BLUE + "Reading the stats file ..." + NOCOLOR
infile = open( sys.argv[1], "r" )
outfile = open( sys.argv[2], "w" )

#########################################
def write_to_output ( titles, stats, outfile ):
    ind=0
    first = 1
    for s in stats :
        T = titles[ind]
        T = T[T.rfind("scenario")+len("scenario")+1:]
        L = T + " $$$ %%%";
        L = L.replace("# [STAT]", "" );
        L = L.strip();
        L = L.replace("\t", "," );
        L = L.replace("//", "," );
        L = L.replace("/", "," );
        L = L.replace("-", "," );
        L = L.replace(" ", "," );
        L = L.replace("#", "," );

        # Find the title of each part
        tokens=L.split(",");
        for token in tokens:
            if ( token.find(".stat") >= 0 ):
                topic = token.replace(".stat", "")
                break
        L = L.replace(topic, "")
        L = L.replace(".stat", "")
        
        if ( first ):
            first = 0
            outfile.write("###########," + topic+ '\n');


        L = L.replace("%%%", stats[ind])
        L = L.replace("# [STAT] ", "" );
        L = L.replace(topic, "")
        L = L.replace(" ", ",")
        L = L.replace("\t", ",")

        # Replace ,, with ,
        Lrep = L.replace(",,", ",");
        while ( Lrep != L ):
            L = Lrep
            Lrep = L.replace(",,", ",");


        outfile.write(L + '\n');
        ind+=1
    outfile.write('\n');

#########################################
stats = []
titles = []
prev = "TITLE"
for line in infile:
    line = line.replace('\n', '')
    if len(line) >= 1 and line != "":
        if ( line.find( ".stat") >= 0 ):
            if ( prev == "STAT" ):
                # Report previous result
                write_to_output(titles, stats, outfile)
                stats = []
                titles = []
            titles.append(line);
            prev = "TITLE"
        else:
            stats.append(line);
            prev = "STAT"
            
if ( len(stats) > 0 ):
    write_to_output(titles, stats, outfile)

infile.close()
outfile.close();
