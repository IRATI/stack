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
#include <asm/uaccess.h>

#include "vmpi-iovec.h"


size_t
iovec_to_buf(struct iovec **iov, unsigned int *iovcnt,
             void *to, size_t tolen)
{
        size_t copylen;
        size_t tot  = 0;

        while (*iovcnt && tolen) {
                copylen = (*iov)->iov_len;
                if (tolen < copylen) {
                        copylen = tolen;
                }
                memcpy(to, (*iov)->iov_base, copylen);
                tot += copylen;
                tolen -= copylen;
                to += copylen;
                (*iov)->iov_base += copylen;
                (*iov)->iov_len -= copylen;
                if ((*iov)->iov_len == 0) {
                    (*iovcnt)--;
                    (*iov)++;
                }
        }

        return tot;
}

size_t
iovec_from_buf(struct iovec **iov, unsigned int *iovcnt,
               const void *from, size_t fromlen)
{
        size_t copylen;
        size_t tot  = 0;

        while (*iovcnt && fromlen) {
                copylen = (*iov)->iov_len;
                if (fromlen < copylen) {
                        copylen = fromlen;
                }
                memcpy((*iov)->iov_base, from, copylen);
                tot += copylen;
                fromlen -= copylen;
                from += copylen;
                (*iov)->iov_base += copylen;
                (*iov)->iov_len -= copylen;
                if ((*iov)->iov_len == 0) {
                        (*iovcnt)--;
                        (*iov)++;
                }
        }

        return tot;
}
