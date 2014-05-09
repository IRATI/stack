/* Test program for the VMPI interface on the guest (VM)
 *
 * Copyright 2014 Vincenzo Maffione <v.maffione@nextworks.it> Nextworks
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/poll.h>
#include <time.h>


#ifdef VERBOSE
#define IFV(x) x
#else   /* VERVOSE */
#define IFV(x)
#endif  /* VERBOSE */

static __inline struct timespec
timespec_add(struct timespec a, struct timespec b)
{
	struct timespec ret = { a.tv_sec + b.tv_sec, a.tv_nsec + b.tv_nsec };
	if (ret.tv_nsec >= 1000000000) {
		ret.tv_sec++;
		ret.tv_nsec -= 1000000000;
	}
	return ret;
}

static __inline struct timespec
timespec_sub(struct timespec a, struct timespec b)
{
	struct timespec ret = { a.tv_sec - b.tv_sec, a.tv_nsec - b.tv_nsec };
	if (ret.tv_nsec < 0) {
		ret.tv_sec--;
		ret.tv_nsec += 1000000000;
	}
	return ret;
}

/*
 * wait until ts, either busy or sleeping if more than 1ms.
 * Return wakeup time.
 */
static struct timespec
wait_time(struct timespec ts)
{
	for (;;) {
		struct timespec w, cur;
		clock_gettime(CLOCK_MONOTONIC, &cur);
		w = timespec_sub(ts, cur);
		if (w.tv_sec < 0)
			return cur;
		else if (w.tv_sec > 0 || w.tv_nsec > 1000000)
			poll(NULL, 0, 1);
	}
}

void rate_print(unsigned long long *limit, struct timespec *ts, unsigned int bytes)
{
    struct timespec now;
    unsigned long long elapsed_ns;
    double kpps;
    double mbps;

    clock_gettime(CLOCK_MONOTONIC, &now);

    elapsed_ns = ((now.tv_sec - ts->tv_sec) * 1000000000 +
            now.tv_nsec - ts->tv_nsec);
    kpps = ((1000000) * (double)*limit) / elapsed_ns;
    mbps = ((8 * 1000) * (double)bytes) / elapsed_ns;
    printf("rate: %f Kpss, %f Mbps\n", kpps, mbps);
    if (elapsed_ns < 1000000000U) {
        *limit *= 2;
    } else if (elapsed_ns > 3 * 1000000000U) {
        *limit /= 2;
    }
    clock_gettime(CLOCK_MONOTONIC, ts);
}


static const char *devname = "/dev/vmpi-test";
static const char *text = "Ciao dal guest!";
#define MODE_WRITE  0
#define MODE_READ   1

#define BUF_SIZE    2000
static char buffer[BUF_SIZE];

int main(int argc, char **argv)
{
    unsigned long int i;
    int fd;
    int len;
    int n;
    int mode = MODE_WRITE;
    char ch;
    unsigned int rate = 0;
    unsigned long int iterations = 0;
    unsigned int msglen = strlen(text) + 1;
    struct timespec period_ts;
    struct timespec next_ts;
    struct timespec rate_ts;
    unsigned long long rate_cnt = 0;
    unsigned long long rate_cnt_limit = 10;
    unsigned long long rate_bytes = 0;

    while ((ch = getopt(argc, argv, "n:f:R:l:")) != -1) {
        switch (ch) {
            default:
                printf("bad option %c %s\n", ch, optarg);
                return -1;
                break;

            case 'f':
                if (!strcmp(optarg, "r")) {
                    mode = MODE_READ;
                } else if (!strcmp(optarg, "w")) {
                    mode = MODE_WRITE;
                } else {
                    mode = MODE_WRITE;
                }
                break;

            case 'n':
                iterations = atol(optarg);
                if (iterations < 1) {
                    iterations = 1;
                }
                break;

            case 'R':
                rate = atoi(optarg);
                if (rate < 0) {
                    rate = 0;
                }
                break;

            case 'l':
                msglen = atoi(optarg);
                if (msglen > BUF_SIZE) {
                    msglen = BUF_SIZE;
                } else if (msglen < 1) {
                    msglen = 1;
                }
                break;
        }
    }

    /* If a rate is specified, compute 'period_ts' and 'rate_cnt_limit'
       from 'rate'. */
    if (rate) {
        if (rate == 1) {
            period_ts.tv_sec = 1;
            period_ts.tv_nsec = 0;
        } else {
            period_ts.tv_sec = 0;
            period_ts.tv_nsec = 1000000000 / rate;
        }

        rate_cnt_limit = (2 * 1000000000) /
                        (period_ts.tv_sec * 1000000000 + period_ts.tv_nsec);
    }

    /* Fill in the buffer for writes. */
    i = 0;
    len = strlen(text);
    while (i < msglen - 1) {
        unsigned int copylen = msglen - 1 - i;

        if (len < copylen) {
            copylen = len;
        }
        memcpy(buffer + i, text, copylen);
        i += copylen;
    }
    buffer[msglen-1] = '\0';

    printf("mode:       %d\n", mode);
    printf("#iter:      %lu\n", iterations);
    printf("rate:       %d\n", rate);
    printf("msglen:     %d\n", msglen);

    /* Open the vmpi-test device. */
    fd = open(devname, O_RDWR);
    if (fd < 0) {
        perror("open\n");
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &next_ts);
    rate_ts = next_ts;

    if (mode == MODE_WRITE) {
        for (i = 0; !iterations || i < iterations; i++) {
            n = write(fd, buffer, msglen);
            if (n < 0) {
                perror("write\n");
                return -1;
            }

            rate_bytes += n;
            rate_cnt++;
            if (rate_cnt == rate_cnt_limit) {
                rate_print(&rate_cnt_limit, &rate_ts, rate_bytes);
                rate_cnt = 0;
                rate_bytes = 0;
            }

            if (rate) {
                wait_time(next_ts);
                next_ts = timespec_add(next_ts, period_ts);
            }

            IFV(printf("written %d bytes\n", n));
        }
    } else if (mode == MODE_READ) {
        IFV(int j);

        for (i = 0; !iterations || i < iterations; i++) {
            n = read(fd, buffer, sizeof(buffer));
            if (n < 0) {
                perror("read\n");
                return -1;
            }

            rate_bytes += n;
            rate_cnt++;
            if (rate_cnt == rate_cnt_limit) {
                rate_print(&rate_cnt_limit, &rate_ts, rate_bytes);
                rate_cnt = 0;
                rate_bytes = 0;
            }
            if (rate) {
                wait_time(next_ts);
                next_ts = timespec_add(next_ts, period_ts);
            }

#ifdef VERBOSE
            printf("read string: '");
            for (j = 0; j < n; j++) {
                printf("%c", buffer[j]);
            }
            printf("'\n");
#endif  /* VERBOSE */
        }
    } else {
        printf("Error: unknown mode");

        return -1;
    }

    close(fd);

    return 0;
}
