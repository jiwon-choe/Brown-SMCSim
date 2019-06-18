from m5.SimObject import *
from m5.params import *
from m5.proxy import *
from Device import DmaDevice


##############################################################################
class ethz_DMA(DmaDevice):
    type = 'ethz_DMA'
    cxx_header = "dev/ethz_dma.hh"
    clk_per_ps = Param.Unsigned(1000, "Clock period of the DMA state machine (ps)")
    PIM_DMA_STATUS_ADDR = Param.Int(0, "DMA Status Register(Absolute Address)");
