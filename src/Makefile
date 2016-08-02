CC = g++
BIN = ../lib/liblvcore.a
DEBUG = -g -O0
LIBS =
#DEFINE = -D_DEBUG_DUMP -D_SQ64
DEFINE = -D_SQ64
INCLUDE = -I../include -I. -Iinclude
CXXFLAGS = -c $(DEBUG) $(DEFINE) -m64 -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing $(INCLUDE)
LFLAGS = -Wall $(DEBUG) $(LIBS)

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

# OLD
#sq64:
#	gcc -O2 -m64 -D_SQ64 -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing -c $(SRCS) $(INCZ) $(DEFS)
#	ar rc $(OUT) *.o
#	rm *.o

#sq32:
#	gcc -O2 -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing -c $(SRCS) $(INCZ) $(DEFS)
#	ar rc $(OUT) *.o
#	rm *.o

#sqprof:
#	gcc -O2 -pg -fno-exceptions -fno-rtti -pie -gstabs -g3 -Wall -fno-strict-aliasing -c $(SRCS) $(INCZ) $(DEFS)
#	ar rc $(OUT) *.o
#	rm *.o

