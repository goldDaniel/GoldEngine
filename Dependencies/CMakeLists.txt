add_subdirectory_with_folder(Dependencies/glm "glm")

#SDL
if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
set_property(GLOBAL PROPERTY SDL_BINARY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/SDL/bin_msvc")
else()
    message(FATAL_ERROR "Could not find supported compiler")
endif()

set_property(GLOBAL PROPERTY SDL2_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/SDL/include")

get_property(sdl_bin_dir GLOBAL PROPERTY SDL_BINARY_DIR)
set_property(GLOBAL PROPERTY SDL2_LIBRARIES "${sdl_bin_dir}/SDL2.lib;${sdl_bin_dir}/SDL2main.lib")

#glad
file(GLOB_RECURSE GLAD_SOURCE_LIST "${PROJECT_SOURCE_DIR}/Dependencies/glad/src/*.cpp")
set_property(GLOBAL PROPERTY GLAD_SOURCE ${GLAD_SOURCE_LIST})
set_property(GLOBAL PROPERTY GLAD_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glad/include")


#assimp
set(ZLIB_LIBRARIES zlibstatic)
set(ENABLE_BOOST_WORKAROUND ON)
set(BUILD_STATIC_LIB ON)

add_subdirectory_with_folder(Dependencies/assimp "assimp")
