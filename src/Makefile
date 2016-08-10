CC = g++
BIN = ../lib/liblvcore.a
DEBUG = -g -O0
LIBS =
DEFINE = -D_DEBUG
INCLUDE = -I../include -I. -Iinclude
CXXFLAGS = -c $(DEBUG) $(DEFINE) -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing $(INCLUDE)
LFLAGS = -Wall $(DEBUG) $(LIBS)

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
   DEFINE += -D_SQ64
   CXXFLAGS += -m64
endif

OBJS = \
	api.o \
	baselib.o \
	funcstate.o \
	debug.o \
	lexer.o \
	object.o \
	compiler.o \
	state.o \
	table.o \
	mem.o \
	vm.o \
	aux.o \
	blob.o \
	stream.o \
	lvstring.o \
	io.o \
	class.o

all: $(OBJS)
	$(AR) rc $(BIN) $(OBJS)

clean:
	$(RM) *.o
	$(RM) $(BIN)
