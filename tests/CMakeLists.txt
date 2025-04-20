# tests/CMakeLists.txt — すべてのユニットテストをここで定義

# ----------------------------------------------------------
# 0. GoogleTest を取ってくる
#   (vcpkg toolchain 経由なら find_package で解決します)
# ----------------------------------------------------------
find_package(GTest REQUIRED)
include(GoogleTest)

# ----------------------------------------------------------
# 1. smoke — ビルド確認のみ
# ----------------------------------------------------------
add_executable(smoke smoke.cpp)
target_link_libraries(smoke PRIVATE iconv)
add_test(NAME smoke COMMAND smoke)

# ----------------------------------------------------------
# 2. sjis_utf8 — 基本ラウンドトリップ
# ----------------------------------------------------------
add_executable(sjis_utf8 sjis_utf8.cpp)
target_link_libraries(
    sjis_utf8
    PRIVATE
        iconv
        GTest::gtest
        GTest::gtest_main   # ★ 追加：main() を提供
)
gtest_discover_tests(sjis_utf8)

# ----------------------------------------------------------
# 3. auto_rt — 全コード点網羅 (存在すれば)
# ----------------------------------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_executable(auto_rt auto_rt.cpp)
  target_link_libraries(auto_rt PRIVATE iconv GTest::gtest GTest::gtest_main)
  target_link_options(auto_rt PRIVATE "/DEBUG:FASTLINK")  # 高速リンク
  gtest_discover_tests(auto_rt)
endif()

