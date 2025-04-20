#ifndef ICONV_ALT_ICONV_H
#define ICONV_ALT_ICONV_H

#include <stddef.h>     /* size_t */

/* C と C++ で名前修飾を合わせる */
#ifdef __cplusplus
extern "C" {
#endif

/* ---------- 互換 API ―― 最小プロトタイプ ---------- */

typedef void* iconv_t;

iconv_t iconv_open(const char* tocode, const char* fromcode);

size_t iconv(iconv_t       cd,
             char**        inbuf,         size_t* inbytesleft,
             char**        outbuf,        size_t* outbytesleft);

int iconv_close(iconv_t cd);

#ifdef __cplusplus
}
#endif

#endif /* ICONV_ALT_ICONV_H */
