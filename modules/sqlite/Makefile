DEBUG = -g -O0
LIBS =
DEFINE = -D_DEBUG
INCLUDE = -I../../include -I. -Iinclude
CXXFLAGS = -c $(DEBUG) $(DEFINE) -fno-exceptions -fno-rtti -Wall -fno-strict-aliasing $(INCLUDE)
CFLAGS = -c $(DEBUG) $(DEFINE) -Wall -fno-strict-aliasing $(INCLUDE) -std=c99
LFLAGS = -Wall $(DEBUG) $(LIBS)

LBITS := $(shell getconf LONG_BIT)
ifeq ($(LBITS),64)
   DEFINE += -D_LV64
   CXXFLAGS += -m64
endif

OBJS= \
	sqlite3.o \
	module.o

all: $(OBJS)

sqlite3.o: sqlite3.c
	$(CC) -c -o $@ $< -Wall -fno-strict-aliasing $(INCLUDE) -std=c99 -DSQLITE_OMIT_LOAD_EXTENSION -DSQLITE_THREADSAFE=0

clean:
	$(RM) *.o
