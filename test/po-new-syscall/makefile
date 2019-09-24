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

CC = g++
LD = g++
CXX = g++

# Flags passed to the C++ compiler.
CXXFLAGS += -g -Wall -Wno-unused -Wno-sign-compare -Wextra -pthread -I. -I$(ROOTDIR) -std=c++11

run-all: run-ut run-mt run-mmt run-st

run-ut: all-ut

run-mt: all-mt

run-mmt: all-mmt

run-st:	all-st

clean:
	@rm -rf $(OBJDIR)/*
