CC = g++
BIN = ../lib/liblvstdlib.a
DEBUG = -g -O0
LIBS =
DEFINE = -D_SQ64
INCLUDE = -I../include -I. -Iinclude
CXXFLAGS = -c $(DEBUG) $(DEFINE) -m64 -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing $(INCLUDE)
LFLAGS = -Wall $(DEBUG) $(LIBS)

OBJS= \
	sqstdblob.o \
	sqstdio.o \
	sqstdstream.o \
	sqstdmath.o \
	sqstdsystem.o \
	sqstdstring.o \
	sqstdaux.o \
	sqstdrex.o

all: $(OBJS)
	$(AR) rc $(BIN) $(OBJS)

clean:
	$(RM) *.o
	$(RM) $(BIN)

#sq64:
#	gcc -O2 -m64 -fno-exceptions -D_SQ64 -fno-rtti -Wall -fno-strict-aliasing -c $(SRCS) $(INCZ)
#	ar rc $(OUT) *.o
#	rm *.o

#sq32:
#	gcc -O2 -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing -c $(SRCS) $(INCZ)
#	ar rc $(OUT) *.o
#	rm *.o

#sqprof:
#	gcc -O2 -pg -fno-exceptions -fno-rtti -pie -gstabs -g3 -Wall -fno-strict-aliasing -c $(SRCS) $(INCZ)
#	ar rc $(OUT) *.o
#	rm *.o
