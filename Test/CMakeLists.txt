file(GLOB_RECURSE TEST_APP_HEADER_LIST CONFIGURE_DEPENDS  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp" 
                                                        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

file(GLOB_RECURSE TEST_APP_SOURCE_LIST CONFIGURE_DEPENDS  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(TestApp  ${TEST_APP_HEADER_LIST}
                        ${TEST_APP_SOURCE_LIST})

target_compile_features(TestApp PRIVATE cxx_std_17)

target_link_libraries(TestApp Engine)