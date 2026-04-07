.PHONY: build clean

build:
	@cmake -S . -B build
	@cmake --build build

clean:
	@rm -rf build
	@echo "Cleaned build directory"
