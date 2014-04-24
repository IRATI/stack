#include <linux/string.h>

#include "vmpi-iovec.h"


size_t iovec_to_buf(const struct iovec *iov, unsigned int iovcnt,
                    void *to, size_t len)
{
    size_t copylen;
    size_t tot  = 0;

    while (iovcnt && len) {
        copylen = iov->iov_len;
        if (len < copylen) {
            copylen = len;
        }
        memcpy(to, iov->iov_base, copylen);
        tot += copylen;
        len -= copylen;
        to += copylen;
        iovcnt--;
        iov++;
    }

    return tot;
}

size_t iovec_from_buf(struct iovec *iov, unsigned int iovcnt,
                      void *from, size_t len)
{
    size_t copylen;
    size_t tot  = 0;

    while (iovcnt && len) {
        copylen = iov->iov_len;
        if (len < copylen) {
            copylen = len;
        }
        memcpy(iov->iov_base, from, copylen);
        tot += copylen;
        len -= copylen;
        from += copylen;
        iovcnt--;
        iov++;
    }

    return tot;
}
