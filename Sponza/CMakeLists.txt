file(GLOB_RECURSE TEST_APP_HEADER_LIST CONFIGURE_DEPENDS  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp" 
                                                        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

file(GLOB_RECURSE TEST_APP_SOURCE_LIST CONFIGURE_DEPENDS  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

add_executable(Sponza  ${TEST_APP_HEADER_LIST}
                        ${TEST_APP_SOURCE_LIST})

target_compile_features(Sponza PRIVATE cxx_std_17)

target_link_libraries(Sponza Engine)

if(MSVC)
  target_compile_options(Sponza PRIVATE /W4 /WX)
  set_target_properties(Sponza PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "DataFolderRedirect=../../../Sponza/Assets")
else()
  target_compile_options(Sponza PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()