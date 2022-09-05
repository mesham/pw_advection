.RECIPEPREFIX +=
#######################################################################################
.PHONY: help
help:
  @echo "Makefile Usage:"
  @echo "  make build TARGET=<sw_emu/hw_emu/hw>"
  @echo "      Command to generate the design for specified Target and Device."
  @echo ""
  @echo "  make host "
  @echo "      Command to generate host."
  @echo ""
  @echo "  make device "
  @echo "      Command to generate hardware platform files(xo,xclbin)."
  @echo ""
  @echo "  make clean "
  @echo "      Command to remove the generated files."
  @echo ""
#######################################################################################
TARGET := sw_emu
PLATFORM := xilinx_vck5000_gen4x8_xdma_2_202210_1
HOST_EXE := host
XO := pw_advection_kernel.$(TARGET).xo
XCLBIN := pw_advection_kernel.$(TARGET).xclbin

KERNEL_NAME := pw_advection_kernel

DEVICE_SRCDIR:=kernel
TEMP_DIR := ./_x.$(TARGET)

SOURCES  := $(wildcard $(DEVICE_SRCDIR)/*.cpp)
INCLUDES := $(wildcard $(DEVICE_SRCDIR)/*.h)
OBJECTS  := $(SOURCES:$(DEVICE_SRCDIR)/%.cpp=%.xo)

# Host building global settings
CXXFLAGS := -I$(XILINX_XRT)/include/ -I$(XILINX_VIVADO)/include/ -Wall -O3 -std=c++11 -L$(XILINX_XRT)/lib/ -lpthread -lrt -lstdc++
CXXFLAGS2 := -lxilinxopencl

.PHONY: $(HOST_EXE)
# Building Host
$(HOST_EXE): src/host.cpp
  g++ $(CXXFLAGS) -o ../bin/host '$<' $(CXXFLAGS2)

.PHONY: cpu
cpu: cpu/cpu_code.c
  g++ $(CXXFLAGS) -o ../bin/cpu_code '$<' $(CXXFLAGS2) -fopenmp

.PHONY: emconfig
emconfig:
  emconfigutil --platform $(PLATFORM)

# Cleaning stuff
.PHONY: clean
clean:
  rm -f $(HOST_EXE) *mmult.$(TARGET).$(PLATFORM).* *.log *.json *.xo
