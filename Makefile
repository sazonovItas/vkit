EXECUTABLE ?= ./build/vkit

.PHONY: glslc
glslc:
	glslc ./src/glsl/mesh.vert -o ./assets/shaders/mesh.vert
	glslc ./src/glsl/mesh.frag -o ./assets/shaders/mesh.frag

.PHONY: cmake
cmake:
	cmake -S . -B build

.PHONY: build
build: glslc cmake
	make -C build

.PHONY: run
run: build
	${EXECUTABLE}
