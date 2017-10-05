#include <cstdlib>
#include <cstring>
#include <iostream>
#include <fstream>
#include <string>
#include <list>
#include <set>
#include <vector>
#include <sstream>
#include <algorithm>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <poll.h>
#include <pthread.h>
#include <errno.h>
#include <stdint.h>
#include <rina/api.h>

#include "cdap.hpp"
#include "fdfwd.hpp"
#include "iporina.pb.h"

using namespace std;

/*
 * Internal data structures.
 */

struct IPAddr {
    string repr;
    uint32_t addr;
    unsigned netbits;

    IPAddr() : addr(0), netbits(0) {}
    IPAddr(const string &p);
    IPAddr(uint32_t naddr, unsigned nbits);

    bool empty() const { return netbits == 0 && addr == 0; }

    bool operator<(const IPAddr &o) const
    {
        return addr < o.addr || (addr == o.addr && netbits < o.netbits);
    }

    /* Pick an host address in belonging to the same subnet. */
    IPAddr hostaddr(uint32_t host) const
    {
        uint32_t hostmask = ~((1 << (32 - netbits)) - 1);

        if (host & hostmask) {
            throw "Host number out of range";
        }

        return IPAddr((addr & hostmask) + host, netbits);
    }

    string noprefix() const;
    operator string() const { return repr; }
};

struct Route {
    IPAddr subnet;

    Route(const IPAddr &i) : subnet(i) {}
    bool operator<(const Route &o) const { return subnet < o.subnet; }
};

struct Local {
    string app_name;
    list<string> dif_names;

    Local() {}
    Local(const string &a) : app_name(a) {}
};

enum {
    IPOR_CTRL = 0,
    IPOR_DATA,
    IPOR_MAX,
};

struct Remote {
    /* Application name and DIF of the remote */
    string app_name;
    string dif_name;

    /* Name of the local tun device and associated file descriptor. */
    string tun_name;
    int tun_fd;

    /* IP address of the local and remote tunnel endpoint, and tunnel subnet. */
    IPAddr tun_local_addr;
    IPAddr tun_remote_addr;
    IPAddr tun_subnet;

    /* True if we need to allocate a control/data flow for this remote. */
    bool flow_alloc_needed[IPOR_MAX];

    /* Routes reachable through this remote. */
    set<Route> routes;

    /* Data file descriptor for the flow that supports the tunnel. */
    int rfd;

    Remote() : tun_fd(-1), rfd(-1)
    {
        flow_alloc_needed[IPOR_CTRL] = flow_alloc_needed[IPOR_DATA] = true;
    }

    Remote(const string &a, const string &d, const IPAddr &i)
        : app_name(a), dif_name(d), tun_fd(-1), tun_subnet(i), rfd(-1)
    {
        flow_alloc_needed[IPOR_CTRL] = flow_alloc_needed[IPOR_DATA] = true;
    }

    /* Allocate a tunnel device for this remote. */
    int tun_alloc();

    /* Configure tunnel device IP address and routes. */
    int ip_configure() const;

    int mss_configure() const;
};

#define NUM_WORKERS 1

struct IPoRINA {
    /* Enable verbose mode */
    int verbose;

    /* Control device to listen for incoming connections. */
    int rfd;

    /* Daemon configuration. */
    Local local;
    map<string, Remote> remotes;
    list<Route> local_routes;

    /* Worker threads that forward traffic. */
    vector<FwdWorker *> workers;

    IPoRINA();
    ~IPoRINA();

    void start_workers();
};

/*
 * CDAP objects with their serialization and deserialization routines
 */

struct Obj {
    virtual int serialize(char *buf, unsigned int size) const = 0;
    virtual ~Obj() {}
};

static int
ser_common(::google::protobuf::MessageLite &gm, char *buf, int size)
{
    if (gm.ByteSize() > size) {
        fprintf(stderr, "User buffer too small [%u/%u]\n", gm.ByteSize(), size);
        return -1;
    }

    gm.SerializeToArray(buf, size);

    return gm.ByteSize();
}

struct Hello : public Obj {
    string tun_subnet;   /* Subnet to be used for the tunnel */
    string tun_src_addr; /* IP address of the source */
    string tun_dst_addr; /* IP address of the destination */
    uint32_t num_routes; /* How many route to exchange */

    Hello() : num_routes(0) {}
    Hello(const char *buf, unsigned int size);
    int serialize(char *buf, unsigned int size) const;
};

static void
gpb2Hello(Hello &m, const gpb::hello_msg_t &gm)
{
    m.tun_subnet   = gm.tun_subnet();
    m.tun_src_addr = gm.tun_src_addr();
    m.tun_dst_addr = gm.tun_dst_addr();
    m.num_routes   = gm.num_routes();
}

static int
Hello2gpb(const Hello &m, gpb::hello_msg_t &gm)
{
    gm.set_tun_subnet(m.tun_subnet);
    gm.set_tun_src_addr(m.tun_src_addr);
    gm.set_tun_dst_addr(m.tun_dst_addr);
    gm.set_num_routes(m.num_routes);

    return 0;
}

Hello::Hello(const char *buf, unsigned int size) : num_routes(0)
{
    gpb::hello_msg_t gm;

    gm.ParseFromArray(buf, size);

    gpb2Hello(*this, gm);
}

int
Hello::serialize(char *buf, unsigned int size) const
{
    gpb::hello_msg_t gm;

    Hello2gpb(*this, gm);

    return ser_common(gm, buf, size);
}

struct RouteObj : public Obj {
    string route; /* Route represented as a string. */

    RouteObj() {}
    RouteObj(const string &s) : route(s) {}
    RouteObj(const char *buf, unsigned int size);
    int serialize(char *buf, unsigned int size) const;
};

static void
gpb2RouteObj(RouteObj &m, const gpb::route_msg_t &gm)
{
    m.route = gm.route();
}

static int
RouteObj2gpb(const RouteObj &m, gpb::route_msg_t &gm)
{
    gm.set_route(m.route);

    return 0;
}

RouteObj::RouteObj(const char *buf, unsigned int size)
{
    gpb::route_msg_t gm;

    gm.ParseFromArray(buf, size);

    gpb2RouteObj(*this, gm);
}

int
RouteObj::serialize(char *buf, unsigned int size) const
{
    gpb::route_msg_t gm;

    RouteObj2gpb(*this, gm);

    return ser_common(gm, buf, size);
}

/* Send a CDAP message after attaching a serialized object. */
static int
cdap_obj_send(CDAPConn *conn, CDAPMessage *m, int invoke_id, const Obj *obj)
{
    char objbuf[4096];
    int objlen;

    if (obj) {
        objlen = obj->serialize(objbuf, sizeof(objbuf));
        if (objlen < 0) {
            errno = EINVAL;
            fprintf(stderr, "serialization failed\n");
            return objlen;
        }

        m->set_obj_value(objbuf, objlen);
    }

    return conn->msg_send(m, invoke_id);
}

/*
 * Global variables to hold the daemon state.
 */

static IPoRINA _g;
static IPoRINA *g = &_g;

#if 0
static string
int2string(int x)
{
    stringstream sstr;
    sstr << x;
    return sstr.str();
}
#endif

static int
string2int(const string &s, int &ret)
{
    char *dummy;
    const char *cstr = s.c_str();

    ret = strtoul(cstr, &dummy, 10);
    if (!s.size() || *dummy != '\0') {
        ret = ~0U;
        return -1;
    }

    return 0;
}

IPoRINA::IPoRINA() : verbose(0), rfd(-1) {}

void
IPoRINA::start_workers()
{
    for (int i = 0; i < NUM_WORKERS; i++) {
        workers.push_back(new FwdWorker(i, verbose));
    }
}

IPoRINA::~IPoRINA()
{
    for (unsigned int i = 0; i < workers.size(); i++) {
        delete workers[i];
    }
}

IPAddr::IPAddr(const string &_p) : repr(_p)
{
    string p = _p;
    string digit;
    size_t slash;
    int m;

    slash = p.find("/");

    if (slash == string::npos) {
        throw "Invalid IP prefix";
    }

    /* Extract the mask m in "a.b.c.d/m" */
    if (string2int(p.substr(slash + 1), m)) {
        throw "Invalid IP prefix";
    }
    if (m < 1 || m > 30) {
        throw "Invalid IP prefix";
    }
    netbits = m;
    p       = p.substr(0, slash);

    /* Extract a, b, c and d. */
    std::replace(p.begin(), p.end(), '.', ' ');

    stringstream ss(p);

    addr = 0;
    while (ss >> digit) {
        int d;

        if (string2int(digit, d)) {
            throw "Invalid IP prefix";
        }

        if (d < 0 || d > 255) {
            throw "Invalid IP prefix";
        }
        addr <<= 8;
        addr |= (unsigned)d;
    }

    return;
}

IPAddr::IPAddr(uint32_t naddr, unsigned nbits)
{
    unsigned a, b, c, d;
    stringstream ss;

    if (!nbits || nbits > 30) {
        throw "Invalid IP prefix";
    }

    addr    = naddr;
    netbits = nbits;

    d = naddr & 0xff;
    naddr >>= 8;
    c = naddr & 0xff;
    naddr >>= 8;
    b = naddr & 0xff;
    naddr >>= 8;
    a = naddr & 0xff;

    ss << a << "." << b << "." << c << "." << d << "/" << nbits;
    repr = ss.str();
}

string
IPAddr::noprefix() const
{
    size_t slash = repr.find("/");

    if (slash == string::npos) {
        throw "Invalid IP prefix";
    }

    return repr.substr(0, slash);
}

/* Arguments taken by the function:
 *
 * char *dev: the name of an interface (or '\0'). MUST have enough
 *            space to hold the interface name if '\0' is passed
 * int flags: interface flags (eg, IFF_TUN, IFF_NO_PI, IFF_TAP etc.)
 */
static int
os_tun_alloc(char *dev, int flags)
{
    struct ifreq ifr;
    int fd, err;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("open(/dev/net/tun)");
        return fd;
    }

    memset(&ifr, 0, sizeof(ifr));

    ifr.ifr_flags = flags; /* IFF_TUN or IFF_TAP, plus maybe IFF_NO_PI */

    if (*dev) {
        /* If a device name was specified, put it in the structure; otherwise,
         * the kernel will try to allocate the "next" device of the
         * specified type */
        strncpy(ifr.ifr_name, dev, IFNAMSIZ);
    }

    /* Try to create the device */
    if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return err;
    }

    /* If the operation was successful, write back the name of the
     * interface to the variable "dev", so the caller can know
     * it. Note that the caller MUST reserve space in *dev (see calling
     * code below) */
    strcpy(dev, ifr.ifr_name);

    /* this is the special file descriptor that the caller will use to talk
     * with the virtual interface */
    return fd;
}

/* Binary search for the MSS size for a RINA flow :)
 * (currently unused) */
unsigned int
probe_mss(int fd)
{
    unsigned int ub = 1;
    unsigned int lb = 1;
#define BUFSIZE 65536
    char buf[BUFSIZE];

    memset(buf, 0, sizeof(buf));

    for (;;) {
        int n = write(fd, buf, ub);

        if (n < 0) {
            /* we may check for (errno == EMSGSIZE) */
            break;
        }

        if ((ub << 1) > BUFSIZE) {
            /* Safety check. */
            break;
        }
        ub <<= 1;
    }

    for (;;) {
        unsigned int m = (lb + ub) / 2;
        int n;

        if (m == lb) {
            break;
        }

        n = write(fd, buf, m);
        if (n < 0) {
            /* we may check for (errno == EMSGSIZE) */
            ub = m;
        } else {
            lb = m;
        }
    }

    return lb;
#undef BUFSIZE
}

static int
parse_conf(const char *path)
{
    ifstream fin(path);

    if (fin.fail()) {
        cerr << "Cannot open configuration file " << path << endl;
        return -1;
    }

    for (unsigned int lines_cnt = 1; !fin.eof(); lines_cnt++) {
        string line;

        getline(fin, line);

        istringstream iss(line);
        vector<string> tokens;
        string token;

        while (iss >> token) {
            tokens.push_back(token);
        }

        if (tokens.size() <= 0 || tokens[0][0] == '#') {
            /* Ignore comments and white spaces. */
            continue;
        }

        if (tokens[0] == "local") {
            if (tokens.size() < 3) {
                cerr << "Invalid 'local' directive at line " << lines_cnt
                     << endl;
                return -1;
            }

            if (g->local.app_name.size()) {
                cerr << "Duplicated 'local' directive at line " << lines_cnt
                     << endl;
                return -1;
            }

            g->local = Local(tokens[1]);
            for (unsigned int i = 2; i < tokens.size(); i++) {
                g->local.dif_names.push_back(tokens[i]);
            }

        } else if (tokens[0] == "remote") {
            IPAddr subnet;

            if (tokens.size() != 4) {
                cerr << "Invalid 'remote' directive at line " << lines_cnt
                     << endl;
                return -1;
            }

            try {
                subnet = IPAddr(tokens[3]);
            } catch (...) {
                cerr << "Invalid IP prefix at line " << lines_cnt << endl;
                return -1;
            }

            if (g->remotes.count(tokens[1])) {
                cerr << "Duplicated 'remote' directive at line " << lines_cnt
                     << endl;
                return -1;
            }
            g->remotes[tokens[1]] = Remote(tokens[1], tokens[2], subnet);

        } else if (tokens[0] == "route") {
            IPAddr subnet;

            if (tokens.size() != 2) {
                cerr << "Invalid 'route' directive at line " << lines_cnt
                     << endl;
                return -1;
            }

            try {
                subnet = IPAddr(tokens[1]);
            } catch (...) {
                cerr << "Invalid IP prefix at line " << lines_cnt << endl;
                return -1;
            }

            g->local_routes.push_back(Route(subnet));
        }
    }

    fin.close();

    return 0;
}

static void
dump_conf(void)
{
    cout << "Local: " << g->local.app_name << " in DIFs";
    for (list<string>::iterator l = g->local.dif_names.begin();
         l != g->local.dif_names.end(); l++) {
        cout << " " << *l;
    }
    cout << endl;

    cout << "Remotes:" << endl;
    for (map<string, Remote>::iterator r = g->remotes.begin();
         r != g->remotes.end(); r++) {
        cout << "   " << r->second.app_name << " in DIF " << r->second.dif_name
             << ", tunnel prefix " << r->second.tun_subnet.repr << endl;
    }

    cout << "Advertised routes:" << endl;
    for (list<Route>::iterator l = g->local_routes.begin();
         l != g->local_routes.end(); l++) {
        cout << "   " << l->subnet.repr << endl;
    }
}

int
Remote::tun_alloc()
{
    char tname[IFNAMSIZ];

    if (tun_name != string()) {
        /* Nothing to do. */
        return 0;
    }

    tname[0] = '\0';
    tun_fd   = os_tun_alloc(tname, IFF_TUN | IFF_NO_PI);
    if (tun_fd < 0) {
        cerr << "Failed to create tunnel" << endl;
        return -1;
    }
    tun_name = tname;
    if (g->verbose) {
        cout << "Created tunnel device " << tun_name << endl;
    }

    return 0;
}

static int
execute_command(stringstream &cmdss)
{
    vector<string> tokens;
    string token;
    pid_t child_pid;
    int child_status;
    int ret = -1;
    char **argv;

    if (g->verbose) {
        cout << "Exec command '" << cmdss.str() << "'" << endl;
    }

    /* Separate the arguments into a vector. */
    while (cmdss >> token) {
        tokens.push_back(token);
    }

    /* Allocate a working copy for the arguments. */
    argv = new char *[tokens.size() + 1];
    for (unsigned i = 0; i <= tokens.size(); i++) {
        argv[i] = NULL;
    }
    for (unsigned i = 0; i < tokens.size(); i++) {
        argv[i] = strdup(tokens[i].c_str());
        if (!argv[i]) {
            cerr << "execute_command: out of memory while allocating "
                    "arguments"
                 << endl;
            goto out;
        }
    }

    child_pid = fork();
    if (child_pid == 0) {
        /* Child process: Redirect standard input, output and
         * error to /dev/null and execute the target command. */

        close(0);
        close(1);
        close(2);
        if (open("/dev/null", O_RDONLY) < 0 ||
            open("/dev/null", O_WRONLY) < 0 ||
            open("/dev/null", O_WRONLY) < 0) {
            /* Redirection failed. */
            return -1;
        }
        execvp(argv[0], argv);
        perror("execvp()");
        exit(0);
    }

    /* Parent process. Wait for the child to exit. */
    waitpid(child_pid, &child_status, 0);
    if (WIFEXITED(child_status)) {
        ret = WEXITSTATUS(child_status);
    }

out:
    for (unsigned i = 0; i < tokens.size(); i++) {
        if (argv[i]) {
            free(argv[i]);
            argv[i] = NULL;
        }
    }

    return ret;
}

static int
execute_command(const string &cmds)
{
    stringstream cmdss(cmds);

    return execute_command(cmdss);
}

int
Remote::ip_configure() const
{
    {
        /* Flush previous IP addresses, if any. */
        stringstream cmdss;

        cmdss << "ip addr flush dev " << tun_name;
        if (execute_command(cmdss)) {
            cerr << "Failed to flush address for interface " << tun_name
                 << endl;
            return -1;
        }
    }

    {
        /* Disable TSO and GSO. */
        stringstream cmdss;

        cmdss << "ethtool -K " << tun_name << " tso off gso off";
        if (execute_command(cmdss)) {
            cerr << "Failed to disable TSO and GSO for " << tun_name << endl;
            return -1;
        }
    }

    {
        /* Bring the tunnel device up. */
        stringstream cmdss;

        cmdss << "ip link set dev " << tun_name << " up";
        if (execute_command(cmdss)) {
            cerr << "Failed to bring device " << tun_name << " up" << endl;
            return -1;
        }
    }

    {
        /* Assign the designated IP address to the tunnel device. */
        stringstream cmdss;

        cmdss << "ip addr add " << tun_local_addr.repr << " dev " << tun_name;
        if (execute_command(cmdss)) {
            cerr << "Failed to assign IP address to interface " << tun_name
                 << endl;
            return -1;
        }
    }

    /* Setup the routes advertised by the remote peer. */
    for (set<Route>::iterator ri = routes.begin(); ri != routes.end(); ri++) {
        stringstream cmdss;

        cmdss << "ip route add " << static_cast<string>(ri->subnet) << " via "
              << tun_remote_addr.noprefix() << " dev " << tun_name;
        if (execute_command(cmdss)) {
            cerr << "Failed to add route for subnet "
                 << static_cast<string>(ri->subnet) << endl;
            return -1;
        }
    }

    return 0;
}

int
Remote::mss_configure() const
{
    stringstream cmdss;
    unsigned int mss = rina_flow_mss_get(rfd);

    if (!mss) { /* fallback */
        mss = 1200;
        cout << "Warning: using tentative MSS = " << mss << " bytes" << endl;
    }

    if (mss > FDFWD_MAX_BUFSZ) {
        /* Linux does not support MTU larger than 65535 for tun devices. */
        mss = FDFWD_MAX_BUFSZ;
    }

    cmdss << "ip link set mtu " << mss << " dev " << tun_name;

    if (execute_command(cmdss)) {
        cerr << "Failed to set MTU for device " << tun_name << endl;
    }

    return 0;
}

static int
setup(void)
{
    g->rfd = rina_open();
    if (g->rfd < 0) {
        perror("rina_open()");
        return -1;
    }

    /* Register us to one or more local DIFs. */
    for (list<string>::iterator l = g->local.dif_names.begin();
         l != g->local.dif_names.end(); l++) {
        int ret;

        ret = rina_register(g->rfd, l->c_str(), g->local.app_name.c_str(), 0);
        if (ret) {
            perror("rina_register()");
            cerr << "Failed to register " << g->local.app_name << " in DIF "
                 << *l << endl;
            return -1;
        }
    }

    /* Create a TUN device for each remote. */
    for (map<string, Remote>::iterator r = g->remotes.begin();
         r != g->remotes.end(); r++) {
        if (r->second.tun_alloc()) {
            return -1;
        }
    }

    return 0;
}

/* Try to connect to all the user-specified remotes. */
static void *
connect_to_remotes(void *opaque)
{
    string myname = g->local.app_name;

    for (;;) {
        for (map<string, Remote>::iterator re = g->remotes.begin();
             re != g->remotes.end(); re++) {
            for (unsigned i = 0; i < IPOR_MAX; i++) {
                struct rina_flow_spec spec;
                struct pollfd pfd;
                int rfd;
                int ret;
                int wfd;

                if (!re->second.flow_alloc_needed[i]) {
                    /* We already connected to this remote. */
                    continue;
                }

                /* Try to allocate a reliable flow for the control connection
                 * and an unreliable flow for the data connection. */
                rina_flow_spec_unreliable(&spec);
                if (i == IPOR_CTRL) {
                    spec.max_sdu_gap       = 0;
                    spec.in_order_delivery = 1;
                    spec.msg_boundaries    = 1;
                    spec.spare3            = 1;
                }
                wfd = rina_flow_alloc(
                    re->second.dif_name.c_str(), myname.c_str(),
                    re->second.app_name.c_str(), &spec, RINA_F_NOWAIT);
                if (wfd < 0) {
                    perror("rina_flow_alloc()");
                    cout << "Failed to connect to remote "
                         << re->second.app_name << " through DIF "
                         << re->second.dif_name << endl;
                    continue;
                }
                pfd.fd     = wfd;
                pfd.events = POLLIN;
                ret        = poll(&pfd, 1, 3000);
                if (ret <= 0) {
                    if (ret < 0) {
                        perror("poll(wfd)");
                    } else if (g->verbose) {
                        cout << "Failed to connect to remote "
                             << re->second.app_name << " through DIF "
                             << re->second.dif_name << endl;
                    }
                    close(wfd);
                    continue;
                }

                rfd = rina_flow_alloc_wait(wfd);
                if (rfd < 0) {
                    if (errno != EPERM || g->verbose > 1) {
                        if (errno != EPERM) {
                            perror("rina_flow_alloc_wait()");
                        }
                        cout << "Failed to connect to remote "
                             << re->second.app_name << " through DIF "
                             << re->second.dif_name << endl;
                    }
                    continue;
                }

                if (g->verbose > 1) {
                    cout << "Flow allocated to remote " << re->second.app_name
                         << " through DIF " << re->second.dif_name << endl;
                }

                CDAPConn conn(rfd);
                CDAPMessage m, *rm = NULL;
                Hello hello;

                /* CDAP connection setup. */
                if (conn.connect(myname, re->second.app_name, gpb::AUTH_NONE,
                                 NULL)) {
                    cerr << "CDAP connection failed" << endl;
                    goto abor;
                }

                if (i == IPOR_DATA) {
                    /* This is a data connection, send an empty CDAP start
                     * message to inform the remote peer. */
                    m.m_start(gpb::F_NO_FLAGS, "data", "/data", 0, 0, string());
                    if (cdap_obj_send(&conn, &m, 0, NULL) < 0) {
                        cerr << "Failed to send M_START(data)" << endl;
                        goto abor;
                    }
                    re->second.rfd = rfd;
                    re->second.mss_configure();

                    /* Submit the new fd mapping to a worker thread. */
                    g->workers[0]->submit(re->second.rfd, re->second.tun_fd);

                } else {
                    /* This is a control connection. */

                    /* Assign tunnel IP addresses if needed. */
                    if (re->second.tun_local_addr.empty() ||
                        re->second.tun_remote_addr.empty()) {
                        re->second.tun_local_addr =
                            re->second.tun_subnet.hostaddr(1);
                        re->second.tun_remote_addr =
                            re->second.tun_subnet.hostaddr(2);
                    }

                    /* Exchange routes. */
                    m.m_start(gpb::F_NO_FLAGS, "hello", "/hello", 0, 0,
                              string());
                    hello.num_routes   = g->local_routes.size();
                    hello.tun_subnet   = re->second.tun_subnet;
                    hello.tun_src_addr = re->second.tun_local_addr;
                    hello.tun_dst_addr = re->second.tun_remote_addr;
                    if (cdap_obj_send(&conn, &m, 0, &hello) < 0) {
                        cerr << "Failed to send M_START(hello)" << endl;
                        goto abor;
                    }

                    for (list<Route>::iterator ro = g->local_routes.begin();
                         ro != g->local_routes.end(); ro++) {
                        RouteObj robj(ro->subnet);

                        m.m_write(gpb::F_NO_FLAGS, "route", "/routes", 0, 0,
                                  string());
                        if (cdap_obj_send(&conn, &m, 0, &robj) < 0) {
                            cerr << "Failed to send M_WRITE" << endl;
                            goto abor;
                        }
                    }
                    re->second.ip_configure();
                }

                re->second.flow_alloc_needed[i] = false;
            abor:
                if (rm) {
                    delete rm;
                }

                /* Don't close a data file descriptor which is going
                 * to be used. */
                if (re->second.rfd != rfd) {
                    close(rfd);
                }
            }
        }

        sleep(3);
    }

    pthread_exit(NULL);
}

static void
usage(void)
{
    cout << "iporinad [OPTIONS]" << endl
         << "   -h : show this help" << endl
         << "   -c CONF_FILE: path to config file "
            "(default /etc/rina/iporinad.conf)"
         << endl;
}

int
main(int argc, char **argv)
{
    const char *confpath = "/etc/rina/iporinad.conf";
    struct pollfd pfd[1];
    pthread_t fa_th;
    int opt;

    while ((opt = getopt(argc, argv, "hc:v")) != -1) {
        switch (opt) {
        case 'h':
            usage();
            return 0;

        case 'c':
            confpath = optarg;
            break;

        case 'v':
            g->verbose++;
            break;

        default:
            printf("    Unrecognized option %c\n", opt);
            usage();
            return -1;
        }
    }

    /* Check that 'ip' and 'ethtool' are available. */
    if (execute_command("which ip")) {
        cerr << "'ip' command not available, please install iproute2" << endl;
        return -1;
    }
    if (execute_command("which ethtool")) {
        cerr << "'ethtool' command not available, please install it" << endl;
        return -1;
    }

    /* Parse configuration file. */
    if (parse_conf(confpath)) {
        return -1;
    }

    if (g->verbose) {
        dump_conf();
    }

    /* Name registration and creation of TUN devices. */
    if (setup()) {
        return -1;
    }

    /* Start the threads that will carry out the forwarding work. */
    g->start_workers();

    /* Start a thread that periodically tries to connect to
     * the remote peers specified by the configuration. */
    if (pthread_create(&fa_th, NULL, connect_to_remotes, NULL)) {
        perror("pthread_create()");
        return -1;
    }

    /* Wait for incoming control/data connections from remote peers. */
    for (;;) {
        int cfd;
        int ret;

        pfd[0].fd     = g->rfd;
        pfd[0].events = POLLIN;
        ret           = poll(pfd, 1, -1);
        if (ret < 0) {
            perror("poll(lfd)");
            return -1;
        }

        if (!pfd[0].revents & POLLIN) {
            continue;
        }

        cfd = rina_flow_accept(g->rfd, NULL, NULL, 0);
        if (cfd < 0) {
            if (errno == ENOSPC) {
                continue;
            }
            perror("rina_flow_accept(lfd)");
            return -1;
        }

        if (g->verbose > 1) {
            cout << "Accepted new connection from remote peer" << endl;
        }

        CDAPConn conn(cfd);
        CDAPMessage *rm;
        CDAPMessage m;
        const char *objbuf;
        size_t objlen;
        Hello hello;
        string remote_name;

        /* CDAP server-side connection setup. */
        rm = conn.accept();
        if (!rm) {
            cerr << "Error while accepting CDAP connection" << endl;
            goto abor;
        }
        remote_name = rm->src_appl;
        delete rm;
        rm = NULL;

        /* Receive Hello or Data message. */
        rm = conn.msg_recv();
        if (rm->op_code != gpb::M_START) {
            cerr << "M_START expected" << endl;
            goto abor;
        }

        if (rm->obj_class == "data") {
            /* This is a data flow. */
            delete rm;
            rm = NULL;
            if (g->remotes.count(remote_name) == 0) {
                cerr << "M_START(data) for unknown remote" << endl;
                goto abor;
            }
            if (g->verbose) {
                cout << "M_START(data) received from " << remote_name << endl;
            }
            g->remotes[remote_name].rfd                          = cfd;
            g->remotes[remote_name].flow_alloc_needed[IPOR_DATA] = false;
            g->remotes[remote_name].mss_configure();
            /* Submit the new fd mapping to a worker thread. */
            g->workers[0]->submit(g->remotes[remote_name].rfd,
                                  g->remotes[remote_name].tun_fd);
            continue;
        }

        /* This is a control connection, process the Hello message. */
        rm->get_obj_value(objbuf, objlen);
        if (!objbuf) {
            cerr << "M_START(hello) does not contain a nested message" << endl;
            goto abor;
        }
        hello = Hello(objbuf, objlen);
        delete rm;
        rm = NULL;

        if (g->remotes.count(remote_name) == 0) {
            g->remotes[remote_name] = Remote();

            Remote &r = g->remotes[remote_name];

            r.app_name = remote_name;
            r.dif_name = string();
            if (r.tun_alloc()) {
                close(cfd);
                goto abor;
            }
        }

        g->remotes[remote_name].tun_local_addr  = IPAddr(hello.tun_dst_addr);
        g->remotes[remote_name].tun_remote_addr = IPAddr(hello.tun_src_addr);

        cout << "Hello received from " << remote_name << ": "
             << hello.num_routes << " routes, tun_subnet " << hello.tun_subnet
             << ", local IP "
             << static_cast<string>(g->remotes[remote_name].tun_local_addr)
             << ", remote IP "
             << static_cast<string>(g->remotes[remote_name].tun_remote_addr)
             << endl;

        /* Receive routes from peer. */
        for (unsigned int i = 0; i < hello.num_routes; i++) {
            RouteObj robj;
            size_t prevlen;

            rm = conn.msg_recv();
            if (rm->op_code != gpb::M_WRITE) {
                cerr << "M_WRITE expected" << endl;
                goto abor;
            }

            rm->get_obj_value(objbuf, objlen);
            if (!objbuf) {
                cerr << "M_WRITE does not contain a nested message" << endl;
                goto abor;
            }
            robj = RouteObj(objbuf, objlen);
            delete rm;
            rm = NULL;

            /* Add the route in the set. */
            prevlen = g->remotes[remote_name].routes.size();
            g->remotes[remote_name].routes.insert(Route(robj.route));
            if (g->remotes[remote_name].routes.size() > prevlen) {
                /* Log only if the route was added to the set. */
                cout << "Route " << robj.route << " reachable through "
                     << remote_name << endl;
            }
        }

        g->remotes[remote_name].ip_configure();

    abor:
        close(cfd);
    }

    pthread_exit(NULL);

    return 0;
}
