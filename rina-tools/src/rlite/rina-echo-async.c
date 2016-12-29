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
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
#include <sys/select.h>

#include <rina/api.h>


#define SDU_SIZE_MAX    64
#define MAX_CLIENTS     128

struct echo_async {
    int cfd;
    const char *cli_appl_name;
    const char *srv_appl_name;
    char *dif_name;
    struct rina_flow_spec flowspec;
    int p; /* parallel clients */
};

#define MAX(a,b) ((a)>(b) ? (a) : (b))

struct fdfsm {
    char buf[SDU_SIZE_MAX];
    int buflen;
#define SELFD_S_ALLOC   1
#define SELFD_S_WRITE   2
#define SELFD_S_READ    3
#define SELFD_S_NONE    4
#define SELFD_S_ACCEPT  5
    int state;
    int fd;
};

static void
shutdown_flow(struct fdfsm *fsm)
{
    close(fsm->fd);
    fsm->state = SELFD_S_NONE;
    fsm->fd = -1;
}

static int
client(struct echo_async *rea)
{
    const char *msg = "Hello guys, this is a test message!";
    struct fdfsm *fsms;
    fd_set rdfs, wrfs;
    int maxfd;
    int ret;
    int size;
    int i;

    fsms = calloc(rea->p, sizeof(*fsms));
    if (fsms == NULL) {
        printf("Failed to allocate memory for %d parallel flows\n", rea->p);
        return -1;
    }

    /* Start flow allocations in parallel, without waiting for completion. */

    for (i = 0; i < rea->p; i ++) {
        fsms[i].state = SELFD_S_ALLOC;
        fsms[i].fd = rina_flow_alloc(rea->dif_name, rea->cli_appl_name,
                                     rea->srv_appl_name, &rea->flowspec,
                                     RINA_F_NOWAIT);
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

        ret = select(maxfd + 1, &rdfs, &wrfs, NULL, NULL);
        if (ret < 0) {
            perror("select()\n");
            return ret;
        } else if (ret == 0) {
            /* Timeout */
            printf("Timeout occurred\n");
            break;
        }

        for (i = 0; i < rea->p; i++) {
            switch (fsms[i].state) {
            case SELFD_S_ALLOC:
                if (FD_ISSET(fsms[i].fd, &rdfs)) {
                    /* Complete flow allocation, replacing the fd. */
                    fsms[i].fd = rina_flow_alloc_wait(fsms[i].fd);
                    if (fsms[i].fd < 0) {
                        printf("rina_flow_alloc_wait(): flow %d denied\n", i);
                        shutdown_flow(fsms + i);
                    } else {
                        fsms[i].state = SELFD_S_WRITE;
                        printf("Flow %d allocated\n", i);
                    }
                }
                break;

            case SELFD_S_WRITE:
                if (FD_ISSET(fsms[i].fd, &wrfs)) {
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
                    /* Ready to read. */
                    ret = read(fsms[i].fd, fsms[i].buf, sizeof(fsms[i].buf));
                    if (ret > 0) {
                        fsms[i].buf[ret] = '\0';
                        printf("Response: '%s'\n", fsms[i].buf);
                        printf("Flow %d deallocated\n", i);
                    }
                    shutdown_flow(fsms + i);
                }
                break;
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
    int maxfd;
    int ret;
    int i;

    /* In listen mode also register the application names. */
    ret = rina_register(rea->cfd, rea->dif_name, rea->srv_appl_name);
    if (ret) {
        perror("rina_register()");
        return ret;
    }

    fsms[0].state = SELFD_S_ACCEPT;
    fsms[0].fd = rea->cfd;

    for (i = 1; i <= MAX_CLIENTS; i++) {
        fsms[i].state = SELFD_S_NONE;
        fsms[i].fd = -1;
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
                FD_SET(fsms[i].fd, &rdfs);
                break;
            case SELFD_S_NONE:
                /* Do nothing */
                break;
            }

            maxfd = MAX(maxfd, fsms[i].fd);
        }

        assert(maxfd >= 0);

        ret = select(maxfd + 1, &rdfs, &wrfs, NULL, NULL);
        if (ret < 0) {
            perror("select()\n");
            return ret;
        } else if (ret == 0) {
            /* Timeout */
            printf("Timeout occurred\n");
            break;
        }

        for (i = 0; i <= MAX_CLIENTS; i++) {
            switch (fsms[i].state) {
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
                    handle = rina_flow_accept(fsms[i].fd, NULL, NULL,
                                              RINA_F_NORESP);
                    if (handle < 0) {
                        perror("rina_flow_accept()");
                        return handle;
                    }

                    /* Respond positively if we have found a slot. */
                    fsms[j].fd = rina_flow_respond(fsms[i].fd, handle,
                                                   j > MAX_CLIENTS ? -1 : 0);
                    if (fsms[j].fd < 0) {
                        perror("rina_flow_respond()");
                        return fsms[j].fd;
                    }

                    if (j <= MAX_CLIENTS) {
                        fsms[j].state = SELFD_S_READ;
                        printf("Accept client %d\n", j);
                    }
                }
                break;

            case SELFD_S_READ:
                if (FD_ISSET(fsms[i].fd, &rdfs)) {
                    /* File descriptor is ready for reading. */
                    fsms[i].buflen = read(fsms[i].fd, fsms[i].buf,
                                          sizeof(fsms[i].buf));
                    if (fsms[i].buflen < 0) {
                        shutdown_flow(fsms + i);
                        printf("Shutdown client %d\n", i);
                    } else {
                        fsms[i].buf[fsms[i].buflen] = '\0';
                        printf("Request: '%s'\n", fsms[i].buf);
                        fsms[i].state = SELFD_S_WRITE;
                    }
                }
                break;

            case SELFD_S_WRITE:
                if (FD_ISSET(fsms[i].fd, &wrfs)) {
                    ret = write(fsms[i].fd, fsms[i].buf, fsms[i].buflen);
                    if (ret == fsms[i].buflen) {
                        printf("Response sent back\n");
                        printf("Close client %d\n", i);
                    } else {
                        printf("Shutdown client %d\n", i);
                    }
                    shutdown_flow(fsms + i);
                }
            }
        }
    }

    return 0;
}

static void
sigint_handler(int signum)
{
    exit(EXIT_SUCCESS);
}

static void
usage(void)
{
    printf("rina-echo-async [OPTIONS]\n"
        "   -h : show this help\n"
        "   -l : run in server mode (listen)\n"
        "   -d DIF : name of DIF to which register or ask to allocate a flow\n"
        "   -a APNAME : application process name/instance of the echo_async client\n"
        "   -z APNAME : application process name/instance of the echo_async server\n"
        "   -g NUM : max SDU gap to use for the data flow\n"
        "   -p NUM : open NUM parallel flows\n"
          );
}

int
main(int argc, char **argv)
{
    struct sigaction sa;
    struct echo_async rea;
    const char *dif_name = NULL;
    int listen = 0;
    int ret;
    int opt;

    memset(&rea, 0, sizeof(rea));

    rea.cli_appl_name = "rina-echo-async:client";
    rea.srv_appl_name = "rina-echo-async:server";
    rea.p = 1;

    /* Start with a default flow configuration (unreliable flow). */
    rina_flow_spec_default(&rea.flowspec);

    while ((opt = getopt(argc, argv, "hld:a:z:g:p:")) != -1) {
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
                    printf("Invalid -p argument '%d'\n", rea.p);
                    return -1;
                }
                break;

            default:
                printf("    Unrecognized option %c\n", opt);
                usage();
                return -1;
        }
    }

    /* Set some signal handler */
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    ret = sigaction(SIGINT, &sa, NULL);
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
        server(&rea);

    } else {
        client(&rea);
    }

    return close(rea.cfd);
}
