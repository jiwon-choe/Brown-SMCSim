import json
import sys

# Convert json to txt

def print_recursive(prefix, d):
    if isinstance(d, dict):
        if ( "path" in d.keys() ):
            prefix = d["path"]

        for k in d.keys():
            if ( prefix == "" ):
                print_recursive(str(k), d[k])
            else:
                print_recursive(prefix + "." + str(k), d[k])

    elif isinstance(d, list):
        for i in d:
            print_recursive(prefix, i)
    else:
        print(prefix + " " + str(d))

if len(sys.argv) > 1:
    s = sys.argv[1]
    with open(s) as json_data:
        d = json.load(json_data)
        print_recursive("", d)
