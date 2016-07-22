MAKE = make
SRC = squirrel
LIB = sqstdlib
CLI = lv

la64: folders
	$(MAKE) -C $(SRC)
	$(MAKE) -C $(LIB)
	$(MAKE) -C $(CLI)

clean:
	$(MAKE) -C $(SRC) clean
	$(MAKE) -C $(LIB) clean
	$(MAKE) -C $(CLI) clean

sqprof: folders
	cd squirrel; $(MAKE) sqprof
	cd sqstdlib; $(MAKE) sqprof
	cd sq; $(MAKE) sqprof

folders:
	@mkdir -p lib
	@mkdir -p bin
