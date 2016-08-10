BIN = ../bin/lv
LIBDIR = -L../lib
DEBUG = -g -O0
LIBS = -llvcore -llvmods -lcurl
DEFINE = -D_DEBUG
INCLUDE = -I../include -I. -Iinclude
CXXFLAGS = -c $(DEBUG) $(DEFINE) -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing $(INCLUDE)
CFLAGS = -c $(DEBUG) $(DEFINE) -Wall -fno-strict-aliasing $(INCLUDE) -std=c99
LFLAGS = -Wall $(DEBUG) $(LIBDIR) $(LIBS)

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
   DEFINE += -D_LV64
   CXXFLAGS += -m64
endif

OBJS= \
	lv.o

all: $(OBJS)
	$(CXX) $(OBJS) $(LFLAGS) -o $(BIN)

clean:
	$(RM) *.o
	$(RM) $(BIN)
