cmake_minimum_required(VERSION 3.13 FATAL_ERROR)
project(wipeout-rewrite)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "In-tree builds are not allowed.")
endif()

include(GNUInstallDirs)
include(CMakeDependentOption)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if("${CMAKE_SYSTEM_NAME}" STREQUAL "Emscripten")
	set(EMSCRIPTEN true)
endif()

set(platform_options "SDL2" "SOKOL" "NULL")
set(PLATFORM "SDL2" CACHE STRING "Graphics platform to handle input/output")
set_property(CACHE PLATFORM PROPERTY STRINGS "${platform_options}")
if(NOT PLATFORM IN_LIST platform_options)
	message(FATAL_ERROR "PLATFORM must be one of ${platform_options}")
endif()

set(renderer_options "GL" "GLES2" "SOFTWARE" "NULL" "VULKAN")
set(RENDERER "GL" CACHE STRING "Graphics rendering backend")
set_property(CACHE RENDERER PROPERTY STRINGS "${renderer_options}")
if(NOT RENDERER IN_LIST renderer_options)
	message(FATAL_ERROR "RENDERER must be one of ${renderer_options}")
endif()
if("${RENDERER}" STREQUAL "VULKAN")
	set(using_vulkan true)
elseif("${RENDERER}" MATCHES "GL(ES2)?")
	set(using_gl true)
endif()
if("${CMAKE_SYSTEM_NAME}" MATCHES "(Linux|FreeBSD)")
	set(has_glvnd true)
endif()

cmake_dependent_option(USE_GLVND "Link against modern GLVND ABIs" ON "using_gl;has_glvnd" OFF)
cmake_dependent_option(MINIMAL_BUNDLE "Do not include music/movies for web builds" OFF "EMSCRIPTEN" OFF)
option(PATH_ASSETS "Path to where the game assets should be located.")
option(PATH_USERDATA "Path to where user data (e.g. game saves) should be located.")
option(DEV_BUILD "Set asset/userdata paths to the source directory for testing" OFF)
if (DEV_BUILD)
	set(PATH_ASSETS "${CMAKE_SOURCE_DIR}/")
	set(PATH_USERDATA "${CMAKE_SOURCE_DIR}/")
endif()

find_package(OpenGL)
find_package(GLEW)
find_package(SDL2)
find_package(Vulkan)

if(using_vulkan)
	find_program(GLSLC glslc)
	if(NOT GLSLC)
		message(FATAL_ERROR "glslc not found. Install the Vulkan SDK.")
	endif()
endif()

set(common_src
	src/wipeout/camera.c
	src/wipeout/camera.h
	src/wipeout/droid.c
	src/wipeout/droid.h
	src/wipeout/game.c
	src/wipeout/game.h
	src/wipeout/hud.c
	src/wipeout/hud.h
	src/wipeout/image.c
	src/wipeout/image.h
	src/wipeout/ingame_menus.c
	src/wipeout/ingame_menus.h
	src/wipeout/intro.c
	src/wipeout/intro.h
	src/wipeout/main_menu.c
	src/wipeout/main_menu.h
	src/wipeout/menu.c
	src/wipeout/menu.h
	src/wipeout/object.c
	src/wipeout/object.h
	src/wipeout/particle.c
	src/wipeout/particle.h
	src/wipeout/race.c
	src/wipeout/race.h
	src/wipeout/scene.c
	src/wipeout/scene.h
	src/wipeout/sfx.c
	src/wipeout/sfx.h
	src/wipeout/ship.c
	src/wipeout/ship.h
	src/wipeout/ship_ai.c
	src/wipeout/ship_ai.h
	src/wipeout/ship_player.c
	src/wipeout/ship_player.h
	src/wipeout/title.c
	src/wipeout/title.h
	src/wipeout/track.c
	src/wipeout/track.h
	src/wipeout/ui.c
	src/wipeout/ui.h
	src/wipeout/weapon.c
	src/wipeout/weapon.h
	src/input.c
	src/input.h
	src/mem.c
	src/mem.h
	src/platform.h
	src/render.h
	src/system.c
	src/system.h
	src/types.c
	src/types.h
	src/utils.c
	src/utils.h

	packaging/windows/wipeout.exe.manifest
	packaging/windows/wipeout.rc
)

add_executable(wipeout WIN32 ${common_src})
set_property(TARGET wipeout PROPERTY C_STANDARD 11)
target_include_directories(wipeout PRIVATE src)
target_include_directories(wipeout SYSTEM PRIVATE src/libs)
target_compile_options(wipeout PRIVATE
	$<$<CXX_COMPILER_ID:MSVC>:/W4>
	$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra>
)
target_compile_definitions(wipeout PRIVATE
	$<$<BOOL:${PATH_ASSETS}>:-DPATH_ASSETS=${PATH_ASSETS}>
	$<$<BOOL:${PATH_USERDATA}>:-DPATH_USERDATA=${PATH_USERDATA}>
)

if(WIN32)
	target_compile_definitions(wipeout PRIVATE
		"NOMINMAX"
		"_USE_MATH_DEFINES"
		"_CRT_SECURE_NO_WARNINGS"
	)
elseif(APPLE)
	target_compile_definitions(wipeout PRIVATE
		"_THREAD_SAFE"
		"GL_SILENCE_DEPRECATION"
	)
	target_link_libraries(wipeout PUBLIC "-framework Foundation")
	set_source_files_properties(src/platform_sokol.c PROPERTIES COMPILE_FLAGS "-x objective-c")
	if("${PLATFORM}" STREQUAL SOKOL)
		target_link_libraries(wipeout PUBLIC
			"-framework Cocoa"
			"-framework QuartzCore"
			"-framework AudioToolbox"
		)
	endif()
elseif(EMSCRIPTEN)
	# Emscripten's CMake modules don't define targets like the standard
	# ones do, so we define them ourselves here.
	add_library(GLEW::GLEW INTERFACE IMPORTED)
	add_library(OpenGL::GL INTERFACE IMPORTED)
	if (NOT TARGET SDL2::Main)
		add_library(SDL2::Main INTERFACE IMPORTED)
	endif()
	set_target_properties(OpenGL::GL PROPERTIES
		IMPORTED_LIBNAME "GL"
	)
	set_target_properties(GLEW::GLEW PROPERTIES
		IMPORTED_LIBNAME "GLEW"
	)
	set_target_properties(SDL2::Main PROPERTIES
		IMPORTED_LIBNAME "SDL2"
		INTERFACE_COMPILE_OPTIONS "SHELL:-s USE_SDL=2"
		INTERFACE_LINK_LIBRARIES  "SHELL:-s USE_SDL=2"
	)

	target_link_options(wipeout PRIVATE
		"SHELL:-s ALLOW_MEMORY_GROWTH=1"
		"SHELL:-s ENVIRONMENT=web"
		"SHELL:-s FORCE_FILESYSTEM"
		"SHELL:--preload-file ${CMAKE_SOURCE_DIR}/wipeout/@/wipeout"
	)
	if(MINIMAL_BUNDLE)
		target_link_options(wipeout PRIVATE
			"SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/wipeout/music"
			"SHELL:--exclude-file ${CMAKE_SOURCE_DIR}/intro.mpeg"
		)
	endif()

	configure_file("${CMAKE_SOURCE_DIR}/src/wasm-index.html" "game.html" COPYONLY)
elseif(UNIX)
	target_link_libraries(wipeout PUBLIC m)
	if ("${PLATFORM}" STREQUAL "SOKOL" AND "${CMAKE_SYSTEM_NAME}" STREQUAL "Linux")
		find_package(Threads REQUIRED)
		find_package(X11 REQUIRED)
		find_package(ALSA REQUIRED)
		target_link_libraries(wipeout PUBLIC
			X11::X11
			X11::Xcursor
			Threads::Threads
			X11::Xi
			dl
			ALSA::ALSA
		)
	endif()
endif()

if(using_gl)
	target_compile_definitions(wipeout PRIVATE "RENDERER_GL")
	target_sources(wipeout PRIVATE src/render_gl.c)

	target_include_directories(wipeout PUBLIC ${OPENGL_INCLUDE_DIR})
	if ("${RENDERER}" STREQUAL "GLES2")
		target_compile_definitions(wipeout PRIVATE "USE_GLES2")
		if (TARGET OpenGL::GLES2)
			target_link_libraries(wipeout PUBLIC OpenGL::GLES2)
		endif()
	endif()

	if(USE_GLVND AND TARGET OpenGL::OpenGL)
		target_link_libraries(wipeout PUBLIC OpenGL::OpenGL)
	else()
		target_link_libraries(wipeout PUBLIC OpenGL::GL)
	endif()

	if(NOT APPLE)
		target_include_directories(wipeout PRIVATE ${GLEW_INCLUDE_DIRS})
		target_link_libraries(wipeout PRIVATE GLEW::GLEW)
	endif()
elseif("${RENDERER}" STREQUAL "SOFTWARE")
	target_compile_definitions(wipeout PRIVATE "RENDERER_SOFTWARE")
	target_sources(wipeout PRIVATE src/render_software.c)
elseif("${RENDERER}" STREQUAL "NULL")
	target_compile_definitions(wipeout PRIVATE "RENDERER_NULL")
	target_sources(wipeout PRIVATE src/render_null.c)
elseif("${RENDERER}" STREQUAL "VULKAN")
	target_compile_definitions(wipeout PRIVATE "RENDERER_VULKAN")
	target_sources(wipeout PRIVATE src/render_vulkan.c)
	
	# Compile GLSL shaders to SPIR-V, embedded into the binary as C
	# initializer-list headers (render_vulkan.c #includes them as array
	# initializers — no runtime shader files, no path resolution).
	# -O: shader code optimization. Note: deliberately NO
	# --target-env=vulkan1.3 — at that target glslang pre-declares SPIR-V
	# Capability DemoteToHelperInvocation in every fragment module (even
	# where unused), which VVL rejects unless the matching device feature
	# is negotiated. The sources use no 1.3 shader features, and plain -O
	# output validates clean against --target-env vulkan1.3.
	set(SHADER_SOURCES
		src/shaders/game.vert.spvasrc
		src/shaders/game.frag.spvasrc
		src/shaders/post.vert.spvasrc
		src/shaders/post.frag.spvasrc
		src/shaders/post_crt.frag.spvasrc
	)
	set(COMPILED_SHADERS "")
	foreach(shader_src ${SHADER_SOURCES})
		get_filename_component(shader_name ${shader_src} NAME)
		set(shader_out "${CMAKE_CURRENT_BINARY_DIR}/shaders/${shader_name}.spvh")
		
		# Detect shader stage from filename
		set(SHADER_STAGE "vert")
		if(shader_src MATCHES ".frag.")
			set(SHADER_STAGE "frag")
		endif()
		
		add_custom_command(
			OUTPUT "${shader_out}"
			COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_BINARY_DIR}/shaders"
			COMMAND ${GLSLC} -mfmt=c -O -fshader-stage=${SHADER_STAGE} "${CMAKE_SOURCE_DIR}/${shader_src}" -o "${shader_out}"
			VERBATIM
			DEPENDS "${CMAKE_SOURCE_DIR}/${shader_src}"
		)
		list(APPEND COMPILED_SHADERS "${shader_out}")
	endforeach()
	add_custom_target(vulkan_shaders ALL DEPENDS ${COMPILED_SHADERS})
	# render_vulkan.c #includes the generated headers: make sure the shader
	# target runs (and the headers exist) before wipeout is compiled.
	add_dependencies(wipeout vulkan_shaders)
	target_include_directories(wipeout PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
	
	target_include_directories(wipeout PRIVATE ${Vulkan_INCLUDE_DIRS})
	# Headers only — do NOT link libvulkan: render_vulkan.c compiles with
	# VK_NO_PROTOTYPES and resolves every entry point at runtime
	# (SDL_Vulkan_GetVkGetInstanceProcAddr -> vkGetInstanceProcAddr ->
	# vkGetDeviceProcAddr), so the loader is loaded by SDL, not by us.
	# find_package(Vulkan) still provides the headers and glslc.
endif()

if("${PLATFORM}" STREQUAL SDL2)
	target_sources(wipeout PRIVATE src/platform_sdl.c)
	target_link_libraries(wipeout PUBLIC SDL2::Main)
elseif("${PLATFORM}" STREQUAL SOKOL)
	target_sources(wipeout PRIVATE src/platform_sokol.c)
elseif("${PLATFORM}" STREQUAL NULL)
	target_sources(wipeout PRIVATE src/platform_null.c)
endif()

install(TARGETS wipeout)
