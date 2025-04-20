#include <iconv.h>
#include <cassert>

int main() {
    const char s[] = "abc";
    char buf[16]{};
    size_t ilen = 3, olen = sizeof(buf);
    char* in = (char*)s, * out = buf;

    iconv_t cd = iconv_open("UTF-8", "SHIFT_JIS");
    assert(cd != (iconv_t)-1);
    iconv_close(cd);
    return 0;
}
