CC = g++
BIN = ../lib/liblvcore.a
DEBUG = -g -O0
LIBS =
DEFINE =
INCLUDE = -I../include -I. -Iinclude
CXXFLAGS = -c $(DEBUG) $(DEFINE) -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing $(INCLUDE)
LFLAGS = -Wall $(DEBUG) $(LIBS)

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
   DEFINE += -D_SQ64
   CXXFLAGS += -m64
endif

OBJS = \
	sqapi.o \
	sqbaselib.o \
	sqfuncstate.o \
	sqdebug.o \
	sqlexer.o \
	sqobject.o \
	sqcompiler.o \
	sqstate.o \
	sqtable.o \
	sqmem.o \
	sqvm.o \
	sqaux.o \
	sqblob.o \
	sqstream.o \
	sqstring.o \
	sqio.o \
	sqclass.o

all: $(OBJS)
	$(AR) rc $(BIN) $(OBJS)

clean:
	$(RM) *.o
	$(RM) $(BIN)
