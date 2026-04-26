EXECUTABLE ?= ./build/vkit

.PHONY: glslc
glslc:
	glslc ./shaders/primitive.vert -o ./assets/shaders/primitive.vert
	glslc ./shaders/primitive_diffuse.frag -o ./assets/shaders/primitive_diffuse.frag
	glslc ./shaders/primitive_diffuse_specular.frag -o ./assets/shaders/primitive_diffuse_specular.frag
	glslc ./shaders/primitive_principled_bsdf.frag -o ./assets/shaders/primitive_principled_bsdf.frag

	glslc ./shaders/ray_sphere.vert -o ./assets/shaders/ray_sphere.vert
	glslc ./shaders/ray_sphere_diffuse.frag -o ./assets/shaders/ray_sphere_diffuse.frag
	glslc ./shaders/ray_sphere_diffuse_specular.frag -o ./assets/shaders/ray_sphere_diffuse_specular.frag
	glslc ./shaders/ray_sphere_principled_bsdf.frag -o ./assets/shaders/ray_sphere_principled_bsdf.frag

	glslc ./shaders/skybox.vert -o ./assets/shaders/skybox.vert
	glslc ./shaders/skybox.frag -o ./assets/shaders/skybox.frag

	glslc ./shaders/ibl/brdf_lut.comp -o ./assets/shaders/ibl/brdf_lut.comp
	glslc ./shaders/ibl/diffuse_ibl.comp -o ./assets/shaders/ibl/diffuse_ibl.comp
	glslc ./shaders/ibl/specular_ibl.comp -o ./assets/shaders/ibl/specular_ibl.comp

.PHONY: cmake
cmake:
	cmake -S . -B build -DCMAKE_CXX_COMPILER=clang++

.PHONY: build
build: glslc cmake
	make -C build

.PHONY: run
run: build
	${EXECUTABLE}
