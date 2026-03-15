EXECUTABLE ?= ./build/vkit

.PHONY: glslc
glslc:
	# primitive shader
	glslc ./src/glsl/primitive.vert -o ./assets/shaders/primitive.vert
	glslc ./src/glsl/primitive.frag -o ./assets/shaders/primitive.frag

	# skybox shader
	glslc ./src/glsl/skybox.vert -o ./assets/shaders/skybox.vert
	glslc ./src/glsl/skybox.frag -o ./assets/shaders/skybox.frag

	# prefilter compute shaders
	glslc ./src/glsl/prifilter_diffuse_ibl.comp -o ./assets/shaders/prefilter_diffuse_ibl.comp
	glslc ./src/glsl/prifilter_specular_ibl.comp -o ./assets/shaders/prifilter_specular_ibl.comp

.PHONY: cmake
cmake:
	cmake -S . -B build -DCMAKE_CXX_COMPILER=clang++

.PHONY: build
build: glslc cmake
	make -C build

.PHONY: run
run: build
	${EXECUTABLE}
