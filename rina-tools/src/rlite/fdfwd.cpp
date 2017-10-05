/*
 * Copyright (C) 2015-2017 Nextworks
 * Author: Vincenzo Maffione <v.maffione@gmail.com>
 *
 * This file is part of rlite.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <map>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <cassert>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <poll.h>
#include <signal.h>
#include <fcntl.h>

#include "fdfwd.hpp"

using namespace std;

static void *
worker_function(void *opaque)
{
    FwdWorker *w = (FwdWorker *)opaque;

    w->run();

    return NULL;
}

FwdWorker::FwdWorker(int idx_, int verb) : idx(idx_), nfds(0), verbose(verb)
{
    syncfd = eventfd(0, 0);
    if (syncfd < 0) {
        perror("eventfd()");
        exit(EXIT_FAILURE);
    }
    pthread_create(&th, NULL, worker_function, this);
    pthread_mutex_init(&lock, NULL);
}

FwdWorker::~FwdWorker()
{
    if (pthread_join(th, NULL)) {
        perror("pthread_join");
    }
    close(syncfd);
}

int
FwdWorker::repoll()
{
    uint64_t x = 1;
    int n;

    n = write(syncfd, &x, sizeof(x));
    if (n != sizeof(x)) {
        perror("write(syncfd)");
        if (n < 0) {
            return n;
        }
        return -1;
    }

    return 0;
}

int
FwdWorker::drain_syncfd()
{
    uint64_t x;
    int n;

    n = read(syncfd, &x, sizeof(x));
    if (n != sizeof(x)) {
        perror("read(syncfd)");
        if (n < 0) {
            return n;
        }
        return -1;
    }

    return 0;
}

void
FwdWorker::submit(int cfd, int rfd)
{
    pthread_mutex_lock(&lock);

    if (nfds >= 2 * MAX_SESSIONS) {
        pthread_mutex_unlock(&lock);
        printf("too many sessions, shutting down %d <--> %d\n", cfd, rfd);
        close(cfd);
        close(rfd);
        return;
    }

    /* Append two entries in the fds array. Mapping is implicit
     * between two consecutive entries. */
    fds[nfds++] = Fd(rfd);
    fds[nfds++] = Fd(cfd);
    repoll();
    pthread_mutex_unlock(&lock);

    if (verbose >= 1) {
        printf("New mapping created %d <--> %d [entries=%d,%d]\n", cfd, rfd,
               nfds - 2, nfds - 1);
    }
}

/* Called under worker lock. */
void
FwdWorker::terminate(unsigned int i, int ret, int errcode)
{
    unsigned j;
    string how;

    i &= ~(0x1);
    j = i | 0x1;

    close(fds[i].fd);
    close(fds[j].fd);
    fds[i].close = fds[j].close = 1;

    if (verbose >= 1) {
        if (ret == 0 || errcode == EPIPE) {
            how = "normally";
        } else {
            how = "with errors";
        }

        cout << "w" << idx << ": Session " << fds[i].fd << " <--> " << fds[j].fd
             << " closed " << how << " [entries=" << i << "," << j << "]"
             << endl;
    }

    /* fds entries are recovered at the beginning of the run() main loop */
}

void
FwdWorker::run()
{
    struct pollfd pollfds[1 + MAX_SESSIONS * 2];
    struct pollfd *pfds = pollfds + 1;

    if (verbose >= 1) {
        printf("w%d starts\n", idx);
    }

    for (;;) {
        int nrdy;

        pfds[-1].fd     = syncfd;
        pfds[-1].events = POLLIN;

        /* Load the poll array with the active fd mappings, also recycling
         * the entries of closed sessions. */
        pthread_mutex_lock(&lock);
        for (int i = 0; i < nfds;) {
            int j = i + 1; /* index of the mapped entry */

            if (fds[i].close) {
                nfds -= 2;
                if (i < nfds) {
                    fds[i] = fds[nfds - 1];
                    fds[j] = fds[nfds];
                    if (verbose >= 1) {
                        printf("Recycled entries %d,%d\n", i, j);
                    }
                }
                continue;
            }

            pfds[i].fd     = fds[i].fd;
            pfds[j].fd     = fds[j].fd;
            pfds[i].events = pfds[j].events = 0;

            /* If there is pending data in the output buffer for entry i,
             * request POLLOUT to flush that data. Otherwise request POLLIN
             * on the mapped entry, but only if the mapped entry does not
             * need to flush its own output buffer. */
            if (fds[i].len) {
                pfds[i].events = POLLOUT;
            } else if (!fds[j].len) {
                pfds[j].events = POLLIN;
            }

            /* Same logic for the mapped entry. */
            if (fds[j].len) {
                pfds[j].events = POLLOUT;
            } else if (!fds[i].len) {
                pfds[i].events = POLLIN;
            }

            i += 2;
        }
        pthread_mutex_unlock(&lock);

        if (verbose >= 2) {
            printf("w%d polls %d file descriptors\n", idx, nfds + 1);
        }
        nrdy = poll(pollfds, nfds + 1, -1);
        if (nrdy < 0) {
            perror("poll()");
            break;

        } else if (nrdy == 0) {
            printf("w%d: poll() timeout\n", idx);
            continue;
        }

        if (pfds[-1].revents) {
            /* We've been requested to repoll the queue. */
            nrdy--;
            if (pfds[-1].revents & POLLIN) {
                if (verbose >= 2) {
                    printf("w%d: Mappings changed, rebuilding poll array\n",
                           idx);
                }
                pthread_mutex_lock(&lock);
                drain_syncfd();
                pthread_mutex_unlock(&lock);

            } else {
                printf("w%d: Error event %d on syncfd\n", idx,
                       pfds[-1].revents);
            }

            continue;
        }

        pthread_mutex_lock(&lock);

        for (int i = 0, n = 0; n < nrdy && i < nfds; i++) {
            int j;

            if (!pfds[i].revents || fds[i].close) {
                /* No events on this fd, or the session has
                 * been terminated by the mapped fd (in a previous
                 * iteration of this loop). Let's skip it. */
                continue;
            }

            if (verbose >= 2) {
                printf("w%d: fd %d ready, events %d\n", idx, pfds[i].fd,
                       pfds[i].revents);
            }

            /* Consume the events on this fd. */
            n++;

            j = i ^ 0x1; /* index to the mapped fds entry */

            if (pfds[i].revents & POLLIN) {
                int m;

                assert(fds[j].len == 0);
                /* The output buffer for entry j is empty and, there
                 * is data to read from the mapped entry i. Load the
                 * output buffer with this data. */
                m = read(fds[i].fd, fds[j].data, FDFWD_MAX_BUFSZ);
                if (m <= 0) {
                    terminate(i, m, errno);
                } else {
                    fds[j].len = m;
                    fds[j].ofs = 0;
                }

            } else if (pfds[i].revents & POLLOUT) {
                int m;

                assert(fds[i].len > 0);
                /* There is data in the output buffer of entry i. Try to
                 * flush it. */
                m = write(fds[i].fd, fds[i].data + fds[i].ofs, fds[i].len);
                if (m <= 0) {
                    terminate(i, m, errno);
                } else {
                    fds[i].ofs += m;
                    fds[i].len -= m;
                    if (verbose >= 2) {
                        printf("Forwarded %d bytes %d --> %d\n", m, fds[j].fd,
                               fds[i].fd);
                    }
                }
            }
        }

        pthread_mutex_unlock(&lock);
    }

    if (verbose >= 1) {
        printf("w%d stops\n", idx);
    }
}
