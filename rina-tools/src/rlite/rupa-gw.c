/*
 * rupa-gw: IP over EFCP at the two edges of a 6G-RUPA access network.
 *
 *   rupa-gw ue  -d DIF -i UE_IP [-r GW_APPL] [-t TUN] [-m MTU] [-R]
 *   rupa-gw n6  -d DIF [-l GW_APPL] [-t TUN] [-m MTU] [-c SOCKET] [-b UE_IP]...
 *   rupa-gw ctl SOCKET bind UE_IP | unbind UE_IP | list
 *
 * ue: a TUN device for the UE's IP traffic and one flow to the GUPF's N6
 * gateway, requested from the application "rupagw-ue|UE_IP": the application
 * instance names the PDU session. A failed or deallocated flow is requested
 * again every second; meanwhile IP packets are dropped, as on a lost radio
 * link. The flow is unreliable unless -R asks for a reliable one.
 *
 * n6: a TUN device towards the data network (N6) and one flow per UE. A flow
 * is accepted only from a UE IP that is bound, through the control socket or
 * with -b: the PFCP->EFCP driver binds a UE IP when the SMF establishes the PDU
 * session and unbinds it when the session is released. Packets from the TUN
 * go to the flow of their destination UE IP; packets from a flow must carry
 * that UE IP as their source. Unbinding closes the UE's flow.
 *
 * Control protocol (n6): one request line per connection on a UNIX stream
 * socket, answered by "ok" or "error <reason>"; "list" first prints one line
 * per binding: "<ue-ip> connected|bound up=<pkts> down=<pkts>
 * up_bytes=<bytes> down_bytes=<bytes>", where up is UE to N6 and down is N6
 * to UE; the counters are those of the current binding. A socket path too
 * long for sun_path (107 bytes) is refused, not truncated.
 *
 * The UE IP is the PDU session's address, not an IPCP address, so moving the
 * UE (renumbering its IPCP) changes nothing here: the flow is rebound by EFCP.
 */

#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include <rina/api.h>

#define GW_APPL "rupagw|n6"
#define UE_APPL "rupagw-ue"
#define CTL_SOCKET "/run/rupa-gw.sock"
#define MAX_UES 256
#define BUF_SIZE 65536

static volatile sig_atomic_t stop;

static void on_signal(int sig)
{
    (void)sig;
    stop = 1;
}

static void say(const char *fmt, ...)
{
    struct timespec ts;
    va_list ap;

    clock_gettime(CLOCK_REALTIME, &ts);
    fprintf(stderr, "[%ld.%03ld] ", (long)ts.tv_sec, ts.tv_nsec / 1000000);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

static int nonblock(int fd)
{
    int flags = fcntl(fd, F_GETFL);

    return flags < 0 ? -1 : fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

/* A TUN device without packet information, up, with the given MTU. */
static int tun_open(const char *name, int mtu)
{
    struct ifreq ifr;
    int fd, s;

    fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        perror("open(/dev/net/tun)");
        return -1;
    }
    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;
    strncpy(ifr.ifr_name, name, IFNAMSIZ - 1);
    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return -1;
    }
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
        perror("socket");
        close(fd);
        return -1;
    }
    ifr.ifr_mtu = mtu;
    if (ioctl(s, SIOCSIFMTU, &ifr) < 0) {
        perror("ioctl(SIOCSIFMTU)");
    }
    if (ioctl(s, SIOCGIFFLAGS, &ifr) == 0) {
        ifr.ifr_flags |= IFF_UP | IFF_RUNNING;
        if (ioctl(s, SIOCSIFFLAGS, &ifr) < 0) {
            perror("ioctl(SIOCSIFFLAGS)");
        }
    }
    close(s);
    nonblock(fd);
    return fd;
}

static int ipv4_parse(const char *s, uint32_t *ip)
{
    struct in_addr a;

    if (inet_pton(AF_INET, s, &a) != 1) {
        return -1;
    }
    *ip = a.s_addr;
    return 0;
}

static const char *ipv4_str(uint32_t ip, char *buf)
{
    struct in_addr a = { .s_addr = ip };

    return inet_ntop(AF_INET, &a, buf, INET_ADDRSTRLEN);
}

/* Source and destination of an IPv4 packet; -1 for anything else. */
static int ipv4_ends(const unsigned char *p, ssize_t n, uint32_t *src, uint32_t *dst)
{
    if (n < 20 || (p[0] >> 4) != 4) {
        return -1;
    }
    memcpy(src, p + 12, 4);
    memcpy(dst, p + 16, 4);
    return 0;
}

/* ---- ue ------------------------------------------------------------------ */

static int run_ue(const char *dif, const char *gw_appl, const char *ue_ip,
                  const char *tun_name, int mtu, int reliable)
{
    static unsigned char buf[BUF_SIZE];
    struct rina_flow_spec spec;
    char local[128];
    time_t next_try = 0;
    unsigned long up = 0, down = 0, dropped = 0;
    int tun, flow = -1;
    uint32_t ip;

    if (ipv4_parse(ue_ip, &ip)) {
        fprintf(stderr, "bad UE IP %s\n", ue_ip);
        return 2;
    }
    snprintf(local, sizeof(local), "%s|%s", UE_APPL, ue_ip);
    tun = tun_open(tun_name, mtu);
    if (tun < 0) {
        return 1;
    }
    rina_flow_spec_unreliable(&spec);
    if (reliable) {
        spec.max_sdu_gap = 0;
        spec.in_order_delivery = 1;
    }
    say("ue %s on %s, flow to %s in %s", local, tun_name, gw_appl, dif);

    while (!stop) {
        struct pollfd pfd[2];
        int n = 0, ret;

        if (flow < 0 && time(NULL) >= next_try) {
            flow = rina_flow_alloc(dif, local, gw_appl, &spec, 0);
            if (flow < 0) {
                say("flow to %s failed: %s; retrying in 1 s", gw_appl, strerror(errno));
                next_try = time(NULL) + 1;
            } else {
                nonblock(flow);
                say("flow up (fd %d)", flow);
            }
        }
        pfd[n].fd = tun;
        pfd[n++].events = POLLIN;
        if (flow >= 0) {
            pfd[n].fd = flow;
            pfd[n++].events = POLLIN;
        }
        ret = poll(pfd, n, 1000);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("poll");
            break;
        }
        if (pfd[0].revents & POLLIN) {
            ssize_t len = read(tun, buf, sizeof(buf));

            if (len > 0) {
                if (flow < 0 || write(flow, buf, len) != len) {
                    dropped++;
                    if (flow >= 0 && errno != EAGAIN) {
                        say("flow write failed: %s; reconnecting", strerror(errno));
                        close(flow);
                        flow = -1;
                    }
                } else {
                    up++;
                }
            }
        }
        if (n > 1 && (pfd[1].revents & (POLLIN | POLLHUP | POLLERR))) {
            ssize_t len = read(flow, buf, sizeof(buf));

            if (len > 0) {
                if (write(tun, buf, len) == len) {
                    down++;
                } else {
                    dropped++;
                }
            } else if (len == 0 || errno != EAGAIN) {
                say("flow closed (%s); reconnecting", len == 0 ? "deallocated" : strerror(errno));
                close(flow);
                flow = -1;
                next_try = time(NULL) + 1;
            }
        }
    }
    say("ue stops: up=%lu down=%lu dropped=%lu", up, down, dropped);
    if (flow >= 0) {
        close(flow);
    }
    close(tun);
    return 0;
}

/* ---- n6 ------------------------------------------------------------------ */

struct ue {
    uint32_t ip;
    int fd; /* the UE's flow, -1 while not connected */
    unsigned long up, down;           /* packets */
    unsigned long long up_b, down_b;  /* bytes */
};

static struct ue ues[MAX_UES];
static int nues;

static struct ue *ue_find(uint32_t ip)
{
    int i;

    for (i = 0; i < nues; i++) {
        if (ues[i].ip == ip) {
            return &ues[i];
        }
    }
    return NULL;
}

static int ue_bind(uint32_t ip)
{
    if (ue_find(ip)) {
        return 0;
    }
    if (nues == MAX_UES) {
        return -1;
    }
    ues[nues].ip = ip;
    ues[nues].fd = -1;
    ues[nues].up = ues[nues].down = 0;
    ues[nues].up_b = ues[nues].down_b = 0;
    nues++;
    return 0;
}

static void ue_disconnect(struct ue *u)
{
    if (u->fd >= 0) {
        close(u->fd);
        u->fd = -1;
    }
}

static void ue_unbind(uint32_t ip)
{
    struct ue *u = ue_find(ip);

    if (!u) {
        return;
    }
    ue_disconnect(u);
    *u = ues[--nues];
}

/* The UE IP in a requester name "rupagw-ue|UE_IP|...", or -1. */
static int requester_ip(const char *name, uint32_t *ip)
{
    char buf[128], *inst, *end;
    size_t plen = strlen(UE_APPL);

    if (!name || strncmp(name, UE_APPL "|", plen + 1)) {
        return -1;
    }
    snprintf(buf, sizeof(buf), "%s", name + plen + 1);
    inst = buf;
    end = strchr(inst, '|');
    if (end) {
        *end = '\0';
    }
    return ipv4_parse(inst, ip);
}

static void accept_flow(int rfd)
{
    struct rina_flow_spec spec;
    char *remote = NULL;
    char ipbuf[INET_ADDRSTRLEN];
    struct ue *u = NULL;
    uint32_t ip = 0;
    int h, fd;

    memset(&spec, 0, sizeof(spec));
    spec.version = RINA_FLOW_SPEC_VERSION;
    h = rina_flow_accept(rfd, &remote, &spec, RINA_F_NORESP);
    if (h < 0) {
        say("flow accept failed: %s", strerror(errno));
        return;
    }
    if (requester_ip(remote, &ip) == 0) {
        u = ue_find(ip);
    }
    if (!u) {
        say("refused a flow from %s: not a bound UE", remote ? remote : "?");
        rina_flow_respond(rfd, h, 1);
        free(remote);
        return;
    }
    fd = rina_flow_respond(rfd, h, 0);
    if (fd < 0) {
        say("flow from %s: respond failed: %s", remote, strerror(errno));
        free(remote);
        return;
    }
    if (u->fd >= 0) {
        say("UE %s requested a new flow: replacing fd %d", ipv4_str(ip, ipbuf), u->fd);
        ue_disconnect(u);
    }
    nonblock(fd);
    u->fd = fd;
    /* The spec IRATI hands the acceptor said "unreliable" for a flow the UE
     * requested with -R, so it is not reported here; the connection's qos_id
     * in /sys/rina shows the cube. */
    say("UE %s connected (fd %d)", ipv4_str(ip, ipbuf), fd);
    free(remote);
}

/* A control socket path must fit sun_path: refuse it rather than truncate. */
static int ctl_path_ok(const char *path)
{
    struct sockaddr_un sa;

    if (strlen(path) < sizeof(sa.sun_path)) {
        return 1;
    }
    fprintf(stderr, "control socket path longer than %zu bytes: %s\n",
            sizeof(sa.sun_path) - 1, path);
    return 0;
}

static int ctl_listen(const char *path)
{
    struct sockaddr_un sa;
    int s;

    s = socket(AF_UNIX, SOCK_STREAM, 0);
    if (s < 0) {
        perror("socket(AF_UNIX)");
        return -1;
    }
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    snprintf(sa.sun_path, sizeof(sa.sun_path), "%s", path);
    unlink(path);
    if (bind(s, (struct sockaddr *)&sa, sizeof(sa)) < 0 || listen(s, 8) < 0) {
        perror("control socket");
        close(s);
        return -1;
    }
    return s;
}

/* One request per connection; the client writes its line, then reads. */
static void ctl_serve(int ls)
{
    char req[256], reply[MAX_UES * 160 + 64], ipbuf[INET_ADDRSTRLEN], arg[64];
    char cmd[16];
    struct timeval tmo = { .tv_sec = 0, .tv_usec = 200000 };
    size_t off = 0;
    ssize_t n;
    uint32_t ip;
    int c, i;

    c = accept(ls, NULL, NULL);
    if (c < 0) {
        return;
    }
    /* The data path waits while a request is served: a client that connects
     * and sends nothing may hold it for at most this long. */
    setsockopt(c, SOL_SOCKET, SO_RCVTIMEO, &tmo, sizeof(tmo));
    n = read(c, req, sizeof(req) - 1);
    if (n <= 0) {
        close(c);
        return;
    }
    req[n] = '\0';
    arg[0] = '\0';
    if (sscanf(req, "%15s %63s", cmd, arg) < 1) {
        snprintf(reply, sizeof(reply), "error empty request\n");
    } else if (!strcmp(cmd, "list")) {
        for (i = 0; i < nues; i++) {
            off += snprintf(reply + off, sizeof(reply) - off, "%s %s up=%lu down=%lu up_bytes=%llu down_bytes=%llu\n",
                            ipv4_str(ues[i].ip, ipbuf), ues[i].fd >= 0 ? "connected" : "bound",
                            ues[i].up, ues[i].down, ues[i].up_b, ues[i].down_b);
        }
        snprintf(reply + off, sizeof(reply) - off, "ok\n");
    } else if ((!strcmp(cmd, "bind") || !strcmp(cmd, "unbind")) && ipv4_parse(arg, &ip) == 0) {
        if (cmd[0] == 'b') {
            snprintf(reply, sizeof(reply), ue_bind(ip) ? "error too many UEs\n" : "ok\n");
        } else {
            ue_unbind(ip);
            snprintf(reply, sizeof(reply), "ok\n");
        }
        say("control: %s %s", cmd, arg);
    } else {
        req[strcspn(req, "\n")] = '\0';
        snprintf(reply, sizeof(reply), "error unknown request: %s\n", req);
    }
    if (write(c, reply, strlen(reply)) < 0) {
        perror("control reply");
    }
    close(c);
}

static int run_n6(const char *dif, const char *gw_appl, const char *tun_name, int mtu,
                  const char *ctl_path)
{
    static unsigned char buf[BUF_SIZE];
    struct pollfd pfd[3 + MAX_UES];
    int map[3 + MAX_UES];
    unsigned long no_ue = 0, spoofed = 0, dropped = 0;
    int rfd, tun, ls, i;

    rfd = rina_open();
    if (rfd < 0) {
        perror("rina_open");
        return 1;
    }
    if (rina_register(rfd, dif, gw_appl, 0) < 0) {
        perror("rina_register");
        return 1;
    }
    tun = tun_open(tun_name, mtu);
    if (tun < 0) {
        return 1;
    }
    ls = ctl_listen(ctl_path);
    if (ls < 0) {
        return 1;
    }
    say("n6 %s in %s on %s, control %s", gw_appl, dif, tun_name, ctl_path);

    while (!stop) {
        int n = 0, ret;

        pfd[n].fd = rfd;
        pfd[n++].events = POLLIN;
        pfd[n].fd = tun;
        pfd[n++].events = POLLIN;
        pfd[n].fd = ls;
        pfd[n++].events = POLLIN;
        for (i = 0; i < nues; i++) {
            if (ues[i].fd >= 0) {
                map[n] = i;
                pfd[n].fd = ues[i].fd;
                pfd[n++].events = POLLIN;
            }
        }
        ret = poll(pfd, n, 1000);
        if (ret < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("poll");
            break;
        }
        /* UE flows first: the indices in map[] are only valid until a bind,
         * an unbind or an accept below changes ues[]. */
        for (i = 3; i < n; i++) {
            struct ue *u = &ues[map[i]];
            uint32_t src, dst;
            ssize_t len;

            if (!(pfd[i].revents & (POLLIN | POLLHUP | POLLERR))) {
                continue;
            }
            len = read(u->fd, buf, sizeof(buf));
            if (len > 0) {
                if (ipv4_ends(buf, len, &src, &dst) || src != u->ip) {
                    spoofed++;
                } else if (write(tun, buf, len) == len) {
                    u->up++;
                    u->up_b += len;
                } else {
                    dropped++;
                }
            } else if (len == 0 || errno != EAGAIN) {
                char ipbuf[INET_ADDRSTRLEN];

                say("UE %s flow closed (%s)", ipv4_str(u->ip, ipbuf),
                    len == 0 ? "deallocated" : strerror(errno));
                ue_disconnect(u);
            }
        }
        if (pfd[1].revents & POLLIN) {
            ssize_t len = read(tun, buf, sizeof(buf));
            uint32_t src, dst;
            struct ue *u;

            if (len > 0) {
                if (ipv4_ends(buf, len, &src, &dst) || !(u = ue_find(dst)) || u->fd < 0) {
                    no_ue++;
                } else if (write(u->fd, buf, len) == len) {
                    u->down++;
                    u->down_b += len;
                } else {
                    dropped++;
                }
            }
        }
        if (pfd[0].revents & POLLIN) {
            accept_flow(rfd);
        }
        if (pfd[2].revents & POLLIN) {
            ctl_serve(ls);
        }
    }
    say("n6 stops: no_ue=%lu spoofed=%lu dropped=%lu", no_ue, spoofed, dropped);
    for (i = 0; i < nues; i++) {
        ue_disconnect(&ues[i]);
    }
    rina_unregister(rfd, dif, gw_appl, 0);
    close(ls);
    unlink(ctl_path);
    close(tun);
    return 0;
}

/* ---- ctl ----------------------------------------------------------------- */

static int run_ctl(const char *path, int argc, char **argv)
{
    char req[256], buf[MAX_UES * 160 + 64];
    struct sockaddr_un sa;
    size_t off = 0;
    ssize_t n;
    int s, i, ok;

    for (i = 0; i < argc; i++) {
        off += snprintf(req + off, sizeof(req) - off, "%s%s", i ? " " : "", argv[i]);
    }
    snprintf(req + off, sizeof(req) - off, "\n");
    s = socket(AF_UNIX, SOCK_STREAM, 0);
    memset(&sa, 0, sizeof(sa));
    sa.sun_family = AF_UNIX;
    snprintf(sa.sun_path, sizeof(sa.sun_path), "%s", path);
    if (s < 0 || connect(s, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror(path);
        return 1;
    }
    if (write(s, req, strlen(req)) < 0) {
        perror("write");
        return 1;
    }
    off = 0;
    while (off < sizeof(buf) - 1 && (n = read(s, buf + off, sizeof(buf) - 1 - off)) > 0) {
        off += n;
    }
    buf[off] = '\0';
    close(s);
    fputs(buf, stdout);
    ok = off >= 3 && !strcmp(buf + off - 3, "ok\n");
    return ok ? 0 : 1;
}

static void usage(void)
{
    fprintf(stderr,
            "usage: rupa-gw ue  -d DIF -i UE_IP [-r GW_APPL] [-t TUN] [-m MTU] [-R]\n"
            "       rupa-gw n6  -d DIF [-l GW_APPL] [-t TUN] [-m MTU] [-c SOCKET] [-b UE_IP]...\n"
            "       rupa-gw ctl SOCKET bind UE_IP | unbind UE_IP | list\n");
    exit(2);
}

int main(int argc, char **argv)
{
    const char *mode, *dif = NULL, *ue_ip = NULL, *appl = GW_APPL, *tun = NULL;
    const char *ctl = CTL_SOCKET;
    int mtu = 1400, reliable = 0, opt;
    struct sigaction sa;
    uint32_t ip;

    if (argc < 2) {
        usage();
    }
    mode = argv[1];
    if (!strcmp(mode, "ctl")) {
        if (argc < 4) {
            usage();
        }
        return ctl_path_ok(argv[2]) ? run_ctl(argv[2], argc - 3, argv + 3) : 2;
    }
    if (strcmp(mode, "ue") && strcmp(mode, "n6")) {
        usage();
    }
    optind = 2;
    while ((opt = getopt(argc, argv, "d:i:r:l:t:m:c:b:R")) != -1) {
        switch (opt) {
        case 'd': dif = optarg; break;
        case 'i': ue_ip = optarg; break;
        case 'r':
        case 'l': appl = optarg; break;
        case 't': tun = optarg; break;
        case 'm': mtu = atoi(optarg); break;
        case 'c': ctl = optarg; break;
        case 'R': reliable = 1; break;
        case 'b':
            if (ipv4_parse(optarg, &ip) || ue_bind(ip)) {
                fprintf(stderr, "bad UE IP %s\n", optarg);
                return 2;
            }
            break;
        default: usage();
        }
    }
    if (!dif) {
        usage();
    }
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_signal;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
    signal(SIGPIPE, SIG_IGN);
    setvbuf(stderr, NULL, _IOLBF, 0);

    if (!strcmp(mode, "ue")) {
        if (!ue_ip) {
            usage();
        }
        return run_ue(dif, appl, ue_ip, tun ? tun : "rupa0", mtu, reliable);
    }
    if (!ctl_path_ok(ctl)) {
        return 2;
    }
    return run_n6(dif, appl, tun ? tun : "n6gw", mtu, ctl);
}
