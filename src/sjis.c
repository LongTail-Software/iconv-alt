/*----------------------------------------------------------------------
 *  src/sjis.c  —  Shift‑JIS ⇆ UTF‑8 変換コア
 *--------------------------------------------------------------------*/
#include "sjis_table.h"   /* SJIS_MAP[] / sjis_pair_t                 */
#include <stddef.h>       /* size_t                                   */
#include <stdint.h>       /* uint16_t / uint32_t                      */
#include <string.h>       /* memcpy                                   */

 /*======================================================================
  *  1.  SJIS <-> Unicode ルックアップ
  *====================================================================*/

  /*--- SJIS → Unicode --------------------------------------------------*/
int sjis_to_unicode(uint16_t code, uint32_t* uni)
{
    int lo = 0, hi = (int)(sizeof(SJIS_MAP) / sizeof(SJIS_MAP[0])) - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (SJIS_MAP[mid].sjis == code) { *uni = SJIS_MAP[mid].uni; return 0; }
        (code < SJIS_MAP[mid].sjis) ? (hi = mid - 1) : (lo = mid + 1);
    }
    return -1;                 /* 該当なし */
}

/*--- Unicode → SJIS --------------------------------------------------*/
int unicode_to_sjis(uint32_t uni, uint16_t* sjis)
{
    /* 線形検索でも十分速い（17 k 件 × O(1)）が、簡易ハッシュで高速化 */
    static const uint32_t HASH_MOD = 65521;   /* largest prime < 2^16      */
    uint32_t start = uni % HASH_MOD;
    for (size_t i = 0; i < sizeof(SJIS_MAP) / sizeof(SJIS_MAP[0]); ++i) {
        size_t idx = (start + i) % (sizeof(SJIS_MAP) / sizeof(SJIS_MAP[0]));
        if (SJIS_MAP[idx].uni == uni) { *sjis = SJIS_MAP[idx].sjis; return 0; }
    }
    return -1;
}

/*======================================================================
 *  2.  Unicode <-> UTF‑8 ヘルパ
 *====================================================================*/
static int u32_to_utf8(uint32_t cp, char out[4])
{
    if (cp <= 0x7F) { out[0] = (char)cp; return 1; }
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
    /* Shift‑JIS に 4‑byte 対応は不要だが、生成だけ実装 */
    out[0] = (char)(0xF0 | (cp >> 18));
    out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
    out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
    out[3] = (char)(0x80 | (cp & 0x3F));
    return 4;
}

/* UTF‑8 1 文字をデコード。戻り値 0=OK, <0=不正   */
static int utf8_next(const unsigned char** p,
    const unsigned char* end,
    uint32_t* cp_out)
{
    if (*p >= end) return -1;
    uint32_t c = *(*p)++;
    if (c < 0x80) { *cp_out = c; return 0; }

    if ((c & 0xE0) == 0xC0) {               /* 2‑byte */
        if (*p >= end) return -1;
        uint32_t c2 = *(*p)++;
        if ((c2 & 0xC0) != 0x80) return -1;
        *cp_out = ((c & 0x1F) << 6) | (c2 & 0x3F);
        return 0;
    }
    if ((c & 0xF0) == 0xE0) {               /* 3‑byte */
        if (end - *p < 2) return -1;
        uint32_t c2 = *(*p)++, c3 = *(*p)++;
        if ((c2 & 0xC0) != 0x80 || (c3 & 0xC0) != 0x80) return -1;
        *cp_out = ((c & 0x0F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
        return 0;
    }
    return -1;                              /* SJIS 対象外 (4‑byte) */
}

/*======================================================================
 *  3.  Public 変換 API
 *====================================================================*/

 /*------------------------------------------------------------------*/
 /*  Shift‑JIS → UTF‑8 への一括変換                                 */
 /*------------------------------------------------------------------*/
size_t sjis_to_utf8_buf(const char* in, size_t inlen,
    char* out, size_t outlen)
{
    const unsigned char* p = (const unsigned char*)in;
    const unsigned char* end = p + inlen;
    char* q = out;
    char* q_end = out + outlen;

    while (p < end) {
        uint16_t sj = *p++;

        if (sj >= 0x80) {                  /* 2‑byte 先読み */
            if (p >= end) break;           /* 不完全で終了 */
            sj = (sj << 8) | *p++;
        }
        uint32_t uni;
        if (sjis_to_unicode(sj, &uni) != 0) continue; /* 変換不能 → スキップ */

        char tmp[4]; int n = u32_to_utf8(uni, tmp);
        if (q + n > q_end) break;          /* バッファ切れ */
        memcpy(q, tmp, n); q += n;
    }
    return (size_t)(q - out);
}

/*------------------------------------------------------------------*/
/*  UTF‑8 → Shift‑JIS への一括変換                                 */
/*------------------------------------------------------------------*/
size_t utf8_to_sjis_buf(const char* in, size_t inlen,
    char* out, size_t outlen)
{
    const unsigned char* p = (const unsigned char*)in;
    const unsigned char* end = p + inlen;
    char* q = out;
    char* q_end = out + outlen;

    while (p < end) {
        uint32_t cp;
        const unsigned char* mark = p;
        if (utf8_next(&p, end, &cp) < 0) break;     /* 不正列 → 終了 */

        uint16_t sj;
        if (unicode_to_sjis(cp, &sj) != 0) continue;/* 変換不能 → スキップ */

        if (sj < 0x100) {                           /* 1‑byte */
            if (q_end - q < 1) { p = mark; break; }
            *q++ = (char)sj;
        }
        else {                                    /* 2‑byte */
            if (q_end - q < 2) { p = mark; break; }
            *q++ = (char)(sj >> 8);
            *q++ = (char)(sj & 0xFF);
        }
    }
    return (size_t)(q - out);
}

/*------------------------------------------------------------------*/
/*  SJIS エンコードユーティリティ (iconv_core 用)                   */
/*------------------------------------------------------------------*/
int sjis_put(uint16_t code, char** out, size_t* left)
{
    if (code < 0x100) {
        if (*left < 1) return -1;
        *(*out)++ = (char)code;  (*left)--;
    }
    else {
        if (*left < 2) return -1;
        *(*out)++ = (char)(code >> 8);
        *(*out)++ = (char)(code & 0xFF);
        (*left) -= 2;
    }
    return 0;
}
