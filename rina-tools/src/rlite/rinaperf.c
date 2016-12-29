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
#include <time.h>

#include <rina/api.h>


#define SDU_SIZE_MAX    65535

static int stop = 0; /* Used to stop client on SIGINT. */
static int cli_flow_allocated = 0; /* Avoid to get stuck in rina_flow_alloc(). */

struct rinaperf_test_config {
    uint32_t ty;
    uint32_t size;
    uint32_t cnt;
};

struct rinaperf {
    int cfd;
    const char *cli_appl_name;
    const char *srv_appl_name;
    int dfd;

    unsigned int interval;
    unsigned int burst;
    int ping;

    struct rinaperf_test_config test_config;
};

typedef int (*perf_function_t)(struct rinaperf *);

static int
client_test_config(struct rinaperf *rp)
{
    struct rinaperf_test_config cfg = rp->test_config;
    int ret;

    cfg.ty = htole32(cfg.ty);
    cfg.cnt = htole32(cfg.cnt);
    cfg.size = htole32(cfg.size);

    ret = write(rp->dfd, &cfg, sizeof(cfg));
    if (ret != sizeof(cfg)) {
        if (ret < 0) {
            perror("write(buf)");
        } else {
            printf("Partial write %d/%lu\n", ret,
                    (unsigned long int)sizeof(cfg));
        }
        return -1;
    }

    return 0;
}

static int
server_test_config(struct rinaperf *rp)
{
    struct rinaperf_test_config cfg;
    struct timeval to;
    fd_set rfds;
    int ret;

    FD_ZERO(&rfds);
    FD_SET(rp->dfd, &rfds);
    to.tv_sec = 3;
    to.tv_usec = 0;

    ret = select(rp->dfd + 1, &rfds, NULL, NULL, &to);
    switch (ret) {
        case -1:
            perror("select()");
            return ret;

        case 0:
            printf("timeout while waiting for server test configuration\n");
            return -1;

        default:
            break;
    }

    ret = read(rp->dfd, &cfg, sizeof(cfg));
    if (ret != sizeof(cfg)) {
        if (ret < 0) {
            perror("read(buf");
        } else {
            printf("Error reading test configuration: wrong length %d "
                   "(should be %lu)\n", ret, (unsigned long int)sizeof(cfg));
        }
        return -1;
    }

    cfg.ty = le32toh(cfg.ty);
    cfg.cnt = le32toh(cfg.cnt);
    cfg.size = le32toh(cfg.size);

    if (cfg.size < sizeof(uint16_t)) {
        printf("Invalid test configuration: size %d is invalid\n", cfg.size);
        return -1;
    }

    printf("Configuring test type %u, SDU count %u, SDU size %u\n",
           cfg.ty, cfg.cnt, cfg.size);

    rp->test_config = cfg;

    return 0;
}

/* Used for both ping and rr tests. */
static int
ping_client(struct rinaperf *rp)
{
    unsigned int limit = rp->test_config.cnt;
    struct timeval t_start, t_end, t1, t2;
    unsigned int interval = rp->interval;
    int size = rp->test_config.size;
    char buf[SDU_SIZE_MAX];
    volatile uint16_t *seqnum = (uint16_t *)buf;
    int verb = rp->ping;
    unsigned int i = 0;
    unsigned long us;
    struct pollfd pfd;
    int ret = 0;

    if (size > sizeof(buf)) {
        printf("Warning: size truncated to %u\n", (unsigned int)sizeof(buf));
        size = sizeof(buf);
    }

    if (!verb) {
        printf("Starting request-response test; message size: %d, "
               " number of messages: ", size);
        if (limit) {
            printf("%d\n", limit);
        } else {
            printf("inf\n");
        }
    }

    pfd.fd = rp->dfd;
    pfd.events = POLLIN;

    memset(buf, 'x', size);

    gettimeofday(&t_start, NULL);

    for (i = 0; !stop && (!limit || i < limit); i++) {
        if (verb) {
            gettimeofday(&t1, NULL);
        }

        *seqnum = (uint16_t)i;

        ret = write(rp->dfd, buf, size);
        if (ret != size) {
            if (ret < 0) {
                perror("write(buf)");
            } else {
                printf("Partial write %d/%d\n", ret, size);
            }
            break;
        }
repoll:
        ret = poll(&pfd, 1, 2000);
        if (ret < 0) {
            perror("poll(flow)");
        }

        if (ret == 0) {
            printf("Timeout: %d bytes lost\n", size);
        } else {
            /* Ready to read. */
            ret = read(rp->dfd, buf, sizeof(buf));
            if (ret <= 0) {
                if (ret) {
                    perror("read(buf");
                }
                break;
            }

            if (verb) {
                if (*seqnum == i) {
                    gettimeofday(&t2, NULL);
                    us = 1000000 * (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec);
                    printf("%d bytes from server: rtt = %.3f ms\n", ret,
                           ((float)us)/1000.0);
                } else {
                    printf("Packet lost or out of order: got %u, "
                           "expected %u\n", *seqnum, i);
                    if (*seqnum < i) {
                        goto repoll;
                    }
                }
            }
        }

        if (interval) {
            usleep(interval);
        }
    }

    gettimeofday(&t_end, NULL);
    us = 1000000 * (t_end.tv_sec - t_start.tv_sec) +
            (t_end.tv_usec - t_start.tv_usec);

    if (!verb && i) {
        printf("SDU size: %d bytes, avg latency: %lu us\n", ret,
                (us/i) - interval);
    }

    close(rp->dfd);

    return 0;
}

/* Used for both ping and rr tests. */
static int
ping_server(struct rinaperf *rp)
{
    unsigned int limit = rp->test_config.cnt;
    char buf[SDU_SIZE_MAX];
    struct pollfd pfd;
    unsigned int i;
    int n, ret;

    pfd.fd = rp->dfd;
    pfd.events = POLLIN;

    for (i = 0; !limit || i < limit; i++) {
        n = poll(&pfd, 1, 4000);
        if (n < 0) {
            perror("poll(flow)");
        } else if (n == 0) {
            /* Timeout */
            printf("timeout occurred\n");
            break;
        }

        /* File descriptor is ready for reading. */
        n = read(rp->dfd, buf, sizeof(buf));
        if (n < 0) {
            perror("read(flow)");
            return -1;
        } else if (n == 0) {
            printf("Flow deallocated remotely\n");
            break;
        }

        ret = write(rp->dfd, buf, n);
        if (ret != n) {
            if (ret < 0) {
                perror("write(flow)");
            } else {
                printf("Partial write");
            }
            return -1;
        }
    }

    printf("received %u PDUs out of %u\n", i, limit);

    return 0;
}

static int
perf_client(struct rinaperf *rp)
{
    unsigned limit = rp->test_config.cnt;
    int size = rp->test_config.size;
    unsigned int interval = rp->interval;
    unsigned int burst = rp->burst;
    unsigned int cdown = burst;
    struct timeval t_start, t_end;
    struct timeval w1, w2;
    char buf[SDU_SIZE_MAX];
    unsigned long us;
    unsigned int i = 0;
    int ret;

    if (size > sizeof(buf)) {
        printf("Warning: size truncated to %u\n", (unsigned int)sizeof(buf));
        size = sizeof(buf);
    }

    printf("Starting unidirectional throughput test; message size: %d, "
            " number of messages: ", size);
    if (limit) {
        printf("%d\n", limit);
    } else {
        printf("inf\n");
    }

    memset(buf, 'x', size);

    gettimeofday(&t_start, NULL);

    for (i = 0; !stop && (!limit || i < limit); i++) {
        ret = write(rp->dfd, buf, size);
        if (ret != size) {
            if (ret < 0) {
                perror("write(buf)");
            } else {
                printf("Partial write %d/%d\n", ret, size);
            }
            break;
        }

        if (interval && --cdown == 0) {
            if (interval > 50) { /* slack default is 50 us*/
                usleep(interval);
            } else {
                gettimeofday(&w1, NULL);
                for (;;) {
                    gettimeofday(&w2, NULL);
                    us = 1000000 * (w2.tv_sec - w1.tv_sec) +
                        (w2.tv_usec - w1.tv_usec);
                    if (us >= interval) {
                        break;
                    }
                }
            }
            cdown = burst;
        }
    }

    gettimeofday(&t_end, NULL);
    us = 1000000 * (t_end.tv_sec - t_start.tv_sec) +
            (t_end.tv_usec - t_start.tv_usec);

    if (us) {
        printf("Throughput: %.3f Kpps, %.3f Mbps\n",
                ((float)i) * 1000.0 / us,
                ((float)size) * 8 * i / us);
    }

    close(rp->dfd);

    return 0;
}

static void
rate_print(unsigned long long *bytes, unsigned long long *cnt,
            unsigned long long *bytes_limit, struct timespec *ts)
{
    struct timespec now;
    unsigned long long elapsed_ns;
    double kpps;
    double mbps;

    clock_gettime(CLOCK_MONOTONIC, &now);

    elapsed_ns = ((now.tv_sec - ts->tv_sec) * 1000000000 +
                    now.tv_nsec - ts->tv_nsec);

    kpps = ((1000000) * (double)*cnt) / elapsed_ns;
    mbps = ((8 * 1000) * (double)*bytes) / elapsed_ns;

    /* We don't want to prints which are too close. */
    if (elapsed_ns > 500000000U) {
        printf("rate: %f Kpss, %f Mbps\n", kpps, mbps);
    }

    if (elapsed_ns < 1000000000U) {
            *bytes_limit *= 2;
    } else if (elapsed_ns > 3 * 1000000000U && *bytes >= 1000) {
            *bytes_limit /= 2;
    }

    if (*bytes >= 1000) {
        clock_gettime(CLOCK_MONOTONIC, ts);
        *cnt = 0;
        *bytes = 0;
    }
}

static int
perf_server(struct rinaperf *rp)
{
    unsigned limit = rp->test_config.cnt;
    unsigned long long rate_cnt = 0;
    unsigned long long rate_bytes_limit = 1000;
    unsigned long long rate_bytes = 0;
    struct timespec rate_ts;
    char buf[SDU_SIZE_MAX];
    struct pollfd pfd;
    unsigned int i;
    int n;

    pfd.fd = rp->dfd;
    pfd.events = POLLIN;

    clock_gettime(CLOCK_MONOTONIC, &rate_ts);

    for (i = 0; !limit || i < limit; i++) {
        n = poll(&pfd, 1, 3000);
        if (n < 0) {
            perror("poll(flow)");

        } else if (n == 0) {
            /* Timeout */
            printf("Timeout occurred\n");
            break;
        }

        /* Ready to read. */
        n = read(rp->dfd, buf, sizeof(buf));
        if (n < 0) {
            perror("read(flow)");
            return -1;

        } else if (n == 0) {
            printf("Flow deallocated remotely\n");
            break;
        }

        rate_bytes += n;
        rate_cnt++;

        if (rate_bytes >= rate_bytes_limit) {
            rate_print(&rate_bytes, &rate_cnt, &rate_bytes_limit, &rate_ts);
        }
    }

    printf("Received %u PDUs out of %u\n", i, limit);

    return 0;
}

struct perf_function_desc {
    const char *name;
    perf_function_t client_function;
    perf_function_t server_function;
};

static struct perf_function_desc descs[] = {
    {
        .name = "ping",
        .client_function = ping_client,
        .server_function = ping_server,
    },
    {
        .name = "rr",
        .client_function = ping_client,
        .server_function = ping_server,
    },
    {   .name = "perf",
        .client_function = perf_client,
        .server_function = perf_server,
    },
};

static int
server(struct rinaperf *rp)
{
    for (;;) {
        perf_function_t perf_function = NULL;
        int ret;

        rp->dfd = rina_flow_accept(rp->cfd, NULL, NULL, 0);
        if (rp->dfd < 0) {
            perror("rina_flow_accept()");
            break;
        }

        ret = server_test_config(rp);
        if (ret) {
            goto clos;
        }

        if (rp->test_config.ty >= sizeof(descs)) {
            continue;
        }
        perf_function = descs[rp->test_config.ty].server_function;
        assert(perf_function);

        perf_function(rp);
clos:
        close(rp->dfd);
    }

    return 0;
}

static void
sigint_handler_client(int signum)
{
    if (!cli_flow_allocated) {
        exit(EXIT_SUCCESS);
    }
    stop = 1;
}

static void
sigint_handler_server(int signum)
{
    exit(EXIT_SUCCESS);
}

static void
parse_bandwidth(struct rina_flow_spec *spec, const char *arg)
{
    size_t arglen = strlen(arg);

    if (arglen < 2) {
        goto err;
    }

    spec->avg_bandwidth = 1;
    switch (arg[arglen-1]) {
        case 'G':
            spec->avg_bandwidth *= 1000;
        case 'M':
            spec->avg_bandwidth *= 1000;
        case 'K':
            spec->avg_bandwidth *= 1000;
            break;
        default:
            if (arg[arglen-1] < '0' || arg[arglen-1] > '9') {
                goto err;
            }
            break;
    }

    spec->avg_bandwidth *= strtoul(arg, NULL, 10);
    printf("Parsed bandwidth %llu\n", (long long unsigned)spec->avg_bandwidth);

    return;
err:
    printf("Invalid bandwidth format '%s'\n", arg);
}

static void
usage(void)
{
    printf("rinaperf [OPTIONS]\n"
        "   -h : show this help\n"
        "   -l : run in server mode (listen)\n"
        "   -t TEST : specify the type of the test to be performed "
            "(ping, perf, rr)\n"
        "   -d DIF : name of DIF to which register or ask to allocate a flow\n"
        "   -c NUM : number of SDUs to send during the test\n"
        "   -s NUM : size of the SDUs that are sent during the test\n"
        "   -i NUM : number of microseconds to wait after each SDUs is sent\n"
        "   -g NUM : max SDU gap to use for the data flow\n"
        "   -B NUM : average bandwitdh for the data flow, in bits per second\n"
        "   -f : enable flow control\n"
        "   -b NUM : How many SDUs to send before waiting as "
                "specified by -i option (default b=1)\n"
        "   -a APNAME : application process name and instance of the rinaperf client\n"
        "   -z APNAME : application process name and instance of the rinaperf server\n"
        "   -x : use a separate control connection\n"
          );
}

int
main(int argc, char **argv)
{
    struct sigaction sa;
    struct rinaperf rp;
    const char *type = "ping";
    const char *dif_name = NULL;
    perf_function_t perf_function = NULL;
    struct rina_flow_spec flowspec;
    int interval_specified = 0;
    int listen = 0;
    int cnt = 0;
    int size = sizeof(uint16_t);
    int interval = 0;
    int burst = 1;
    int have_ctrl = 0;
    int ret;
    int opt;
    int i;

    rp.cli_appl_name = "rinaperf-data:client";
    rp.srv_appl_name = "rinaperf-data:server";

    /* Start with a default flow configuration (unreliable flow). */
    rina_flow_spec_default(&flowspec);

    while ((opt = getopt(argc, argv, "hlt:d:c:s:i:B:g:fb:a:z:x")) != -1) {
        switch (opt) {
            case 'h':
                usage();
                return 0;

            case 'l':
                listen = 1;
                break;

            case 't':
                type = optarg;
                break;

            case 'd':
                dif_name = optarg;
                break;

            case 'c':
                cnt = atoi(optarg);
                if (cnt < 0) {
                    printf("    Invalid 'cnt' %d\n", cnt);
                    return -1;
                }
                break;

            case 's':
                size = atoi(optarg);
                if (size < sizeof(uint16_t)) {
                    printf("    Invalid 'size' %d\n", size);
                    return -1;
                }
                break;

            case 'i':
                interval = atoi(optarg);
                if (interval < 0) {
                    printf("    Invalid 'interval' %d\n", interval);
                    return -1;
                }
                interval_specified = 1;
                break;

            case 'g': /* Set max_sdu_gap flow specification parameter. */
                flowspec.max_sdu_gap = atoll(optarg);
                break;

            case 'B': /* Set the average bandwidth parameter. */
                parse_bandwidth(&flowspec, optarg);
                break;

            case 'f': /* Enable flow control. */
                break;

            case 'b':
                burst = atoi(optarg);
                if (burst <= 0) {
                    printf("    Invalid 'burst' %d\n", burst);
                    return -1;
                }
                break;

            case 'a':
                rp.cli_appl_name = optarg;
                break;

            case 'z':
                rp.srv_appl_name = optarg;
                break;

            case 'x':
                have_ctrl = 1;
                printf("Warning: Control connection support is incomplete\n");
                break;

            default:
                printf("    Unrecognized option %c\n", opt);
                usage();
                return -1;
        }
    }

    /*
     * Fixups:
     *   - Use 1 second interval for ping tests, if the user did not
     *     specify the interval explicitly.
     *   - Set rp.ping variable to distinguish between ping and rr tests,
     *     which share the same functions.
     */
    if (strcmp(type, "ping") == 0) {
        if (!interval_specified) {
            interval = 1000000;
        }
        rp.ping = 1;

    } else if (strcmp(type, "rr") == 0) {
        rp.ping = 0;
    }

    /* Set defaults. */
    rp.interval = interval;
    rp.burst = burst;

    /* Function selection. */
    if (!listen) {
        for (i = 0; i < sizeof(descs)/sizeof(descs[0]); i++) {
            if (strcmp(descs[i].name, type) == 0) {
                perf_function = descs[i].client_function;
                break;
            }
        }

        if (perf_function == NULL) {
            printf("    Unknown test type '%s'\n", type);
            usage();
            return -1;
        }
        rp.test_config.ty = i;
        rp.test_config.cnt = cnt;
        rp.test_config.size = size;
    }

    /* Set some signal handler */
    sa.sa_handler = listen ? sigint_handler_server : sigint_handler_client;
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

    /* Open control file descriptor. */
    rp.cfd = rina_open();
    if (rp.cfd < 0) {
        perror("rina_open()");
        return rp.cfd;
    }

    if (listen) {
        /* Server-side initializations. */

        /* In listen mode also register the application names. */
        if (have_ctrl) {
            ret = rina_register(rp.cfd, dif_name, "rinaperf-ctrl:server");
            if (ret) {
                perror("rina_register()");
                return ret;
            }
        }

        ret = rina_register(rp.cfd, dif_name, rp.srv_appl_name);
        if (ret) {
            perror("rina_register()");
            return ret;
        }

        server(&rp);

    } else {
        /* We're the client: allocate a flow and run the perf function. */
        rp.dfd = rina_flow_alloc(dif_name, rp.cli_appl_name,
                               rp.srv_appl_name, &flowspec, 0);
        cli_flow_allocated = 1;
        if (rp.dfd < 0) {
            perror("rina_flow_alloc()");
            return rp.dfd;
        }

        ret = client_test_config(&rp);
        if (ret) {
            return ret;
        }

        perf_function(&rp);
    }

    return close(rp.cfd);
}
