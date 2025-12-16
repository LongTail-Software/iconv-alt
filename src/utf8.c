/*----------------------------------------------------------------------
 *  src/utf8.c  —  UTF-8 encoding/decoding utilities
 *--------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>

/*======================================================================
 *  u32_to_utf8 - Encode a Unicode code point to UTF-8
 *  
 *  Input:  cp  - Unicode code point (U+0000 to U+10FFFF)
 *          out - Output buffer (must have at least 4 bytes)
 *  Output: Number of bytes written (1-4)
 *====================================================================*/
int u32_to_utf8(uint32_t cp, char* out)
{
    if (cp <= 0x7F) {
        out[0] = (char)cp;
        return 1;
    }
    if (cp <= 0x7FF) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
        return 2;
    }
    if (cp <= 0xFFFF) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
        return 3;
    }
    out[0] = (char)(0xF0 | (cp >> 18));
    out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
}

/*======================================================================
 *  utf8_next - Decode one UTF-8 character
 *  
 *  Input:  p   - Pointer to current position (updated on success)
 *          end - End of buffer
 *  Output: out_cp - Decoded Unicode code point
 *  Return: 0 = OK, <0 = invalid sequence
 *====================================================================*/
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
