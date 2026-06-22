CCACHE_DIR ?= $(CURDIR)/build/.ccache

./bin/Linux/main: src/* include/* CMakeLists.txt CMakePresets.json
	cmake --preset default-config
	CCACHE_DIR="$(CCACHE_DIR)" cmake --build --preset default-build

.PHONY: clean run
clean:
	rm -f bin/Linux/main

run: ./bin/Linux/main
	cd bin/Linux && ./main
