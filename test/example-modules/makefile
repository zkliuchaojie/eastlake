.PHONY: clean run-ut

# used for storing binary files
OBJDIR = obj
# this is the eastlake directory
ROOTDIR = ../../../eastlake
UTDIR = ut
MTDIR = mt
MMTDIR = mmt
STDIR = st

include $(ROOTDIR)/test/test-framework/makefile
include ut/makefile
include mt/makefile
include mmt/makefile
include st/makefile

CC = gcc
LD = gcc
AR = ar

# Flags passed to the C++ compiler.
CFLAGS += -g -Wall -Wno-unused -Wno-sign-compare -Wextra -pthread -I. -I$(ROOTDIR) -ansi
ARFLAGS = crv

run-all: run-ut run-mt run-mmt run-st

run-ut: all-ut

run-mt: all-mt

run-mmt: all-mmt

run-st:	all-st

clean:
	@rm -rf $(OBJDIR)/*
