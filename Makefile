##
## Copyright (c) 2016 okertanov@gmail.com
##

##
## Configuration
##
PWD:=$(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))

##
## Tools
##
CXX:=clang++

##
## Build Configuration
##
CXXFLAGS:=\
	-Wall -Wextra \
	-I./ \
	-std=c++14

LDFLAGS:=\
	-framework Hypervisor

LDLIBS:=

##
## Sources
##
SRCS:=vm.cpp util.cpp osxv.cpp
EXCLUDE:=
SRCS:=$(filter-out $(EXCLUDE),$(SRCS))
OBJS:=$(SRCS:.cpp=.o)

all: osxv

osxv: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(OBJS) -o $@

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	-@rm -f $(OBJS)
	-@rm -f osxv

.PHONY: all clean

.SILENT: clean
