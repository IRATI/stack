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

struct Fd {
    int fd;
    int len;
    int ofs;
    char close;
    char data[FDFWD_MAX_BUFSZ];

    Fd(int _fd) : fd(_fd), len(0), ofs(0), close(0) {}
    Fd() : fd(0), len(0), ofs(0), close(0) {}
};

struct FwdWorker {
    pthread_t th;
    pthread_mutex_t lock;
    int syncfd;
    int idx;

    /* Holds the active mappings between RINA file descriptors and
     * socket file descriptors. */
    struct Fd fds[MAX_SESSIONS * 2];
    int nfds;

    int verbose;

    FwdWorker(int idx_, int verb);
    ~FwdWorker();

    int repoll();
    int drain_syncfd();
    void submit(int cfd, int rfd);
    void terminate(unsigned int i, int ret, int errcode);
    void run();
};

#endif /* __FDFWD_HH__ */
