BIN = ../lib/liblvmods.a
OBJS = $(wildcard */*.o)
MODS = \
	math \
	crypto \
	curl \
	json \
	sqlite \
	pgsql \
	system

all: build
	$(AR) rc $(BIN) $(OBJS)

build:
	@for dir in $(MODS) ; do \
		$(MAKE) -C $$dir ; \
	done

clean:
	@for dir in $(MODS) ; do \
		$(MAKE) -C $$dir clean ; \
	done
	$(RM) $(BIN)
