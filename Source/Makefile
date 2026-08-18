################################################################################
# roOs Makefile
#
# Created: 29/03/2023
#
# Author: Alexy Torres Aurora Dugo
#
# Main makefile. This makefile will proceed to compile all required modules
# and configurations.
# Once compiled it will produce the final binary file for the desired
# architecture
################################################################################

CPU_ARCH_LIST= x86_64
TARGET_LIST= x86_64
KERNEL_SRC_DIR= ./Kernel
USER_SRC_DIR = ./User
CONFIG_DIR= ./Config

KERNEL_ARTIFACTS_DIR = Kernel/ARTIFACTS
KERNEL_BUILD_DIR = build/kernel
USER_BUILD_DIR = build/user

KERNEL = roOs

.PHONY: all
all: init build_kernel

init:
ifeq ($(target), x86_64)
	$(CONFIG_DIR)/arch/x86_64/init_build.sh
else
	@echo "\e[1m\e[31m\n=== ERROR: Unknown target $(target)\e[22m\e[39m"
	@echo "Available targets: $(TARGET_LIST)\n"
	@exit 1
endif

build_modules:
	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Building kernel modules for target $(target)\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

ifeq ($(target), x86_64)
	@make -C $(KERNEL_SRC_DIR)/arch target_board=x86 target_cpu=x86_64
else
	@echo "\e[1m\e[31m\n=== ERROR: Unknown target $(target)\e[22m\e[39m"
	@echo "Available targets: $(TARGET_LIST)\n"
	@exit 1
endif

	@make -C $(KERNEL_SRC_DIR)/libs
	@make -C $(KERNEL_SRC_DIR)/core
	@make -C $(KERNEL_SRC_DIR)/time
	@make -C $(KERNEL_SRC_DIR)/io

ifeq ($(TESTS), TRUE)
	# Build the tests module
	@make -C $(KERNEL_SRC_DIR)/tests target=$(target)
endif

	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Generated kernel modules for target $(target)\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

####################################
# Build Kernel and integrated initrd
####################################

build_kernel: build_modules
	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Building kernel for target $(target)\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

	@make -C $(KERNEL_SRC_DIR)/ARTIFACTS KERNEL_NAME=$(KERNEL) BUILD_DIR=$(KERNEL_BUILD_DIR)

	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Generated kernel for target $(target)\e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

####################################
# Build rootfs and user apps
####################################

clean:

# Dummy settings
	@mkdir -p Kernel/ARTIFACTS
	@touch Kernel/ARTIFACTS/settings.mk

# Clean modules
ifeq ($(target), x86_64)
	@make -C $(KERNEL_SRC_DIR)/arch target_board=x86 target_cpu=x86_64 clean
	@make -C $(KERNEL_SRC_DIR)/tests target=$(x86_64) clean
else
	@make -C $(KERNEL_SRC_DIR)/arch target_board=x86 target_cpu=x86_64 clean
	@make -C $(KERNEL_SRC_DIR)/tests target=$(x86_64) clean
endif

	@make -C $(KERNEL_SRC_DIR)/libs clean
	@make -C $(KERNEL_SRC_DIR)/core clean
	@make -C $(KERNEL_SRC_DIR)/time clean
	@make -C $(KERNEL_SRC_DIR)/io clean


	@echo "\e[1m\e[34m\n#-------------------------------------------------------------------------------\e[22m\e[39m"
	@echo "\e[1m\e[34m| Cleaned kernel \e[22m\e[39m"
	@echo "\e[1m\e[34m#-------------------------------------------------------------------------------\n\e[22m\e[39m"

	@rm -rf Kernel/ARTIFACTS

run:
	@make run -f $(CONFIG_DIR)/arch/$(target)/makerun.mk KERNEL_BUILD_DIR=$(KERNEL_ARTIFACTS_DIR)/$(KERNEL_BUILD_DIR) KERNEL=$(KERNEL)

debug:
	@make debug -f $(CONFIG_DIR)/arch/$(target)/makerun.mk KERNEL_BUILD_DIR=$(KERNEL_ARTIFACTS_DIR)/$(KERNEL_BUILD_DIR) KERNEL=$(KERNEL)

qemu-test-mode:
	@make qemu-test-mode -f $(CONFIG_DIR)/arch/$(target)/makerun.mk KERNEL_BUILD_DIR=$(KERNEL_ARTIFACTS_DIR)/$(KERNEL_BUILD_DIR) KERNEL=$(KERNEL)