cmake_minimum_required(VERSION 3.29)
project(iconv-alt VERSION 0.1.0 LANGUAGES C CXX)

# --------------------------------------------------------------------
# 0. オプション
# --------------------------------------------------------------------
option(BUILD_SHARED_LIBS "Build shared library (DLL)" OFF)

# --------------------------------------------------------------------
# 1. 自動生成: CP932 → Unicode テーブル
# --------------------------------------------------------------------
find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_custom_command(
  OUTPUT   ${CMAKE_CURRENT_SOURCE_DIR}/include/sjis_table.h
  COMMAND  ${Python3_EXECUTABLE}
           ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_sjis_table.py
  DEPENDS  ${CMAKE_CURRENT_SOURCE_DIR}/scripts/gen_sjis_table.py
  COMMENT  "Generating sjis_table.h from CP932.TXT"
)

add_custom_target(gen_sjis_table
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/include/sjis_table.h
)

# --------------------------------------------------------------------
# 2. ライブラリ iconv (STATIC / SHARED 可)
# --------------------------------------------------------------------
add_library(iconv
    STATIC                               # BUILD_SHARED_LIBS=ON なら SHARED になる
      src/iconv_core.c
      src/sjis.c
      src/utf8.c
)

add_dependencies(iconv gen_sjis_table)   # ヘッダ生成を先に

target_include_directories(iconv PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(iconv PUBLIC c_std_17)

if(MSVC)
  target_compile_options(iconv PRIVATE /utf-8)
endif()

# --------------------------------------------------------------------
# 3. インストール (任意)
# --------------------------------------------------------------------
include(GNUInstallDirs)
install(TARGETS iconv
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES include/iconv.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# --------------------------------------------------------------------
# 4. テスト: tests/ に一任
# --------------------------------------------------------------------
enable_testing()
add_subdirectory(tests)
