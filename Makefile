SRC = src
MOD = modules
CLI = cli
TEST = test
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

test: la64
	$(MAKE) -C $(TEST)

folders:
	@mkdir -p lib
	@mkdir -p bin
