CC = g++
BIN = ../bin/lv
LIBDIR = -L../lib
DEBUG = -g -O0
LIBS = -llvcore -llvmods -lcurl
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
	lv.o

all: $(OBJS)
	$(CC) $(OBJS) $(LFLAGS) -o $(BIN)

clean:
	$(RM) *.o
	$(RM) $(BIN)

#sq64:
#	g++ -O2 -m64 -fno-exceptions -fno-rtti -D_SQ64 -o $(OUT) $(SRCS) $(INCZ) $(LIBZ) $(LIB)
#
#sq32:
#	g++ -O2 -fno-exceptions -fno-rtti -o $(OUT) $(SRCS) $(INCZ) $(LIBZ) $(LIB)
#
#sqprof:
#	g++ -O2 -pg -fno-exceptions -fno-rtti -pie -gstabs -g3 -o $(OUT) $(SRCS) $(INCZ) $(LIBZ) $(LIB)

