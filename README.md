# Brown-SMCSim

Brown-SMCSim an extended version of [SMCSim](https://iis-git.ee.ethz.ch/erfan.azarkhish/SMCSim) (originally by Erfan Azarkhish). 
It is being actively used for near-data-processing research by PhD student [Jiwon Choe](https://jiwon-choe.github.io/) and her collaborators at Brown University. 

The master branch contains the simulator that was used for evaluation in the SPAA '19 paper 
**[Concurrent Data Structures with Near-Data-Processing: an Architecture-Aware Implementation](spaa19-choe.pdf)**. 
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

#### Environment Setup

SMCSim runs best on Ubuntu 14.04. Prior attempts to use it on more recent versions like Ubuntu 16.04 have been unsuccessful. :(

If you would still like to try setting up on newer versions of Ubuntu, please do so! 
And please let me know how it goes -- even if it's just a copy-paste of all the error messages that you get. 
I can't promise that I'll fix the errors, but I would still like to stay informed on why it keeps on failing. 

* If you need to download Ubuntu 14.04: http://releases.ubuntu.com/trusty/
* If you are setting up a VM for your SMCSim environment, make sure that you give the VM **at least 50GB of hard drive space** (dynamically allocated is fine), 
and **at least 4MB of base memory**. 

#### Simulator Setup

* Create a base directory to store all simulator-related files. This can be named whatever you want it to be.

* Clone this repository into the base directory. After the repository clone, the path to "smc.sh" should be `<base directory>/SMC/smc.sh` .

* Download [SMC-WORK.tgz](https://iis-git.ee.ethz.ch/EXT_erfan.azarkhish/SMCSim/-/blob/master/SMC-WORK.tgz) from the original SMCSim directory, and extract it in the base directory. Your directory structure should look like the following: 
```
$ ls <base directory>/SMC-WORK/
busy  checkpoints  gem5-build  gem5-images  linux_kernel  scenarios  traces
```

* Copy the 8cpu dtb file to SMC-WORK:

  `cp SMC/binaries/vexpress.aarch32.ll_20131205.0-gem5.8cpu.dtb SMC-WORK/gem5-images/aarch-system-2014-10/binaries`
 


* Set the working directory inside the simulation environment. From `<base directory>`, run: 

  `echo "<absolute path to base directory>/SMC-WORK" > SMC/UTILS/variables/SMC_WORK_DIR.txt`

  (`SMC/UTILS/variables` directory does not exist in the repository, but it will be created once you run this command.)

* Install all the tool dependencies. This can be done easily by running `SMC/amy/install_needed_libs.bash`.


## Building and Running Simulations

SMCSim is a gem5 full-system simulator, meaning that the gem5 architecture framework simulates a full fledged system and a Linux is loaded on top of the simulated system. A simulation using SMCSim first builds and runs this gem5 full-system, and then it loads and runs a software program on top of that. 

So simply put, there are two parts to the simulator: the gem5 code that takes care of all the simulated architecture (what the hardware components are, what kind of details the components have, how components are put together, etc.), and the actual software code that runs on top of that architecture.


The entire simulation is built and executed by running a **scenario script**, which sets up all the variables needed for the simulation.
In order to explain how to run the simulation, I will use the scenario script used for running experiments for the [SPAA'19 paper](https://jiwon-choe.github.io/spaa19-choe.pdf). This script is: `SMC/scenarios/4-pim-change/1-ptrchasing-list.sh`. The scenario script must be executed from the `SMC` directory.


#### Building the Simulator

This part builds the gem5 portion of the simulation.


```
$ sudo ./scenarios/4-pim-change/1-ptrchasing-list.sh -b
```
The -b option with the scenario script builds the simulation. 
Upon the first build, disk images required for full-system simulation will be prepared.
You should see the following message:
```
Info: Disk images were prepared successfully! ...
Info: ((( Please rerun your script now )))
```

Now run the build again:
```
$ sudo ./scenarios/4-pim-change/1-ptrchasing-list.sh -b
```

gem5 will now start to build itself and this procedure may take a long time.
Make sure that the build process has been successful:
```
scons: done building targets.
ARM/gem5.opt was built successfully
```

Again, the "build" compiles and builds the gem5 infrastructure for the simulation. 
Since the gem5 infrastructure is shared across different scenarios, really any scenario script can be used for the build, Also, it is not necessary to re-run the build across different executions (even across different scenario scripts), unless you changed something in the gem5 code.

#### Running the Simulation

The -o option with the scenario script runs the simulation.

```
$ sudo ./scenarios/4-pim-change/1-ptrchasing-list.sh -o
```

The first time that the scenario script is executed with the -o option, the Linux on the simulated system boots up, and once the system is up and running a checkpoint is taken, then the simulation exits. (The intended software program is not run yet).

Running the same command the second time will load the existing checkpoint (i.e., the simulation will start from after bootup) and execute the intended software program on that simulated system.

The intended software program is located in the directory specified by `HOST_APP_DIR` in the scenario script file. `HOST_APP_DIR` is located in `SW/HOST/app/`. 

The software program uses an API to communicate with the PIM core(s). The API code is located in `SW/HOST/api/`. There is also driver code for the PIM hardware, but this can be disregarded for now. (Note: PIM and NDP are used interchangeably in this repository. In fact, PIM is used more frequently.)

The software program uses the PIM API to offload a kernel to the PIM core(s). This offloaded kernel is the software that runs on the PIM core(s). This kernel is specified by `LIST_KERNEL_NAME` in the scenario script file, and the code for this kernel is located in `SW/PIM/kernel/`.

All the code related to the software that runs on top of the simulated system are recompiled every time the scenario script is executed with the -o option.

When you execute the scenario script with the -o option, something like this will be printed in the terminal:
```
Listening for system connection on port 3456
```
Once this appears, by running `telnet localhost 3456` in a separate terminal, you can see what gets printed in the system terminal of the simulated system.


#### After changing gem5 code or configurations

After changing gem5 code or configurations, you must delete the existing checkpoint and start from the bootup phase again. (If you changed gem5 code, you must start from the build phase.)
In order to remove an existing checkpoint, you can run the scenario script with the -d option:
```
$ ./scenarios/4-pim-change/1-ptrchasing-list.sh -d
```
