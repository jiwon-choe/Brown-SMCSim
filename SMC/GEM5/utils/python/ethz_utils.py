#TODO ERFAN

import sys
import _gem5_params
from _gem5_params import *
####################################
class colors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    WARNING = '\033[1;33m'
    LIGHTGREEN = '\033[92m'
    BLUE = '\033[94m'
    FAIL = '\033[91m'
    RED = '\033[91m'
    ENDC = '\033[0m'
    YELLOW = '\033[1;33m'

    def disable(self):
        self.HEADER = ''
        self.OKBLUE = ''
        self.LIGHTGREEN = ''
        self.WARNING = ''
        self.YELLOW = ''
        self.BLUE = ''
        self.FAIL = ''
        self.ENDC = ''
        self.RED = ''
####################################
def ethz_print_val( msg, value ):
	print colors.LIGHTGREEN + 'Info: ' + colors.BLUE + msg + colors.LIGHTGREEN +': '+ str(value) + colors.ENDC
	return;

####################################
def ethz_print_msg( msg ):
	print colors.LIGHTGREEN + 'Info: ' + colors.BLUE + msg + colors.LIGHTGREEN + colors.ENDC
	return;

####################################
def ethz_print_err( msg ):
	print colors.LIGHTGREEN + 'Error: ' + colors.FAIL + msg + colors.ENDC
	exit(1);

####################################
def ethz_pretty_print(d, indent=0):
   print "value" + '\t' * (indent+4) + "key"
   print "-----------------------------------"
   for key, value in d.iteritems():
      if isinstance(value, dict):
         ethz_pretty_print(value, indent+1)
      else:
         print str(value) + '\t' * (indent+4) + str(key)
   print "-----------------------------------"

####################################
def ethz_pretty_print_to_file(d, fo ):
   indent = 0;
   fo.write( "value" + '\t' * (indent+4) + "key\n")
   fo.write("-----------------------------------\n")
   for key, value in d.iteritems():
      if isinstance(value, dict):
         ethz_pretty_print(value, indent+1)
      else:
         fo.write( str(value) + '\t' * (indent+4) + str(key) + "\n" )
   fo.write("-----------------------------------\n")
