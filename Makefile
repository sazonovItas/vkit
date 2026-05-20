EXECUTABLE_DEBUG   ?= ./build/linux/debug/vkit
EXECUTABLE_RELEASE ?= ./build/linux/release/vkit

# Default assets bundled into zips
MODEL       ?= mechdrone
ENV_MAP     ?= blaubeuren_night_4k.hdr

.PHONY: glslc
glslc:
	glslc ./shaders/imgui.vert -o ./assets/shaders/imgui.vert
	glslc ./shaders/imgui.frag -o ./assets/shaders/imgui.frag

	glslc ./shaders/primitive.vert -o ./assets/shaders/primitive.vert
	glslc ./shaders/primitive_debug.frag -o ./assets/shaders/primitive_debug.frag
	glslc ./shaders/primitive_material.frag -o ./assets/shaders/primitive_material.frag

	glslc ./shaders/ray_sphere.vert -o ./assets/shaders/ray_sphere.vert
	glslc ./shaders/ray_sphere_debug.frag -o ./assets/shaders/ray_sphere_debug.frag
	glslc ./shaders/ray_sphere_material.frag -o ./assets/shaders/ray_sphere_material.frag

	glslc ./shaders/skybox.vert -o ./assets/shaders/skybox.vert
	glslc ./shaders/skybox.frag -o ./assets/shaders/skybox.frag

	glslc ./shaders/shadow.vert -o ./assets/shaders/shadow.vert

	glslc ./shaders/light_gizmo.vert -o ./assets/shaders/light_gizmo.vert
	glslc ./shaders/light_gizmo.frag -o ./assets/shaders/light_gizmo.frag

	glslc ./shaders/outline.vert -o ./assets/shaders/outline.vert
	glslc ./shaders/outline.frag -o ./assets/shaders/outline.frag

	glslc ./shaders/ibl/brdf_lut.comp -o ./assets/shaders/ibl/brdf_lut.comp
	glslc ./shaders/ibl/diffuse_ibl.comp -o ./assets/shaders/ibl/diffuse_ibl.comp
	glslc ./shaders/ibl/specular_ibl.comp -o ./assets/shaders/ibl/specular_ibl.comp

	glslc ./shaders/procedural/fractal.comp -o ./assets/shaders/procedural/fractal.comp
	glslc ./shaders/procedural/noise.comp -o ./assets/shaders/procedural/noise.comp
	glslc ./shaders/procedural/pattern.comp -o ./assets/shaders/procedural/pattern.comp

	glslc ./shaders/operators/sobel.comp -o ./assets/shaders/operators/sobel.comp
	glslc ./shaders/operators/heightmap.comp -o ./assets/shaders/operators/heightmap.comp
	glslc ./shaders/operators/normalmap.comp -o ./assets/shaders/operators/normalmap.comp
	glslc ./shaders/operators/tint.comp -o ./assets/shaders/operators/tint.comp
	glslc ./shaders/operators/mix.comp -o ./assets/shaders/operators/mix.comp
	glslc ./shaders/operators/channel_remap.comp -o ./assets/shaders/operators/channel_remap.comp
	glslc ./shaders/operators/channel_adjust.comp -o ./assets/shaders/operators/channel_adjust.comp

.PHONY: debug
debug: glslc
	cmake --preset debug
	cmake --build --preset debug

.PHONY: release
release: glslc
	cmake --preset release
	cmake --build --preset release

.PHONY: run-debug
run: debug
	${EXECUTABLE_DEBUG}

.PHONY: run-release
run-release: release
	${EXECUTABLE_RELEASE}

.PHONY: debug-win
debug-win: glslc
	cmake --preset debug-win
	cmake --build --preset debug-win

.PHONY: release-win
release-win: glslc
	cmake --preset release-win
	cmake --build --preset release-win

.PHONY: _zip
_zip:
	rm -rf dist/$(PLATFORM)/$(VARIANT)
	mkdir -p dist/$(PLATFORM)/$(VARIANT)/assets/models
	mkdir -p dist/$(PLATFORM)/$(VARIANT)/assets/environment
	cp $(EXE) dist/$(PLATFORM)/$(VARIANT)/$(EXE_NAME)
	cp -r assets/shaders dist/$(PLATFORM)/$(VARIANT)/assets/
	cp -r assets/fonts   dist/$(PLATFORM)/$(VARIANT)/assets/
	cp assets/environment/$(ENV_MAP) dist/$(PLATFORM)/$(VARIANT)/assets/environment/
	cp -r assets/models/$(MODEL)     dist/$(PLATFORM)/$(VARIANT)/assets/models/
	cp -r examples                   dist/$(PLATFORM)/$(VARIANT)/
	rm -f dist/$(PLATFORM)/vkit-$(PLATFORM)-$(VARIANT).zip
	zip -r dist/$(PLATFORM)/vkit-$(PLATFORM)-$(VARIANT).zip dist/$(PLATFORM)/$(VARIANT)/

.PHONY: zip-debug
zip-debug: debug
	$(MAKE) _zip PLATFORM=linux VARIANT=debug \
	    EXE=build/linux/debug/vkit EXE_NAME=vkit

.PHONY: zip-release
zip-release: release
	$(MAKE) _zip PLATFORM=linux VARIANT=release \
	    EXE=build/linux/release/vkit EXE_NAME=vkit

.PHONY: zip-debug-win
zip-debug-win: debug-win
	$(MAKE) _zip PLATFORM=win VARIANT=debug \
	    EXE=build/win/debug/vkit.exe EXE_NAME=vkit.exe

.PHONY: zip-release-win
zip-release-win: release-win
	$(MAKE) _zip PLATFORM=win VARIANT=release \
	    EXE=build/win/release/vkit.exe EXE_NAME=vkit.exe
