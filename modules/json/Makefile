CC = g++
DEBUG = -g -O0
LIBS =
DEFINE = -D_DEBUG
INCLUDE = -I../../include -I. -Iinclude
CXXFLAGS = -c $(DEBUG) $(DEFINE) -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing $(INCLUDE)
LFLAGS = -Wall $(DEBUG) $(LIBS)

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
   DEFINE += -D_SQ64
   CXXFLAGS += -m64
endif

OBJS= \
	module.o

all: $(OBJS)

clean:
	$(RM) *.o
