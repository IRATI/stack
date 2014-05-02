#include <linux/uio.h>


size_t iovec_to_buf(const struct iovec *iov, unsigned int iovcnt,
                    void *to, size_t len);

size_t iovec_from_buf(struct iovec *iov, unsigned int iovcnt,
                      void *from, size_t len);
