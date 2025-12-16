#include <gtest/gtest.h>
#include <iconv.h>
#include <cerrno>
#include <cstring>

/* 既存: 正常ラウンドトリップ ------------------------------------- */
TEST(RoundTrip, Basic) {
    const char sjis[] = "\x82\xa0\x82\xa2\x82\xa4";  // あいう
    char utf8[32]{}, back[32]{};
    size_t in = sizeof(sjis) - 1, out = sizeof(utf8);

    char* p = (char*)sjis, * q = utf8;
    iconv_t cd = iconv_open("UTF-8", "SHIFT_JIS");
    ASSERT_NE((iconv_t)-1, cd);
    ASSERT_EQ(0u, iconv(cd, &p, &in, &q, &out));
    iconv_close(cd);

    p = utf8; in = strlen(utf8); q = back; out = sizeof(back);
    cd = iconv_open("SHIFT_JIS", "UTF-8");
    ASSERT_NE((iconv_t)-1, cd);
    ASSERT_EQ(0u, iconv(cd, &p, &in, &q, &out));
    iconv_close(cd);

    EXPECT_STREQ(sjis, back);
}

/* -----------------------------------------------------------------
 * 半角カナのラウンドトリップ (0xA1-0xDF → U+FF61-U+FF9F)
 * ----------------------------------------------------------------*/
TEST(RoundTrip, HalfWidthKatakana) {
    // ｱｲｳｴｵ (SJIS: A1 B2 B3 B4 B5 → UTF-8: U+FF61, U+FF72, U+FF73, U+FF74, U+FF75)
    const char sjis[] = "\xB1\xB2\xB3\xB4\xB5";  // ｱｲｳｴｵ
    char utf8[32]{}, back[32]{};
    size_t in = sizeof(sjis) - 1, out = sizeof(utf8);

    char* p = (char*)sjis, * q = utf8;
    iconv_t cd = iconv_open("UTF-8", "SHIFT_JIS");
    ASSERT_NE((iconv_t)-1, cd);
    ASSERT_EQ(0u, iconv(cd, &p, &in, &q, &out));
    iconv_close(cd);

    // Verify UTF-8 output: ｱｲｳｴｵ
    EXPECT_STREQ(u8"ｱｲｳｴｵ", utf8);

    // Round-trip back to SJIS
    p = utf8; in = strlen(utf8); q = back; out = sizeof(back);
    cd = iconv_open("SHIFT_JIS", "UTF-8");
    ASSERT_NE((iconv_t)-1, cd);
    ASSERT_EQ(0u, iconv(cd, &p, &in, &q, &out));
    iconv_close(cd);

    EXPECT_STREQ(sjis, back);
}

/* -----------------------------------------------------------------
 * ASCII のラウンドトリップ
 * ----------------------------------------------------------------*/
TEST(RoundTrip, Ascii) {
    const char sjis[] = "Hello, World!";
    char utf8[32]{}, back[32]{};
    size_t in = sizeof(sjis) - 1, out = sizeof(utf8);

    char* p = (char*)sjis, * q = utf8;
    iconv_t cd = iconv_open("UTF-8", "SHIFT_JIS");
    ASSERT_NE((iconv_t)-1, cd);
    ASSERT_EQ(0u, iconv(cd, &p, &in, &q, &out));
    iconv_close(cd);

    EXPECT_STREQ("Hello, World!", utf8);

    // Round-trip back to SJIS
    p = utf8; in = strlen(utf8); q = back; out = sizeof(back);
    cd = iconv_open("SHIFT_JIS", "UTF-8");
    ASSERT_NE((iconv_t)-1, cd);
    ASSERT_EQ(0u, iconv(cd, &p, &in, &q, &out));
    iconv_close(cd);

    EXPECT_STREQ(sjis, back);
}

/* -----------------------------------------------------------------
 * 混合テスト: 全角 + 半角カナ + ASCII
 * ----------------------------------------------------------------*/
TEST(RoundTrip, Mixed) {
    // あいう + ｱｲｳ + ABC
    const char sjis[] = "\x82\xa0\x82\xa2\x82\xa4\xB1\xB2\xB3" "ABC";
    char utf8[64]{}, back[64]{};
    size_t in = sizeof(sjis) - 1, out = sizeof(utf8);

    char* p = (char*)sjis, * q = utf8;
    iconv_t cd = iconv_open("UTF-8", "SHIFT_JIS");
    ASSERT_NE((iconv_t)-1, cd);
    ASSERT_EQ(0u, iconv(cd, &p, &in, &q, &out));
    iconv_close(cd);

    EXPECT_STREQ(u8"あいうｱｲｳABC", utf8);

    // Round-trip back to SJIS
    p = utf8; in = strlen(utf8); q = back; out = sizeof(back);
    cd = iconv_open("SHIFT_JIS", "UTF-8");
    ASSERT_NE((iconv_t)-1, cd);
    ASSERT_EQ(0u, iconv(cd, &p, &in, &q, &out));
    iconv_close(cd);

    EXPECT_STREQ(sjis, back);
}

/* -----------------------------------------------------------------
 * エラー 1: 変換不能文字 (U+1F600 😀) → errno = EILSEQ
 * ----------------------------------------------------------------*/
TEST(Error, Utf8ToSjis_IllegalSequence) {
    const char utf8[] = u8"😀";           // 4‑byte UTF‑8
    char sjis[8]{};
    char* in = (char*)utf8, * out = sjis;
    size_t inleft = strlen(utf8), outleft = sizeof(sjis);

    iconv_t cd = iconv_open("SHIFT_JIS", "UTF-8");
    ASSERT_NE((iconv_t)-1, cd);

    errno = 0;
    EXPECT_EQ((size_t)-1, iconv(cd, &in, &inleft, &out, &outleft));
    EXPECT_EQ(EILSEQ, errno);            // ← ここを検証
    iconv_close(cd);
}

/* -----------------------------------------------------------------
 * エラー 2: 入力が途中で切れた → errno = EINVAL
 * ----------------------------------------------------------------*/
TEST(Error, Utf8ToSjis_IncompleteSequence) {
    const char part[] = "\xE3\x81";      // “あ” の 3‑byte UTF‑8 の先頭 2 バイト
    char sjis[8]{};
    char* in = (char*)part, * out = sjis;
    size_t inleft = sizeof(part) - 1, outleft = sizeof(sjis);

    iconv_t cd = iconv_open("SHIFT_JIS", "UTF-8");
    ASSERT_NE((iconv_t)-1, cd);

    errno = 0;
    EXPECT_EQ((size_t)-1, iconv(cd, &in, &inleft, &out, &outleft));
    EXPECT_EQ(EINVAL, errno);            // 不完全入力
    iconv_close(cd);
}

/* -----------------------------------------------------------------
 * エラー 3: 出力バッファが足りない → errno = E2BIG
 * ----------------------------------------------------------------*/
TEST(Error, SjisToUtf8_BufferTooSmall) {
    const char sjis[] = "\x82\xa0\x82\xa2\x82\xa4";  // あいう (SJIS 6 バイト)
    char tiny[4]{};                                 // わざと小さく
    char* in = (char*)sjis, * out = tiny;
    size_t inleft = sizeof(sjis) - 1, outleft = sizeof(tiny);

    iconv_t cd = iconv_open("UTF-8", "SHIFT_JIS");
    ASSERT_NE((iconv_t)-1, cd);

    errno = 0;
    EXPECT_EQ((size_t)-1, iconv(cd, &in, &inleft, &out, &outleft));
    EXPECT_EQ(E2BIG, errno);            // 出力不足
    iconv_close(cd);
}

/* -----------------------------------------------------------------
 * エンコーディング名エイリアス: CP932, SJIS, utf-8 など
 * ----------------------------------------------------------------*/
TEST(Alias, CP932ToUtf8) {
    const char sjis[] = "\x82\xa0";  // あ
    char utf8[8]{};
    char* in = (char*)sjis, * out = utf8;
    size_t inleft = 2, outleft = sizeof(utf8);

    iconv_t cd = iconv_open("UTF-8", "CP932");
    ASSERT_NE((iconv_t)-1, cd);
    EXPECT_EQ(0u, iconv(cd, &in, &inleft, &out, &outleft));
    iconv_close(cd);
    EXPECT_STREQ(u8"あ", utf8);
}

TEST(Alias, Utf8ToSjis) {
    const char utf8[] = u8"あ";
    char sjis[8]{};
    char* in = (char*)utf8, * out = sjis;
    size_t inleft = strlen(utf8), outleft = sizeof(sjis);

    iconv_t cd = iconv_open("sjis", "utf8");  // 小文字でもOK
    ASSERT_NE((iconv_t)-1, cd);
    EXPECT_EQ(0u, iconv(cd, &in, &inleft, &out, &outleft));
    iconv_close(cd);
    EXPECT_STREQ("\x82\xa0", sjis);
}

TEST(Alias, Windows31J) {
    iconv_t cd = iconv_open("UTF-8", "Windows-31J");
    EXPECT_NE((iconv_t)-1, cd);
    if (cd != (iconv_t)-1) iconv_close(cd);
}

TEST(Alias, InvalidEncoding) {
    iconv_t cd = iconv_open("UTF-8", "UNKNOWN");
    EXPECT_EQ((iconv_t)-1, cd);
}
