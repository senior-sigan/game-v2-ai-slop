SHELL := /bin/bash
PROJECT := Game
TEST_PATH := test.lua

.PHONY: all
all: build run

.PHONY: reload
reload:
	cmake -Bbuild

.PHONY: build
build: reload
	cmake --build build -j 4

.PHONY: clean
clean:
	rm -rf build

.PHONY: test
test: build
	./build/$(PROJECT) $(TEST_PATH)

.PHONY: run
run:
	./build/$(PROJECT)

.PHONY: tidy
tidy: reload
	cmake --build build --target tidy
