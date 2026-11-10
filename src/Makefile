.PHONY: all cat grep test clean rebuild
all: cat grep

cat:
	$(MAKE) -C cat cat

grep:
	$(MAKE) -C grep grep

test: all
	./tests/integration.sh

clean:
	$(MAKE) -C cat clean
	$(MAKE) -C grep clean

rebuild: clean all
