# Brown-SMCSim

Brown-SMCSim an extended version of [SMCSim](https://iis-git.ee.ethz.ch/erfan.azarkhish/SMCSim) (originally by Erfan Azarkhish). 
It is being actively used for near-data-processing research by PhD student [Jiwon Choe](https://jiwon-choe.github.io/) and her collaborators at Brown University. 

**The scrypt branch contains the simulator and code used for evaluation in the MEMSYS '19 extended abstract
[Attacking Memory-Hard scrypt with Near-Data-Processing](memsys19-choe.pdf).** Please refer to the "Setup" instructions and "Directions for running scrypt" below. 


The master branch contains the simulator that was used for evaluation in the SPAA '19 paper 
[Concurrent Data Structures with Near-Data-Processing: an Architecture-Aware Implementation](spaa19-choe.pdf). 
Please cite the above paper when using this simulator. 

This repository may be merged as a branch to SMCSim in the future.



## Major Modifications from SMCSim

The original SMCSim had a single PIM (NDP) device across all memory vaults, 
and the PIM device and the host processors had shared access to memory 
(parts of memory that were designated as shared regions of memory by the host processor; 
these regions were marked as uncacheable and the page table for these "PIM regions" were offloaded to the PIM device).

These are the major modifications made to SMCSim:

- The memory vaults are divided as host-accessible main memory vaults and NDP vaults (8 each). 

- Each NDP vault has its own PIM (NDP) device, connected directly to the memory controller for each NDP vault. 
The NDP device (also referred to as NDP core) has exclusive access to data in its coupled vault.

- Different memory configurations can be used for the host main memory vaults and NDP vaults 
(by changing the variable `GEM5_PIM_MEMTYPE`). 
`BufferHMCVault` implements a small data buffer in the NDP vault's memory controller, as described in the SPAA '19 paper.


## Setup

- Checkout this repository.

- Download [SMC-WORK.tgz](https://iis-git.ee.ethz.ch/erfan.azarkhish/SMCSim/blob/master/SMC-WORK.tgz) from the original SMCSim 
repository and untar it in the base directory of this repository (the same directory that this README file is located in).
  
- Copy the 8cpu dtb file to SMC-WORK:
  - `cp binaries-SMC-WORK/vexpress.aarch32.ll_20131205.0-gem5.8cpu.dtb SMC-WORK/gem5-images/aarch-system-2014-10/binaries`

- For the remaining setup, you may follow the setup procedure described in the original SMCSim 
([section 3: Build and First Run](https://iis-git.ee.ethz.ch/erfan.azarkhish/SMCSim#3-build-and-first-run)). 
  - **NOTE:** When running the demo scenario, instead of using `scenarios/0-demo/1-singlepim-pagerank.sh`, 
use `scenarios/6-scrypt/1-host-scrypt.sh` or `scenarios/6-scrypt/2-pim-scrypt.sh`.


## Directions for running scrypt
