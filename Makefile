EXECUTABLE ?= ./build/vkit

.PHONY: glslc
glslc:
	glslc ./src/glsl/primitive.vert -o ./assets/shaders/primitive.vert
	glslc ./src/glsl/primitive.frag -o ./assets/shaders/primitive.frag

.PHONY: cmake
cmake:
	cmake -S . -B build -DCMAKE_CXX_COMPILER=clang++

.PHONY: build
build: glslc cmake
	make -C build

.PHONY: run
run: build
	${EXECUTABLE}
