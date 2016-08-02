CC = g++
BIN = ../lib/liblvmods.a
DEBUG = -g -O0
LIBS =
DEFINE = -D_SQ64
INCLUDE = -I../../include -I. -Iinclude
CXXFLAGS = -c $(DEBUG) $(DEFINE) -m64 -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing $(INCLUDE)
LFLAGS = -Wall $(DEBUG) $(LIBS)

OBJS= \
	module.o

all: $(OBJS)

clean:
	$(RM) *.o
