.PHONY: build run clean

build:
	@cmake -B cmake-build-debug > /dev/null 2>&1
	@cmake --build cmake-build-debug

run: build
	@./cmake-build-debug/interpreter

clean:
	@rm -rf cmake-build-debug
	@echo "Cleaned build directory"