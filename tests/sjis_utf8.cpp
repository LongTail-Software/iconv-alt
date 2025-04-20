#include <gtest/gtest.h>
#include <iconv.h>
#include <cstring>

TEST(RoundTrip, Basic) {
    const char sjis[] = "\x82\xa0\x82\xa2\x82\xa4";        // ‚ ‚¢‚¤
    char utf8[32]{}, back[32]{};
    size_t in = sizeof(sjis) - 1, out = sizeof(utf8);

    char* p = (char*)sjis;
    char* q = utf8;

    iconv_t cd = iconv_open("UTF-8", "SHIFT_JIS");
    ASSERT_NE((iconv_t)-1, cd);
    ASSERT_EQ(0u, iconv(cd, &p, &in, &q, &out));
    iconv_close(cd);

    p = utf8; in = strlen(utf8); q = back; out = sizeof(back);
    cd = iconv_open("SHIFT_JIS", "UTF-8");
    ASSERT_EQ(0u, iconv(cd, &p, &in, &q, &out));
    iconv_close(cd);

    EXPECT_STREQ(sjis, back);
}
