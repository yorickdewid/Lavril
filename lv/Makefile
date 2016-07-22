CC = g++
BIN = ../bin/lv
LIBDIR = -L../lib
DEBUG = -g -O0
LIBS = -lsquirrel -lsqstdlib
DEFINE =
INCLUDE = -I../include -I. -Iinclude
CXXFLAGS = -c $(DEBUG) $(DEFINE) -m64 -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing $(INCLUDE)
CFLAGS = -c $(DEBUG) $(DEFINE) -m64 -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing $(INCLUDE)
LFLAGS = -Wall $(DEBUG) $(LIBDIR) $(LIBS)

OBJS= \
	lv.o

all: $(OBJS)
	$(CC) $(OBJS) $(LFLAGS) -o $(BIN)

clean:
	$(RM) *.o

#sq64:
#	g++ -O2 -m64 -fno-exceptions -fno-rtti -D_SQ64 -o $(OUT) $(SRCS) $(INCZ) $(LIBZ) $(LIB)
#
#sq32:
#	g++ -O2 -fno-exceptions -fno-rtti -o $(OUT) $(SRCS) $(INCZ) $(LIBZ) $(LIB)
#
#sqprof:
#	g++ -O2 -pg -fno-exceptions -fno-rtti -pie -gstabs -g3 -o $(OUT) $(SRCS) $(INCZ) $(LIBZ) $(LIB)

