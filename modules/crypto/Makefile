DEBUG = -g -O0
LIBS =
DEFINE =
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
	sha1.o \
	base64.o \
	urlcode.o \
	module.o

all: $(OBJS)

clean:
	$(RM) *.o
