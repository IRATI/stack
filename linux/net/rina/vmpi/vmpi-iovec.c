/*
 * iovec helpers for VMPI
 *
 *    Vincenzo Maffione <v.maffione@nextworks.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/string.h>

#include "vmpi-iovec.h"


size_t
iovec_to_buf(const struct iovec *iov, unsigned int iovcnt,
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

size_t
iovec_from_buf(struct iovec *iov, unsigned int iovcnt,
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
