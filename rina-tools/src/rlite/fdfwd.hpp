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

#ifndef __FDFWD_HH__
#define __FDFWD_HH__

#define MAX_SESSIONS 16
#define FDFWD_MAX_BUFSZ 16384

#include <list>
#include <thread>
#include <mutex>

using FwdToken = unsigned int;

struct Fd {
    int fd;
    int len;
    int ofs;
    bool closed;
    FwdToken token;
    char data[FDFWD_MAX_BUFSZ];

    Fd(int _fd, FwdToken t) : fd(_fd), len(0), ofs(0), closed(false), token(t)
    {
    }
    Fd() : fd(0), len(0), ofs(0), closed(false), token(0) {}
};

class FwdWorker {
    std::thread th;
    std::mutex lock;
    int repoll_syncfd;
    int idx;

    /* Holds the active mappings between RINA file descriptors and
     * socket file descriptors. */
    struct Fd fds[MAX_SESSIONS * 2];
    int nfds;

    /* List of tokens corresponding to terminated mappings, together
     * with an eventfd file descriptor to notify termination. */
    std::list<unsigned int> terminated;
    int closed_syncfd;

    int verbose;

    void eventfd_write(int fd);
    void eventfd_drain(int fd);
    void terminate(unsigned int i, int ret, int errcode);

public:
    FwdWorker(int idx_, int verb);
    ~FwdWorker();

    void submit(FwdToken token, int cfd, int rfd);
    void run();
    FwdToken get_next_closed();
    int closed_eventfd() const { return closed_syncfd; }
};

#endif /* __FDFWD_HH__ */
