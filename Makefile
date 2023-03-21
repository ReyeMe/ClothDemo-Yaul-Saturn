THIS_ROOT:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

ifeq ($(strip $(YAUL_INSTALL_ROOT)),)
  $(error Undefined YAUL_INSTALL_ROOT (install root directory))
endif

include $(YAUL_INSTALL_ROOT)/share/build.pre.mk

# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS+=

SH_PROGRAM:= cloth-mic3d
SH_SRCS:= \
	cloth-mic3d.c \
	cloth-sim.c \

SH_LIBRARIES:= mic3d
SH_CFLAGS+= -O2 -I. -DDEBUG -g

IP_VERSION:= V1.000
IP_RELEASE_DATE:= 20160101
IP_AREAS:= JTUBKAEL
IP_PERIPHERALS:= JAMKST
IP_TITLE:= MIC 3D
IP_MASTER_STACK_ADDR:= 0x06004000
IP_SLAVE_STACK_ADDR:= 0x06001E00
IP_1ST_READ_ADDR:= 0x06004000
IP_1ST_READ_SIZE:= 0

include $(YAUL_INSTALL_ROOT)/share/build.post.iso-cue.mk
