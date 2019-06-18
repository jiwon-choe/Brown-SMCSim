
# Do not run this script in standalone mode
# Use build.sh to run it
# Parameters will be added up here automatically 
################################################

MODULES := pim.o

# guest architecture
obj-m := $(MODULES)

# path of the arm compiled kernel
ROOTDIR := $(COMPILED_KERNEL_PATH)

MAKEARCH := $(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(HOST_CROSS_COMPILE)

all: modules
modules:
# 	@echo "Cross-compiler: $(HOST_CROSS_COMPILE) Guest Architecture: $(ARCH)"
	$(MAKEARCH) -C $(ROOTDIR) M=${shell pwd} modules -Wall
	
clean:
	$(MAKEARCH) -C $(ROOTDIR) M=${shell pwd} clean
