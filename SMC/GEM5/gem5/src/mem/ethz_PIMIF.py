from m5.params import *
from m5.proxy import *
from MemObject import MemObject

# This is an interface module to attach the PIM to the memory interconnect

class ethz_PIMIF(MemObject):
	type = 'ethz_PIMIF'
	cxx_header = "mem/ethz_pim_if.hh"
	slave = SlavePort('Slave port')
	master = MasterPort('Master port')
	system = Param.System(Parent.any, "System we belong to")	# A pointer to the system is necessary for each master components
    
	# Maybe I will delete these later
	# Maybe I will delete these later
	# Maybe I will delete these later
	req_size = Param.Unsigned(16, "The number of requests to buffer")
	resp_size = Param.Unsigned(16, "The number of responses to buffer")
	delay = Param.Latency('0ns', "The latency")
	ranges = VectorParam.AddrRange([AllMemory],
					"Address ranges to pass through")
