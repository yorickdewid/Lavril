BIN = ../lib/liblvmods.a
OBJS = $(wildcard */*.o)
SUBDIRS = $(shell find . -maxdepth 1 ! -path . -type d)

all: build
	$(AR) rc $(BIN) $(OBJS)

build:
	@for dir in $(SUBDIRS) ; do \
		$(MAKE) -C $$dir ; \
	done

clean:
	@for dir in $(SUBDIRS) ; do \
		$(MAKE) -C $$dir clean ; \
	done
	$(RM) $(BIN)
