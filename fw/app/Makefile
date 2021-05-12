################################################################################
# Copyright (C) 2014 Maxim Integrated Products, Inc., All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
# OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
# ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
# OTHER DEALINGS IN THE SOFTWARE.
#
# Except as contained in this notice, the name of Maxim Integrated
# Products, Inc. shall not be used except as stated in the Maxim Integrated
# Products, Inc. Branding Policy.
#
# The mere transfer of this software does not imply any licenses
# of trade secrets, proprietary technology, copyrights, patents,
# trademarks, maskwork rights, or any other form of intellectual
# property whatsoever. Maxim Integrated Products, Inc. retains all
# ownership rights.
#
# $Id: Makefile 44505 2019-07-09 20:32:03Z nathan.goldstick $
#
################################################################################

# Include project configuration.
ifneq "$(wildcard config.mk)" ""
include config.mk
endif

# This is the name of the build output file
ifeq "$(PROJECT)" ""
PROJECT=max32665
endif

# Specify the target processor
ifeq "$(TARGET)" ""
TARGET=MAX32665
endif

# Select 'GCC' or 'IAR' compiler
COMPILER=GCC

# Specify the board used
ifeq "$(BOARD)" ""
BOARD=EvKit_V1
endif

# Create Target name variables
TARGET_UC:=$(shell echo $(TARGET) | tr a-z A-Z)
TARGET_LC:=$(shell echo $(TARGET) | tr A-Z a-z)

# Unset PROJ_CFLAGS so remote building of this application does not carry over
# already defined flags
PROJ_CFLAGS=

# Last octet address modifier
ifneq "$(BLE_ADDR_LSB)" ""
PROJ_CFLAGS+=-DBLE_ADDR_LSB=$(BLE_ADDR_LSB)
endif

## Subsitute WSF critical section entry for default MAXUSB
#PROJ_CFLAGS+=-DMAXUSB_ENTER_CRITICAL=WsfCsEnter
#PROJ_CFLAGS+=-DMAXUSB_EXIT_CRITICAL=WsfCsExit

# This is the path to the CMSIS root directory
ifeq "$(MAXIM_PATH)" ""
# LIBS_DIR=../../../Libraries
# NDALIBS_DIR=../../../NDALibraries
else
# LIBS_DIR=/$(subst \,/,$(subst :,,$(MAXIM_PATH))/Firmware/$(TARGET_UC)/Libraries)
endif

LIBS_DIR=../../fw/Libraries
NDALIBS_DIR=../../fw/NDALibraries
CMSIS_ROOT=$(LIBS_DIR)/CMSIS

# Source files for this test (add path to VPATH below)
SRCS  = main.c

# Board Support Package
SRCS += bsp.c
SRCS += bsp_temp.c
SRCS += bsp_sh.c

# Chip driver
SRCS += max30208.c
SRCS += max32664.c

# BLE handle
SRCS += ble_stack.c
SRCS += ble_main.c

# BLE services
SRCS += ble_bos.c
SRCS += ble_bas.c

# BLE services application
SRCS += bas_app.c

# Where to find source files for this test
VPATH  = .
VPATH  += sys
VPATH  += components
VPATH  += bsp
VPATH  += ble_services
VPATH  += ble_app

# Where to find header files for this test
IPATH  = .
IPATH  += sys
IPATH  += components
IPATH  += bsp
IPATH  += ble_services
IPATH  += ble_app

##############################################################################
# OS Check/Utility definitions
ifeq '$(findstring ;,$(PATH))' ';'
    BUILD_OS := Windows
else
    BUILD_OS := $(shell uname 2>/dev/null || echo Unknown)
    BUILD_OS := $(patsubst MSYS%,MSYS,$(BUILD_OS))
    BUILD_OS := $(patsubst MINGW%,MSYS,$(BUILD_OS))
endif

################################################################################
# Project build configuration.

#-------------------------------------------------------------------------------
# Configuration passed via environment vars.

ifdef BTLE_APP_USE_LEGACY_API
ifeq "$(BTLE_APP_USE_LEGACY_API)" ""
PROJ_CFLAGS+=-DBTLE_APP_USE_LEGACY_API=TRUE
else
ifeq "$(BTLE_APP_USE_LEGACY_API)" "0"
else
ifeq "$(BTLE_APP_USE_LEGACY_API)" "FALSE"
else
PROJ_CFLAGS+=-DBTLE_APP_USE_LEGACY_API=TRUE
endif
endif
endif
endif

ifdef CONSOLE_UART
ifneq "$(CONSOLE_UART)" ""
PROJ_CFLAGS+=-DCONSOLE_UART=$(CONSOLE_UART)
endif
endif

ifdef FW_VERSION
ifneq "$(FW_VERSION)" ""
PROJ_CFLAGS+=-DFW_VERSION=$(FW_VERSION)
endif
endif

ifdef ENABLE_WDX
ifneq "$(ENABLE_WDX)" ""
ifneq "$(ENABLE_WDX)" "0"
PROJ_CFLAGS+=-DWDXC_INCLUDED=TRUE
PROJ_CFLAGS+=-DWDXS_INCLUDED=TRUE
# PROJ_CFLAGS+=-DWSF_TRACE_ENABLED=TRUE
SRCS += wdxs_file.c

# Use linkerfile to create room for bootloader
LINKERFILE=max32665_file.ld

endif
endif
endif

ifdef ENABLE_SDMA
ifneq "$(ENABLE_SDMA)" ""
ifeq "$(ENABLE_SDMA)" "0"
ENABLE_SDMA=
else
ifeq "$(ENABLE_SDMA)" "FALSE"
ENABLE_SDMA=
else
ENABLE_SDMA=1
endif
endif
endif
endif

ifdef ENABLE_SDMA_UNIFIED_CONFIG
ifneq "$(ENABLE_SDMA_UNIFIED_CONFIG)" ""
ifeq "$(ENABLE_SDMA_UNIFIED_CONFIG)" "0"
ENABLE_SDMA_UNIFIED_CONFIG=
else
ifeq "$(ENABLE_SDMA_UNIFIED_CONFIG)" "FALSE"
ENABLE_SDMA_UNIFIED_CONFIG=
else
ENABLE_SDMA_UNIFIED_CONFIG=1
endif
endif
endif
endif

ifdef ENABLE_SDMA_UNIFIED_CONFIG
ifneq "$(ENABLE_SDMA_UNIFIED_CONFIG)" "0"
PROJ_CFLAGS+=-DENABLE_SDMA_UNIFIED_CONFIG=1
endif
endif

ifneq "$(ENABLE_SDMA)" ""
ifdef BTLE_APP_USE_LEGACY_API
ifneq "$(BTLE_APP_USE_LEGACY_API)" ""
ifeq "$(BTLE_APP_USE_LEGACY_API)" "0"
$(error SDMA currently requires BTLE_APP_USE_LEGACY_API)
endif
ifeq "$(BTLE_APP_USE_LEGACY_API)" "FALSE"
$(error SDMA currently requires BTLE_APP_USE_LEGACY_API)
endif
endif
endif
endif

#-------------------------------------------------------------------------------
# Configuration specified here.

#--------------------
# Stack configuration

# Enable assertion checking for development
PROJ_CFLAGS+=-DMXC_ASSERT_ENABLE=TRUE
PROJ_CFLAGS+=-DWSF_ASSERT_ENABLED=TRUE
# PROJ_CFLAGS+=-DLL_TRACE_ENABLED=TRUE
# PROJ_CFLAGS+=-DLL_DBG_PIN_ENABLED=TRUE


#--------------------
# Application CFLAGS configuration

#--------------------
# Stack CFLAGS configuration

PROJ_CFLAGS+=-DINIT_BROADCASTER
# PROJ_CFLAGS+=-DINIT_OBSERVER
# PROJ_CFLAGS+=-DINIT_CENTRAL
PROJ_CFLAGS+=-DINIT_PERIPHERAL
PROJ_CFLAGS+=-DINIT_ENCRYPTED
PROJ_CFLAGS+=-DINIT_PHY

ENABLE_LL_TESTER?=#1

# Use this variable to override default compilier optimization.
#MXC_OPTIMIZE_CFLAGS=-Os
#MXC_OPTIMIZE_CFLAGS=-O0 -fomit-frame-pointer

# Point this variable to a linker file to override the default file
#LINKERFILE=$(CMSIS_ROOT)/Device/Maxim/$(TARGET_UC)/Source/GCC/$(TARGET_LC).ld

################################################################################
# Include external library makefiles here
DEBUG=1

# Include the BSP
BOARD_DIR=$(LIBS_DIR)/Boards/$(BOARD)
include $(BOARD_DIR)/board.mk

# Include the Peripheral Driver Library
PERIPH_DRIVER_DIR=$(LIBS_DIR)/$(TARGET)PeriphDriver
include ${PERIPH_DRIVER_DIR}/periphdriver.mk

# Include Cordio BTLE Library
CORDIO_DIR=$(LIBS_DIR)/BTLE
include ${CORDIO_DIR}/btle.mk

################################################################################
# Include the rules for building for this target. All other makefiles should be
# included before this one.
include $(CMSIS_ROOT)/Device/Maxim/$(TARGET_UC)/Source/$(COMPILER)/$(TARGET_LC).mk
