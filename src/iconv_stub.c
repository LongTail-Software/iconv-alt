#include "iconv.h"
#include <stddef.h>

iconv_t iconv_open(const char* to, const char* from)
{
    (void)to; (void)from;
    return (iconv_t)1;          /* ダミー */
}

size_t iconv(iconv_t cd,
             char** inbuf,  size_t* inbytesleft,
             char** outbuf, size_t* outbytesleft)
{
    (void)cd; (void)inbuf; (void)inbytesleft;
    (void)outbuf; (void)outbytesleft;
    return 0;                   /* 変換なし */
}

int iconv_close(iconv_t cd)
{
    (void)cd;
    return 0;
}
