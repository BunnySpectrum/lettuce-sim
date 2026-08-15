cmake_minimum_required(VERSION 3.20)

project(LettuceTidy LANGUAGES C CXX)

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

add_library(LettuceTidy MODULE
    HeaderInternalObjectCheck.cpp
    LettuceTidyModule.cpp
)

target_compile_features(LettuceTidy PRIVATE cxx_std_17)
target_include_directories(LettuceTidy PRIVATE
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
)
target_compile_definitions(LettuceTidy PRIVATE ${LLVM_DEFINITIONS})
target_link_libraries(LettuceTidy PRIVATE clangTidy)

# clang-tidy's --load option accepts a regular shared-library path. Avoid the
# platform-specific "lib" prefix so scripts can use one predictable basename.
set_target_properties(LettuceTidy PROPERTIES PREFIX "")
if(APPLE)
    set_target_properties(LettuceTidy PROPERTIES SUFFIX ".dylib")
endif()
