#include <stdint.h>
#include <stddef.h>

/* 返り値: 0=OK, <0=不正系列 */
int utf8_next(const unsigned char** p, const unsigned char* end, uint32_t* out_cp)
{
    if (*p == end) return -1;
    uint32_t c = *(*p)++;
    if (c < 0x80) { *out_cp = c; return 0; }
    if ((c & 0xE0) == 0xC0) {          /* 2 byte */
        if (*p >= end) return -1;
        uint32_t c2 = *(*p)++;
        if ((c2 & 0xC0) != 0x80) return -1;
        *out_cp = ((c & 0x1F) << 6) | (c2 & 0x3F);
        return 0;
    }
    if ((c & 0xF0) == 0xE0) {          /* 3 byte */
        if (end - *p < 2) return -1;
        uint32_t c2 = *(*p)++, c3 = *(*p)++;
        if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) return -1;
        *out_cp = ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
        return 0;
    }
    /* 4 byte は Shift‑JIS に無いので不正とみなす */
    return -1;
}
