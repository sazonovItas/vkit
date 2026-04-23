EXECUTABLE ?= ./build/vkit

.PHONY: glslc
glslc:
	glslc ./src/glsl/material/primitive.vert -o ./assets/shaders/material/primitive.vert
	glslc ./src/glsl/material/primitive_material_bsdf.frag -o ./assets/shaders/material/primitive_material_bsdf.frag
	glslc ./src/glsl/material/ray_sphere.vert -o ./assets/shaders/material/ray_sphere.vert
	glslc ./src/glsl/material/ray_sphere_material_bsdf.frag -o ./assets/shaders/material/ray_sphere_material_bsdf.frag

	# primitive shader
	glslc ./src/glsl/primitive.vert -o ./assets/shaders/primitive.vert
	glslc ./src/glsl/primitive.frag -o ./assets/shaders/primitive.frag

	# skybox shader
	glslc ./src/glsl/skybox.vert -o ./assets/shaders/skybox.vert
	glslc ./src/glsl/skybox.frag -o ./assets/shaders/skybox.frag

	# prefilter ibl compute shaders
	glslc ./src/glsl/prefilter_diffuse_ibl.comp -o ./assets/shaders/prefilter_diffuse_ibl.comp
	glslc ./src/glsl/prefilter_specular_ibl.comp -o ./assets/shaders/prefilter_specular_ibl.comp

	# procedural generation texture
	glslc ./src/glsl/procedural_texture.comp -o ./assets/shaders/procedural_texture.comp

.PHONY: cmake
cmake:
	cmake -S . -B build -DCMAKE_CXX_COMPILER=clang++

.PHONY: build
build: glslc cmake
	make -C build

.PHONY: run
run: build
	${EXECUTABLE}
