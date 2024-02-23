file(GLOB_RECURSE TEST_APP_HEADER_LIST CONFIGURE_DEPENDS  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp" 
                                                        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h")

file(GLOB_RECURSE TEST_APP_SOURCE_LIST CONFIGURE_DEPENDS  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")

file(GLOB SHADER_LIST "${CMAKE_CURRENT_SOURCE_DIR}/Assets/shaders/*.glsl")

add_executable(VulkanSandbox ${TEST_APP_HEADER_LIST}
                             ${TEST_APP_SOURCE_LIST}
                             ${SHADER_LIST})

source_group("shaders" FILES ${SHADER_LIST})

target_compile_features(VulkanSandbox PRIVATE cxx_std_17)
find_package(Vulkan REQUIRED)

target_link_libraries(VulkanSandbox Engine Vulkan::Vulkan)

if(MSVC)
  target_compile_options(VulkanSandbox PRIVATE /W4 /WX)
  set_target_properties(VulkanSandbox PROPERTIES VS_DEBUGGER_COMMAND_ARGUMENTS "DataFolderRedirect=../../../VulkanSandbox/Assets")
else()
  target_compile_options(VulkanSandbox PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()