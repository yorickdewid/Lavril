SRC = src
MOD = modules
CLI = cli
UTIL = util

la64: folders
	$(MAKE) -C $(SRC)
	$(MAKE) -C $(MOD)
	$(MAKE) -C $(CLI)
	$(MAKE) -C $(UTIL)

clean:
	$(MAKE) -C $(SRC) clean
	$(MAKE) -C $(MOD) clean
	$(MAKE) -C $(CLI) clean
	$(MAKE) -C $(UTIL) clean

sqprof: folders
	cd squirrel; $(MAKE) sqprof
	cd sqstdlib; $(MAKE) sqprof
	cd sq; $(MAKE) sqprof

folders:
	@mkdir -p lib
	@mkdir -p bin
