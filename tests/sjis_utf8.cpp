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
