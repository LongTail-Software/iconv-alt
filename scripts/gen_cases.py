#!/usr/bin/env python3
"""
Generate tests/auto_rt.cpp — round‑trip every CP932 code point.
Requires sjis_table.h to exist.
"""
from pathlib import Path
import re, textwrap

TABLE = Path("include/sjis_table.h")
DEST  = Path("tests/auto_rt.cpp")

pat = re.compile(r"\{0x([0-9A-F]{4}), 0x([0-9A-F]{4})}")

pairs = []
for line in TABLE.read_text(encoding="utf-8").splitlines():
    m = pat.search(line)
    if m:
        pairs.append((int(m[1], 16), int(m[2], 16)))

body = []
for sj, uni in pairs:
    body.append(f'''    CASE(0x{sj:04X}, u"{chr(uni)}");''')

cpp = textwrap.dedent(f"""
    #include <gtest/gtest.h>
    #include <iconv.h>
    #include <cstring>

    #define CASE(SJ, UNI)                                               \\
        do {{                                                           \\
            char s[]={{ (char)((SJ)>>8),(char)((SJ)&0xFF),0 }};         \\
            char out[8]={{}}; size_t inlen=2,outlen=sizeof(out);        \\
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
