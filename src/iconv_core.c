/*----------------------------------------------------------------------
 *  src/iconv_core.c  ―  iconv_open / iconv / iconv_close
 *  SJIS ⇆ UTF‑8   (CP932 superset)  —  完全ストリーム対応
 *--------------------------------------------------------------------*/
#define _CRT_SECURE_NO_WARNINGS
#include "iconv.h"
#include "sjis_table.h"            /* SJIS_MAP[]                       */
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*======================================================================
 *  0.  External functions (defined in sjis.c and utf8.c)
 *====================================================================*/
extern int sjis_to_unicode(uint16_t code, uint32_t* uni);
extern int unicode_to_sjis(uint32_t uni, uint16_t* sjis);
extern int u32_to_utf8(uint32_t cp, char* out);

static int utf8_feed(uint8_t byte, uint8_t* buf, uint8_t* need, uint32_t* cp)
{
    /* Feed one byte; buf[0..*need-1] contains sequence being built.
       return -1 on illegal regardless, 0 if sequence incomplete, 1 if done. */
    if (*need == 0) {                 /* first byte */
        if (byte <= 0x7F) { *cp = byte; return 1; }
        else if ((byte & 0xE0) == 0xC0) { buf[0] = byte; *need = 1; *cp = byte & 0x1F; }
        else if ((byte & 0xF0) == 0xE0) { buf[0] = byte; *need = 2; *cp = byte & 0x0F; }
        else return -1;               /* 4‑byte以上を不正として弾く */
        return 0;
    }
    if ((byte & 0xC0) != 0x80) return -1;
    *cp = (*cp << 6) | (byte & 0x3F);
    (*need)--;
    return (*need == 0) ? 1 : 0;
}

/*======================================================================
 *  2.  状態構造体
 *====================================================================*/
typedef enum { M_SJIS2U8, M_U82SJIS } conv_mode;

typedef struct {
    conv_mode mode;
    /* --- pending for SJIS -> UTF‑8 --- */
    uint8_t    lead;          /* first byte saved          */
    uint8_t    have_lead;     /* 1 if lead is valid        */
    /* --- pending for UTF‑8 -> SJIS --- */
    uint8_t    utf8_need;     /* bytes still needed        */
    uint32_t   utf8_cp;       /* partially built scalar    */
} iconv_ctx;

/*======================================================================
 *  3.  iconv_open / close
 *====================================================================*/

/* Check if encoding name matches SJIS variants (case-insensitive) */
static int is_sjis_encoding(const char* name)
{
    /* Common SJIS aliases */
    static const char* sjis_aliases[] = {
        "SHIFT_JIS", "SHIFT-JIS", "SHIFTJIS",
        "SJIS", "CP932", "MS932", "WINDOWS-31J",
        "CSSHIFTJIS", "X-SJIS", "X-MS-CP932",
        NULL
    };
    for (const char** p = sjis_aliases; *p; ++p) {
        const char* a = *p;
        const char* n = name;
        /* Case-insensitive comparison */
        while (*a && *n) {
            char ca = (*a >= 'a' && *a <= 'z') ? (*a - 32) : *a;
            char cn = (*n >= 'a' && *n <= 'z') ? (*n - 32) : *n;
            if (ca != cn) break;
            ++a; ++n;
        }
        if (*a == '\0' && *n == '\0') return 1;
    }
    return 0;
}

/* Check if encoding name matches UTF-8 variants (case-insensitive) */
static int is_utf8_encoding(const char* name)
{
    static const char* utf8_aliases[] = {
        "UTF-8", "UTF8", "CSUTF8",
        NULL
    };
    for (const char** p = utf8_aliases; *p; ++p) {
        const char* a = *p;
        const char* n = name;
        while (*a && *n) {
            char ca = (*a >= 'a' && *a <= 'z') ? (*a - 32) : *a;
            char cn = (*n >= 'a' && *n <= 'z') ? (*n - 32) : *n;
            if (ca != cn) break;
            ++a; ++n;
        }
        if (*a == '\0' && *n == '\0') return 1;
    }
    return 0;
}

iconv_t iconv_open(const char* tocode, const char* fromcode)
{
    iconv_ctx* c = (iconv_ctx*)calloc(1, sizeof(iconv_ctx));
    if (!c) return (iconv_t)-1;

    if (is_sjis_encoding(fromcode) && is_utf8_encoding(tocode))
        c->mode = M_SJIS2U8;
    else if (is_utf8_encoding(fromcode) && is_sjis_encoding(tocode))
        c->mode = M_U82SJIS;
    else { free(c); errno = EINVAL; return (iconv_t)-1; }

    return (iconv_t)c;
}

int iconv_close(iconv_t cd)
{
    free(cd);
    return 0;
}

/*======================================================================
 *  4.  iconv() 本体
 *====================================================================*/
static int put_sjis(uint16_t sj, char** out, size_t* left)   /* helper */
{
    if (sj < 0x100) {
        if (*left < 1) return -1;
        *(*out)++ = (char)sj; (*left)--;
    }
    else {
        if (*left < 2) return -1;
        *(*out)++ = (char)(sj >> 8);
        *(*out)++ = (char)(sj & 0xFF);
        (*left) -= 2;
    }
    return 0;
}

size_t iconv(iconv_t cd,
    char** inbuf, size_t* inbytesleft,
    char** outbuf, size_t* outbytesleft)
{
    iconv_ctx* ctx = (iconv_ctx*)cd;
    if (!inbuf || !*inbuf) { errno = EINVAL; return (size_t)-1; }

    const unsigned char* p = (const unsigned char*)(*inbuf);
    const unsigned char* end = p + *inbytesleft;
    char* q = *outbuf;
    size_t  l = *outbytesleft;

    if (ctx->mode == M_SJIS2U8) {
        while (p < end) {
            uint16_t sj;

            /* --- バイト取得 SJIS --- */
            if (ctx->have_lead) {          /* 前回残った 1 バイトと結合 */
                sj = (ctx->lead << 8) | *p;
                ctx->have_lead = 0;  p++;
            }
            else {
                uint8_t b = *p++;
                if (b < 0x80)       sj = b;          /* ASCII */
                else if (b >= 0xA1 && b <= 0xDF) sj = b; /* 半角カナ */
                else {                               /* lead byte */
                    if (p >= end) {                  /* 不完全 */
                        ctx->lead = b; ctx->have_lead = 1;
                        errno = EINVAL; goto stop_err;
                    }
                    sj = (b << 8) | *p++;
                }
            }

            /* --- SJIS -> Unicode --- */
            uint32_t uni;
            if (sjis_to_unicode(sj, &uni) != 0) { errno = EILSEQ; goto stop_err; }

            /* --- Unicode -> UTF‑8 put --- */
            char tmp[4]; int n = u32_to_utf8(uni, tmp);
            if (l < (size_t)n) { p -= (ctx->have_lead ? 0 : 0); errno = E2BIG; goto stop_err; }
            memcpy(q, tmp, n); q += n; l -= n;
        }
    }
    else {  /* -------- UTF‑8 -> SJIS -------- */
        while (p < end) {
            int st = utf8_feed(*p++, &ctx->utf8_need, &ctx->utf8_need, &ctx->utf8_cp);
            if (st < 0) { errno = EILSEQ; goto stop_err; }
            if (st == 0) {               /* more bytes needed */
                if (p == end) { errno = EINVAL; goto stop_err; }
                continue;
            }
            /* 完成したコードポイント */
            uint16_t sj;
            if (unicode_to_sjis(ctx->utf8_cp, &sj) != 0) { errno = EILSEQ; goto stop_err; }
            if (put_sjis(sj, &q, &l) < 0) { errno = E2BIG; goto stop_err; }
            ctx->utf8_need = 0;
        }
    }

    /* --- 正常終了 --- */
    *inbytesleft = (size_t)(end - p);
    *inbuf = (char*)p;
    *outbytesleft = l;
    *outbuf = q;
    return 0;

stop_err:
    *inbytesleft = (size_t)(end - p);
    *inbuf = (char*)p;
    *outbytesleft = l;
    *outbuf = q;
    return (size_t)-1;
}
