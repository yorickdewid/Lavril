CC = g++
BIN = minimal
LIBDIR = -L../lib
DEBUG = -g -O0
LIBS = -llvcore -llvmods
DEFINE =
INCLUDE = -I../include -I. -Iinclude
CXXFLAGS = -c $(DEBUG) $(DEFINE) -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing $(INCLUDE)
CFLAGS = -c $(DEBUG) $(DEFINE) -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing $(INCLUDE)
LFLAGS = -Wall $(DEBUG) $(LIBDIR) $(LIBS)

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
   DEFINE += -D_SQ64
   CXXFLAGS += -m64
endif

OBJS= \
	minimal.o

all: $(OBJS)
	$(CC) $(OBJS) $(LFLAGS) -o $(BIN)

compiler: compiler.o
	$(CC) compiler.o $(LFLAGS) -o compiler

runner: runner.o
	$(CC) runner.o $(LFLAGS) -o runner

vmext: vmext.o
	$(CC) vmext.o $(LFLAGS) -o vmext

fwrapper: fwrapper.o
	$(CC) fwrapper.o $(LFLAGS) -lfcgi -o fwrapper

clean:
	$(RM) *.o
	$(RM) $(BIN)
	$(RM) compiler runner vmext
