add_executable(TestApp main.cpp)

target_compile_features(TestApp PRIVATE cxx_std_17)

target_link_libraries(TestApp Engine)