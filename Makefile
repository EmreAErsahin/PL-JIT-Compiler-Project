.PHONY: cli runtime embedding test clean

cli:
	@cmake -S . -B build
	@cmake --build build --target interpreter

runtime:
	@cmake -S . -B build
	@cmake --build build --target ee_runtime

test: cli
	@cmake --build build --target embedding_tests
	@./scripts/run_all_tests.sh

clean:
	@rm -rf build