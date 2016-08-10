LIBDIR = -L../lib
DEBUG = -g -O0
LIBS = -llvcore -llvmods
DEFINE = -D_DEBUG
INCLUDE = -I../include -I. -Iinclude
CXXFLAGS = -c $(DEBUG) $(DEFINE) -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing $(INCLUDE)
CFLAGS = -c $(DEBUG) $(DEFINE) -Wall -fno-strict-aliasing $(INCLUDE) -std=gnu99
LFLAGS = -Wall $(DEBUG) $(LIBDIR) $(LIBS)

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
   DEFINE += -D_SQ64
   CXXFLAGS += -m64
endif

all: minimal compiler runner vmext lvsh

minimal: minimal.o
	$(CXX) minimal.o $(LFLAGS) -o minimal

compiler: compiler.o
	$(CXX) compiler.o $(LFLAGS) -o compiler

runner: runner.o
	$(CXX) runner.o $(LFLAGS) -o runner

vmext: vmext.o
	$(CXX) vmext.o $(LFLAGS) -o vmext

lvsh: lvsh.o
	$(CXX) lvsh.o $(LFLAGS) -o lvsh

fwrapper: fwrapper.o
	$(CXX) fwrapper.o $(LFLAGS) -lfcgi -o fwrapper

clean:
	$(RM) *.o
	$(RM) minimal compiler runner vmext lvsh fwrapper
