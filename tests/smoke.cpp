#include <iconv.h>
#include <cassert>

int main()
{
    iconv_t cd = iconv_open("UTF-8", "UTF-8");
    assert(cd != (iconv_t)-1);
    iconv_close(cd);
    return 0;
}
