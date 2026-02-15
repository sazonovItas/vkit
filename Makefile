.PHONY: glslc
glslc:
	glslc ./src/glsl/shader.vert -o ./assets/shaders/shader.vert
	glslc ./src/glsl/shader.frag -o ./assets/shaders/shader.frag

.PHONY: cmake
cmake:
	cmake -S . -B build

.PHONY: build
build: glslc cmake
	make -C build

.PHONY: run
run: build
	./build/learn-vk
