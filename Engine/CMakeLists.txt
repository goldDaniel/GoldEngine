
set(ENGINE_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/")

file(GLOB_RECURSE ENGINE_HEADER_LIST CONFIGURE_DEPENDS  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp" 
                                                        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

file(GLOB_RECURSE ENGINE_SOURCE_LIST CONFIGURE_DEPENDS  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" 
                                                        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

file(GLOB_RECURSE GLAD_SOURCE_LIST "${PROJECT_SOURCE_DIR}/Dependencies/glad/src/*.cpp"
                                   "${PROJECT_SOURCE_DIR}/Dependencies/glad/src/*.c")

add_library(Engine STATIC   ${ENGINE_HEADER_LIST} 
                            ${ENGINE_SOURCE_LIST}
                            ${GLAD_SOURCE_LIST})


target_compile_definitions(Engine PRIVATE GOLD_ENGINE)

target_compile_features(Engine PRIVATE cxx_std_17)

target_include_directories(Engine PUBLIC ${ENGINE_HEADER_DIR})

#glm
target_link_libraries(Engine glm)

#SDL
get_property(sdl_include GLOBAL PROPERTY SDL2_INCLUDE_DIRS)
get_property(sdl_lib GLOBAL PROPERTY SDL2_LIBRARIES)
target_include_directories(Engine PRIVATE ${sdl_include})
target_link_libraries(Engine ${sdl_lib})

#glad
get_property(glad_include GLOBAL PROPERTY GLAD_INCLUDE_DIR)
target_include_directories(Engine PRIVATE ${glad_include})

#stb 
target_include_directories(Engine PRIVATE "${PROJECT_SOURCE_DIR}/Dependencies/stb/")

#entt
target_include_directories(Engine PUBLIC "${PROJECT_SOURCE_DIR}/Dependencies/entt/single_include")

foreach(_source IN ITEMS ${ENGINE_SOURCE_LIST})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "${CMAKE_SOURCE_DIR}" "" _group_path "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

foreach(_source IN ITEMS ${ENGINE_HEADER_LIST})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "${CMAKE_SOURCE_DIR}" "" _group_path "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()



get_property(sdl_lib_dir GLOBAL PROPERTY SDL_BINARY_DIR)
add_custom_command(TARGET Engine POST_BUILD COMMAND "${CMAKE_COMMAND}" -E copy "${sdl_lib_dir}/SDL2.dll" "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/")