
set(ENGINE_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/")

file(GLOB_RECURSE ENGINE_HEADER_LIST CONFIGURE_DEPENDS  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp" 
                                                        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

file(GLOB_RECURSE ENGINE_SOURCE_LIST CONFIGURE_DEPENDS  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp" 
                                                        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")

add_library(Engine STATIC   ${ENGINE_HEADER_LIST} 
                            ${ENGINE_SOURCE_LIST})


target_compile_definitions(Engine PRIVATE GOLD_ENGINE)

target_compile_features(Engine PRIVATE cxx_std_17)

target_link_libraries(Engine glm)

target_include_directories(Engine PUBLIC ${ENGINE_HEADER_DIR})

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
