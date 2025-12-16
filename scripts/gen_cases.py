#!/usr/bin/env python3
"""
Generate tests/auto_rt.cpp — round‑trip every CP932 code point.
Requires sjis_table.h to exist.

Handles both 1-byte (ASCII, half-width katakana) and 2-byte SJIS codes.
"""
from pathlib import Path
import re
import textwrap
from typing import List, Tuple

TABLE = Path("include/sjis_table.h")
DEST = Path("tests/auto_rt.cpp")

# Match both 2-digit (00XX) and 4-digit SJIS codes
pat = re.compile(r"\{0x([0-9A-F]{4}), 0x([0-9A-F]{4})\}", re.IGNORECASE)

pairs: List[Tuple[int, int]] = []
for line in TABLE.read_text(encoding="utf-8").splitlines():
    m = pat.search(line)
    if m:
        pairs.append((int(m[1], 16), int(m[2], 16)))


def escape_for_cpp(uni: int) -> str:
    """Convert Unicode code point to escaped C++ u8 string literal content.
    
    Returns the escaped string (without quotes) for use in u8"...".
    """
    # Encode to UTF-8 and escape each byte as \xHH
    utf8_bytes = chr(uni).encode('utf-8')
    return ''.join(f'\\x{b:02X}' for b in utf8_bytes)


body: List[str] = []
for sj, uni in pairs:
    escaped = escape_for_cpp(uni)
    if sj <= 0xFF:
        # 1-byte SJIS (ASCII or half-width katakana)
        body.append(f'    CASE1(0x{sj:02X}, u8"{escaped}");')
    else:
        # 2-byte SJIS
        body.append(f'    CASE2(0x{sj:04X}, u8"{escaped}");')


cpp = textwrap.dedent(f"""\
    #include <gtest/gtest.h>
    #include <iconv.h>
    #include <cstring>

    /* 1-byte SJIS (ASCII, half-width katakana) */
    #define CASE1(SJ, UNI)                                              \\
        do {{                                                           \\
            char s[]={{ (char)(SJ), 0 }};                               \\
            char out[8]={{}}; size_t inlen=1, outlen=sizeof(out);       \\
            char* in=s; char* o=out;                                    \\
            iconv_t cd=iconv_open("UTF-8","SHIFT_JIS");                 \\
            ASSERT_NE((iconv_t)-1,cd);                                  \\
            EXPECT_EQ(0u,iconv(cd,&in,&inlen,&o,&outlen));              \\
            iconv_close(cd);                                            \\
            EXPECT_STREQ(UNI, out);                                     \\
        }} while(0)

    /* 2-byte SJIS */
    #define CASE2(SJ, UNI)                                              \\
        do {{                                                           \\
            char s[]={{ (char)((SJ)>>8),(char)((SJ)&0xFF),0 }};         \\
            char out[8]={{}}; size_t inlen=2, outlen=sizeof(out);       \\
            char* in=s; char* o=out;                                    \\
            iconv_t cd=iconv_open("UTF-8","SHIFT_JIS");                 \\
            ASSERT_NE((iconv_t)-1,cd);                                  \\
            EXPECT_EQ(0u,iconv(cd,&in,&inlen,&o,&outlen));              \\
            iconv_close(cd);                                            \\
            EXPECT_STREQ(UNI, out);                                     \\
        }} while(0)

    TEST(SjisRoundTrip, AllPairs) {{
    {chr(10).join(body)}
    }}
""")

DEST.write_text(cpp, encoding="utf-8")
print(f"Wrote {DEST} with {len(pairs)} cases.")
