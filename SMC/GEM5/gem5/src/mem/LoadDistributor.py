from m5.params import *
from XBar import *

# Written by Erfan
# This class was previously called: ethz_SMCController

class LoadDistributor(NoncoherentXBar):
	type = 'LoadDistributor'
	cxx_header = "mem/load_distributor.hh"
	port = SlavePort("Slave ports")
	
