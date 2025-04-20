cmake_minimum_required(VERSION 3.29)
project(iconv-alt VERSION 0.0.0 LANGUAGES C CXX)

option(BUILD_SHARED_LIBS "Build DLL instead of static lib" ON)
option(ENABLE_ASAN       "Enable AddressSanitizer"        OFF)

add_library(iconv STATIC src/iconv_stub.c)


target_include_directories(iconv PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>       # 'cmake --install' 用
)
target_compile_features(iconv PUBLIC c_std_17)

if(ENABLE_ASAN AND MSVC)
    target_compile_options(iconv PUBLIC /fsanitize=address)
    target_link_options(iconv    PUBLIC /fsanitize=address)
endif()

enable_testing()
add_subdirectory(tests)
