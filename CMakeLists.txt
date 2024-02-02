
cmake_minimum_required(VERSION 3.12...3.27)

project(GoldEngine VERSION 1.0 LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "") 
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin CACHE STRING "")

# 64bit only
if (NOT ${CMAKE_SIZEOF_VOID_P} MATCHES 8)
    message(FATAL_ERROR "We only support 64bit builds!")
endif()



find_package(Git QUIET)

if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                    RESULT_VARIABLE GIT_SUBMOD_RESULT)
    if(NOT GIT_SUBMOD_RESULT EQUAL "0")
        message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
    endif()
endif()


# Let's nicely support folders in IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
define_property(
    TARGET
    PROPERTY FOLDER
    INHERITED
    BRIEF_DOCS "Set the folder name."
    FULL_DOCS  "Use to organize targets in an IDE."
)

function(add_subdirectory_with_folder _folder_name _folder)
    add_subdirectory(${_folder} ${ARGN})
    set_property(DIRECTORY "${_folder}" PROPERTY FOLDER "${_folder_name}")
endfunction()


add_subdirectory_with_folder(Dependencies Dependencies)
add_subdirectory_with_folder(Engine Engine)
add_subdirectory_with_folder("Asset Processor" AssetProcessor)
add_subdirectory(Sponza)
add_subdirectory(VulkanSandbox)
