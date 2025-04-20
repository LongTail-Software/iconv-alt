cmake_minimum_required(VERSION 3.29)
project(iconv-alt VERSION 0.1.0 LANGUAGES C CXX)

# ---------- オプション ----------
option(BUILD_SHARED_LIBS "Build DLL instead of static lib" OFF)

# ---------- 自動生成テーブル ----------
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/include/sjis_table.h
  COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_sjis_table.py
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_sjis_table.py
  COMMENT "Generating sjis_table.h"
)
add_custom_target(gen_sjis_table DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/sjis_table.h)

# ---------- ライブラリ ----------
add_library(iconv
    STATIC
      src/iconv_core.c
      src/sjis.c
      src/utf8.c
      $<TARGET_GENEX_EVAL:gen_sjis_table,${CMAKE_CURRENT_SOURCE_DIR}/include/sjis_table.h>
)
target_include_directories(iconv PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_compile_features(iconv PUBLIC c_std_17)

if(MSVC)
  target_compile_options(iconv PRIVATE /utf-8)
endif()

add_dependencies(iconv gen_sjis_table)

# ---------- テスト ----------
enable_testing()
add_executable(smoke tests/smoke.cpp)
add_executable(sjis_utf8 tests/sjis_utf8.cpp)
add_test(NAME smoke COMMAND smoke)
add_test(NAME sjis_utf8 COMMAND sjis_utf8)
target_link_libraries(sjis_utf8 PRIVATE iconv)

# ---------- install ----------
install(TARGETS iconv DESTINATION lib)
install(FILES include/iconv.h DESTINATION include)
