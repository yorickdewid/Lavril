CC = g++
BIN = minimal
LIBDIR = -L../lib
DEBUG = -g -O0
LIBS = -llvcore -llvmods
DEFINE =
INCLUDE = -I../include -I. -Iinclude
CXXFLAGS = -c $(DEBUG) $(DEFINE) -m64 -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing $(INCLUDE)
CFLAGS = -c $(DEBUG) $(DEFINE) -m64 -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing $(INCLUDE)
LFLAGS = -Wall $(DEBUG) $(LIBDIR) $(LIBS)

OBJS= \
	minimal.o

all: $(OBJS)
	$(CC) $(OBJS) $(LFLAGS) -o $(BIN)

clean:
	$(RM) *.o
	$(RM) $(BIN)
