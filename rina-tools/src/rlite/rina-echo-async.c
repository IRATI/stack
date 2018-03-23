/*
 * Copyright (C) 2015-2016 Nextworks
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <sys/time.h>
#include <assert.h>
#include <endian.h>
#include <signal.h>
#include <poll.h>
#include <time.h>
#include <sys/select.h>

#include <rina/api.h>

#define SDU_SIZE_MAX 64
#define MAX_CLIENTS 128
#define TIMEOUT_SECS 3
#define REGISTER_TIMEOUT_SECS 10
#if TIMEOUT_SECS < 2
#error "TIMEOUT_SECS must be >= 2"
#endif

struct echo_async {
    int cfd;
    const char *cli_appl_name;
    const char *srv_appl_name;
    char *dif_name;
    struct rina_flow_spec flowspec;
    int p; /* parallel clients */
};

#define MAX(a, b) ((a) > (b) ? (a) : (b))

struct fdfsm {
    char buf[SDU_SIZE_MAX];
    int buflen;
#define SELFD_S_ALLOC 1
#define SELFD_S_WRITE 2
#define SELFD_S_READ 3
#define SELFD_S_NONE 4
#define SELFD_S_REGISTER 5
#define SELFD_S_ACCEPT 6
    int state;
    int fd;
    time_t last_activity;
};

#define PRINTF(FMT, ...)                                                       \
    do {                                                                       \
        printf(FMT, ##__VA_ARGS__);                                            \
        fflush(stdout);                                                        \
    } while (0)

static void
shutdown_flow(struct fdfsm *fsm)
{
    close(fsm->fd);
    fsm->state = SELFD_S_NONE;
    fsm->fd    = -1;
}

static int
client(struct echo_async *rea)
{
    const char *msg = "Hello guys, this is a test message!";
    struct timeval to;
    struct fdfsm *fsms;
    fd_set rdfs, wrfs;
    int maxfd;
    int ret;
    int size;
    int i;

    fsms = calloc(rea->p, sizeof(*fsms));
    if (fsms == NULL) {
        PRINTF("Failed to allocate memory for %d parallel flows\n", rea->p);
        return -1;
    }

    /* Start flow allocations in parallel, without waiting for completion. */

    for (i = 0; i < rea->p; i++) {
        fsms[i].state = SELFD_S_ALLOC;
        fsms[i].fd =
            rina_flow_alloc(rea->dif_name, rea->cli_appl_name,
                            rea->srv_appl_name, &rea->flowspec, RINA_F_NOWAIT);
        fsms[i].last_activity = time(NULL);
        if (fsms[i].fd < 0) {
            perror("rina_flow_alloc()");
            return fsms[i].fd;
        }
    }

    for (;;) {
        FD_ZERO(&rdfs);
        FD_ZERO(&wrfs);
        maxfd = 0;

        for (i = 0; i < rea->p; i++) {
            switch (fsms[i].state) {
            case SELFD_S_WRITE:
                FD_SET(fsms[i].fd, &wrfs);
                break;
            case SELFD_S_READ:
            case SELFD_S_ALLOC:
                FD_SET(fsms[i].fd, &rdfs);
                break;
            case SELFD_S_NONE:
                /* Do nothing */
                break;
            }

            maxfd = MAX(maxfd, fsms[i].fd);
        }

        if (maxfd <= 0) {
            /* Nothing more to process. */
            break;
        }

        to.tv_sec  = TIMEOUT_SECS - 1;
        to.tv_usec = 0;

        ret = select(maxfd + 1, &rdfs, &wrfs, NULL, &to);
        if (ret < 0) {
            perror("select()\n");
            return ret;
        } else if (ret == 0) {
            /* Timeout */
        }

        for (i = 0; i < rea->p; i++) {
            switch (fsms[i].state) {
            case SELFD_S_ALLOC:
                if (FD_ISSET(fsms[i].fd, &rdfs)) {
                    fsms[i].last_activity = time(NULL);
                    /* Complete flow allocation, replacing the fd. */
                    fsms[i].fd = rina_flow_alloc_wait(fsms[i].fd);
                    if (fsms[i].fd < 0) {
                        PRINTF("rina_flow_alloc_wait(): flow %d denied\n", i);
                        shutdown_flow(fsms + i);
                        return -1;
                    } else {
                        fsms[i].state = SELFD_S_WRITE;
                        PRINTF("Flow %d allocated\n", i);
                    }
                }
                break;

            case SELFD_S_WRITE:
                if (FD_ISSET(fsms[i].fd, &wrfs)) {
                    fsms[i].last_activity = time(NULL);
                    strncpy(fsms[i].buf, msg, sizeof(fsms[i].buf));
                    size = strlen(fsms[i].buf) + 1;

                    ret = write(fsms[i].fd, fsms[i].buf, size);
                    if (ret == size) {
                        fsms[i].state = SELFD_S_READ;
                    } else {
                        shutdown_flow(fsms + i);
                    }
                }
                break;

            case SELFD_S_READ:
                if (FD_ISSET(fsms[i].fd, &rdfs)) {
                    fsms[i].last_activity = time(NULL);
                    /* Ready to read. */
                    ret = read(fsms[i].fd, fsms[i].buf, sizeof(fsms[i].buf));
                    if (ret > 0) {
                        fsms[i].buf[ret] = '\0';
                        PRINTF("Response: '%s'\n", fsms[i].buf);
                        PRINTF("Flow %d deallocated\n", i);
                    }
                    shutdown_flow(fsms + i);
                }
                break;
            }

            if (time(NULL) - fsms[i].last_activity >= TIMEOUT_SECS) {
                PRINTF("Flow %d timed out\n", i);
                shutdown_flow(fsms + i);
                return -1;
            }
        }
    }

    return 0;
}

static int
server(struct echo_async *rea)
{
    struct fdfsm fsms[MAX_CLIENTS + 1];
    fd_set rdfs, wrfs;
    struct timeval to;
    int maxfd;
    int ret;
    int wfd;
    int i;

    /* In listen mode also register the application names. Here we use
     * two-steps registration (RINA_F_NOWAIT), that will be completed
     * by the event loop. */
    wfd = rina_register(rea->cfd, rea->dif_name, rea->srv_appl_name,
                        RINA_F_NOWAIT);
    if (wfd < 0) {
        perror("rina_register()");
        return wfd;
    }

    fsms[0].state         = SELFD_S_REGISTER;
    fsms[0].fd            = wfd;
    fsms[0].last_activity = time(NULL);

    for (i = 1; i <= MAX_CLIENTS; i++) {
        fsms[i].state         = SELFD_S_NONE;
        fsms[i].fd            = -1;
        fsms[i].last_activity = time(NULL);
    }

    for (;;) {
        FD_ZERO(&rdfs);
        FD_ZERO(&wrfs);
        maxfd = 0;

        for (i = 0; i <= MAX_CLIENTS; i++) {
            switch (fsms[i].state) {
            case SELFD_S_WRITE:
                FD_SET(fsms[i].fd, &wrfs);
                break;
            case SELFD_S_READ:
            case SELFD_S_ACCEPT:
            case SELFD_S_REGISTER:
                FD_SET(fsms[i].fd, &rdfs);
                break;
            case SELFD_S_NONE:
                /* Do nothing */
                break;
            }

            maxfd = MAX(maxfd, fsms[i].fd);
        }

        assert(maxfd >= 0);

        to.tv_sec  = TIMEOUT_SECS - 1;
        to.tv_usec = 0;

        ret = select(maxfd + 1, &rdfs, &wrfs, NULL, &to);
        if (ret < 0) {
            perror("select()\n");
            return ret;
        } else if (ret == 0) {
            /* Timeout */
        }

        for (i = 0; i <= MAX_CLIENTS; i++) {
            time_t inactivity;

            switch (fsms[i].state) {
            case SELFD_S_REGISTER:
                if (FD_ISSET(fsms[i].fd, &rdfs)) {
                    fsms[i].last_activity = time(NULL);
                    ret = rina_register_wait(rea->cfd, fsms[i].fd);
                    if (ret < 0) {
                        perror("rina_register_wait()");
                        return ret;
                    }
                    fsms[i].fd    = rea->cfd;
                    fsms[i].state = SELFD_S_ACCEPT;
                }
                break;

            case SELFD_S_ACCEPT:
                if (FD_ISSET(fsms[i].fd, &rdfs)) {
                    int handle;
                    int j;

                    /* Look for a free slot. */
                    for (j = 1; j <= MAX_CLIENTS; j++) {
                        if (fsms[j].state == SELFD_S_NONE) {
                            break;
                        }
                    }

                    /* Receive flow allocation request without
                     * responding. */
                    handle =
                        rina_flow_accept(fsms[i].fd, NULL, NULL, RINA_F_NORESP);
                    if (handle < 0) {
                        perror("rina_flow_accept()");
                        return handle;
                    }

                    /* Respond positively if we have found a slot. */
                    fsms[j].fd = rina_flow_respond(fsms[i].fd, handle,
                                                   j > MAX_CLIENTS ? -1 : 0);
                    fsms[j].last_activity = time(NULL);
                    if (fsms[j].fd < 0) {
                        perror("rina_flow_respond()");
                        break;
                    }

                    if (j <= MAX_CLIENTS) {
                        fsms[j].state = SELFD_S_READ;
                        PRINTF("Accept client %d\n", j);
                    }
                }
                break;

            case SELFD_S_READ:
                if (FD_ISSET(fsms[i].fd, &rdfs)) {
                    fsms[i].last_activity = time(NULL);
                    /* File descriptor is ready for reading. */
                    fsms[i].buflen =
                        read(fsms[i].fd, fsms[i].buf, sizeof(fsms[i].buf));
                    if (fsms[i].buflen < 0) {
                        shutdown_flow(fsms + i);
                        PRINTF("Shutdown client %d\n", i);
                    } else {
                        fsms[i].buf[fsms[i].buflen] = '\0';
                        PRINTF("Request: '%s'\n", fsms[i].buf);
                        fsms[i].state = SELFD_S_WRITE;
                    }
                }
                break;

            case SELFD_S_WRITE:
                if (FD_ISSET(fsms[i].fd, &wrfs)) {
                    fsms[i].last_activity = time(NULL);
                    ret = write(fsms[i].fd, fsms[i].buf, fsms[i].buflen);
                    if (ret == fsms[i].buflen) {
                        PRINTF("Response sent back\n");
                        PRINTF("Close client %d\n", i);
                    } else {
                        PRINTF("Shutdown client %d\n", i);
                    }
                    shutdown_flow(fsms + i);
                }
            }

            inactivity = time(NULL) - fsms[i].last_activity;
            switch (fsms[i].state) {
            case SELFD_S_READ:
            case SELFD_S_WRITE:
                if (inactivity > TIMEOUT_SECS) {
                    PRINTF("Client %d timed out\n", i);
                    shutdown_flow(fsms + i);
                }
                break;

            case SELFD_S_REGISTER:
                if (inactivity > REGISTER_TIMEOUT_SECS) {
                    PRINTF("Server timed out on rina_register()\n");
                    return -1;
                }
                break;
            }
        }
    }

    return 0;
}

/* Turn this program into a daemon process. */
static void
daemonize(void)
{
    pid_t pid = fork();
    pid_t sid;

    if (pid < 0) {
        perror("fork(daemonize)");
        exit(EXIT_FAILURE);
    }

    if (pid > 0) {
        /* This is the parent. We can terminate it. */
        exit(0);
    }

    /* Execution continues only in the child's context. */
    sid = setsid();
    if (sid < 0) {
        exit(EXIT_FAILURE);
    }

    chdir("/");
}

static void
sigint_handler(int signum)
{
    exit(EXIT_SUCCESS);
}

static void
usage(void)
{
    PRINTF(
        "rina-echo-async [OPTIONS]\n"
        "   -h : show this help\n"
        "   -l : run in server mode (listen)\n"
        "   -d DIF : name of DIF to which register or ask to allocate a flow\n"
        "   -a APNAME : application process name/instance of the echo_async "
        "client\n"
        "   -z APNAME : application process name/instance of the echo_async "
        "server\n"
        "   -g NUM : max SDU gap to use for the data flow\n"
        "   -p NUM : open NUM parallel flows\n"
        "   -w : server runs in background\n");
}

int
main(int argc, char **argv)
{
    struct sigaction sa;
    struct echo_async rea;
    const char *dif_name = NULL;
    int listen           = 0;
    int background       = 0;
    int ret;
    int opt;

    memset(&rea, 0, sizeof(rea));

    rea.cli_appl_name = "rina-echo-async|client";
    rea.srv_appl_name = "rina-echo-async|server";
    rea.p             = 1;

    /* Start with a default flow configuration (unreliable flow). */
    rina_flow_spec_unreliable(&rea.flowspec);

    while ((opt = getopt(argc, argv, "hld:a:z:g:p:w")) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return 0;

        case 'l':
            listen = 1;
            break;

        case 'd':
            dif_name = optarg;
            break;

        case 'a':
            rea.cli_appl_name = optarg;
            break;

        case 'z':
            rea.srv_appl_name = optarg;
            break;

        case 'g': /* Set max_sdu_gap flow specification parameter. */
            rea.flowspec.max_sdu_gap = atoll(optarg);
            break;

        case 'p':
            rea.p = atoll(optarg);
            if (rea.p <= 0) {
                PRINTF("Invalid -p argument '%d'\n", rea.p);
                return -1;
            }
            break;

        case 'w':
            background = 1;
            break;

        default:
            PRINTF("    Unrecognized option %c\n", opt);
            usage();
            return -1;
        }
    }

    /* Set some signal handler */
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    ret         = sigaction(SIGINT, &sa, NULL);
    if (ret) {
        perror("sigaction(SIGINT)");
        return ret;
    }
    ret = sigaction(SIGTERM, &sa, NULL);
    if (ret) {
        perror("sigaction(SIGTERM)");
        return ret;
    }

    /* Initialization of RLITE application. */
    rea.cfd = rina_open();
    if (rea.cfd < 0) {
        perror("rina_open()");
        return rea.cfd;
    }

    rea.dif_name = dif_name ? strdup(dif_name) : NULL;

    if (listen) {
        if (background) {
            daemonize();
        }
        return server(&rea);
    }

    return client(&rea);
}
