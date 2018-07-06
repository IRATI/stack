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
#include <memory>
#include <thread>
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
    int ip_cleanup() const;
    int mss_configure() const;
};

#define NUM_WORKERS 1

class IPoRINA {
    /* Control device to listen for incoming connections. */
    int rfd = -1;

    /* Daemon configuration. */
    Local local;
    map<string, Remote> remotes;
    list<Route> local_routes;

    /* Map to keep track of active sessions. Used to react on
     * session termination. */
    map<FwdToken, string> active_sessions;
    FwdToken next_submit_token = 1;

    /* Worker threads that forward traffic. */
    vector<std::unique_ptr<FwdWorker>> workers;

public:
    IPoRINA();
    ~IPoRINA();

    /* Enable verbose mode */
    int verbose = 0;

    /* QoS parameters */
    int max_delay = 0;
    int max_loss  = RINA_FLOW_SPEC_LOSS_MAX;

    /* Tun device tx queue length */
    int tx_q_len = 0;

    void start_workers();
    int setup();
    int main_loop();
    int parse_conf(const char *path);
    void dump_conf();
    void connect_to_remotes();
    int submit(Remote *r);
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
    char objbuf[4096]; /* Don't change the scope of this buffer. */
    int objlen;

    if (obj) {
        objlen = obj->serialize(objbuf, sizeof(objbuf));
        if (objlen < 0) {
            errno = EINVAL;
            fprintf(stderr, "serialization failed\n");
            return objlen;
        }

        m->set_obj_value(objbuf, objlen); /* no ownership passing */
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

IPoRINA::IPoRINA() : rfd(-1), verbose(0) {}

void
IPoRINA::start_workers()
{
    for (int i = 0; i < NUM_WORKERS; i++) {
        workers.push_back(
            std::unique_ptr<FwdWorker>(new FwdWorker(i, verbose)));
    }
}

IPoRINA::~IPoRINA() {}

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
    ip_cleanup();

    {
        /* Disable TSO and GSO. */
        stringstream cmdss;

        cmdss << "ethtool -K " << tun_name << " tso off gso off";
        if (execute_command(cmdss)) {
            cerr << "Failed to disable TSO and GSO for " << tun_name << endl;
            return -1;
        }
    }

    if (g->tx_q_len) {
	    /* Set tx queue length */
	    stringstream cmdss;

	    cmdss << "ip link set " << tun_name << " txqueuelen " << g->tx_q_len;
	    if (execute_command(cmdss)) {
		    cerr << "Failed to set txqueuelen" << endl;
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
    for (const auto &route : routes) {
        stringstream cmdss;

        cmdss << "ip route add " << static_cast<string>(route.subnet) << " via "
              << tun_remote_addr.noprefix() << " dev " << tun_name;
        if (execute_command(cmdss)) {
            cerr << "Failed to add route for subnet "
                 << static_cast<string>(route.subnet) << endl;
            return -1;
        }
    }

    return 0;
}

int
Remote::ip_cleanup() const
{
    {
        /* Flush previous routes, if any. */
        stringstream cmdss;

        cmdss << "ip route flush dev " << tun_name;
        if (execute_command(cmdss)) {
            cerr << "Failed to flush routes for interface " << tun_name << endl;
            return -1;
        }
    }

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

int
IPoRINA::parse_conf(const char *path)
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

            if (local.app_name.size()) {
                cerr << "Duplicated 'local' directive at line " << lines_cnt
                     << endl;
                return -1;
            }

            local = Local(tokens[1]);
            for (unsigned int i = 2; i < tokens.size(); i++) {
                local.dif_names.push_back(tokens[i]);
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

            if (remotes.count(tokens[1])) {
                cerr << "Duplicated 'remote' directive at line " << lines_cnt
                     << endl;
                return -1;
            }
            remotes[tokens[1]] = Remote(tokens[1], tokens[2], subnet);

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

            local_routes.push_back(Route(subnet));
        }
    }

    fin.close();

    return 0;
}

void
IPoRINA::dump_conf()
{
    cout << "Local: " << local.app_name << " in DIFs";
    for (const auto &dif_name : local.dif_names) {
        cout << " " << dif_name;
    }
    cout << endl;

    cout << "Remotes:" << endl;
    for (const auto &kv : remotes) {
        cout << "   " << kv.second.app_name << " in DIF " << kv.second.dif_name
             << ", tunnel prefix " << kv.second.tun_subnet.repr << endl;
    }

    cout << "Advertised routes:" << endl;
    for (const auto &route : local_routes) {
        cout << "   " << route.subnet.repr << endl;
    }
}

int
IPoRINA::setup(void)
{
    rfd = rina_open();
    if (rfd < 0) {
        perror("rina_open()");
        return -1;
    }

    /* Register us to one or more local DIFs. */
    for (const auto &dif_name : local.dif_names) {
        int ret;

        ret = rina_register(rfd, dif_name.c_str(), local.app_name.c_str(), 0);
        if (ret) {
            perror("rina_register()");
            cerr << "Failed to register " << local.app_name << " in DIF "
                 << dif_name << endl;
            return -1;
        }
    }

    /* Create a TUN device for each remote. */
    for (auto &kv : remotes) {
        if (kv.second.tun_alloc()) {
            return -1;
        }
    }

    return 0;
}

int
IPoRINA::submit(Remote *r)
{
    int dupfd;

    dupfd = dup(r->tun_fd);
    if (dupfd < 0) {
        perror("dup(tun_fd)");
        return dupfd;
    }
    /* Duplicate the tun_fd, since FwdWorker::submit() consumes it and
     * we want the TUN device to survive. */
    workers[0]->submit(next_submit_token, r->rfd, dupfd);
    r->rfd = -1; /* ownership passing, we won't need this anymore */
    active_sessions[next_submit_token++] = r->app_name;

    return 0;
}

int
IPoRINA::main_loop()
{
    /* Wait for incoming control/data connections from remote peers, and
     * also for terminating sessions. */
    for (;;) {
        FwdWorker *const worker = workers[0].get();
        struct pollfd pfd[2];
        int cfd;
        int ret;

        pfd[0].fd     = rfd;
        pfd[1].fd     = worker->closed_eventfd();
        pfd[0].events = pfd[1].events = POLLIN;
        ret = poll(pfd, sizeof(pfd) / sizeof(pfd[0]), -1);
        if (ret < 0) {
            perror("poll(lfd)");
            return -1;
        } else if (ret == 0) {
            /* Timeout or spuriorus notification. */
            continue;
        }

        if (pfd[1].revents & POLLIN) {
            /* Some sessions terminated. */
            FwdToken token;

            while ((token = worker->get_next_closed()) != 0) {
                if (!active_sessions.count(token) ||
                    !remotes.count(active_sessions[token])) {
                    cerr << "Failed to match completed session (token=" << token
                         << ")" << endl;
                } else {
                    Remote &r = remotes[active_sessions[token]];
                    cout << "Remote " << active_sessions[token]
                         << " disconnected" << endl;
                    active_sessions.erase(token);
                    r.ip_cleanup();
                    /* Trigger flow reallocation towards the peer. */
                    r.flow_alloc_needed[IPOR_CTRL] =
                        r.flow_alloc_needed[IPOR_DATA] = true;
                }
            }
            continue;
        }

        /* New incoming connection. */
        assert(pfd[0].revents & POLLIN);
        cfd = rina_flow_accept(rfd, NULL, NULL, 0);
        if (cfd < 0) {
            if (errno == ENOSPC) {
                continue;
            }
            perror("rina_flow_accept(lfd)");
            return -1;
        }

        if (verbose > 1) {
            cout << "Accepted new connection from remote peer" << endl;
        }

        CDAPConn conn(cfd);
        std::unique_ptr<CDAPMessage> rm;
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

        /* Receive Hello or Data message. */
        rm = conn.msg_recv();
        if (rm->op_code != gpb::M_START) {
            cerr << "M_START expected" << endl;
            goto abor;
        }

        if (rm->obj_class == "data") {
            /* This is a data flow. */
            rm = NULL;
            if (remotes.count(remote_name) == 0) {
                cerr << "M_START(data) for unknown remote" << endl;
                goto abor;
            }
            if (verbose) {
                cout << "M_START(data) received from " << remote_name << endl;
            }
            remotes[remote_name].rfd                          = cfd;
            remotes[remote_name].flow_alloc_needed[IPOR_DATA] = false;
            remotes[remote_name].mss_configure();
            /* Submit the new fd mapping to a worker thread. */
            submit(&remotes[remote_name]);
            continue;
        }

        /* This is a control connection, process the Hello message. */
        rm->get_obj_value(objbuf, objlen);
        if (!objbuf) {
            cerr << "M_START(hello) does not contain a nested message" << endl;
            goto abor;
        }
        hello = Hello(objbuf, objlen);

        if (remotes.count(remote_name) == 0) {
            remotes[remote_name] = Remote();

            Remote &r = remotes[remote_name];

            r.app_name = remote_name;
            r.dif_name = string();
            if (r.tun_alloc()) {
                close(cfd);
                goto abor;
            }
        }

        remotes[remote_name].tun_local_addr  = IPAddr(hello.tun_dst_addr);
        remotes[remote_name].tun_remote_addr = IPAddr(hello.tun_src_addr);

        cout << "Hello received from " << remote_name << ": "
             << hello.num_routes << " routes, tun_subnet " << hello.tun_subnet
             << ", local IP "
             << static_cast<string>(remotes[remote_name].tun_local_addr)
             << ", remote IP "
             << static_cast<string>(remotes[remote_name].tun_remote_addr)
             << endl;

        /* Receive routes from peer. */
        remotes[remote_name].routes.clear();
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

            /* Add the route in the set. */
            prevlen = remotes[remote_name].routes.size();
            remotes[remote_name].routes.insert(Route(robj.route));
            if (remotes[remote_name].routes.size() > prevlen) {
                /* Log only if the route was added to the set. */
                cout << "Route " << robj.route << " reachable through "
                     << remote_name << endl;
            }
        }

        remotes[remote_name].ip_configure();

    abor:
        close(cfd);
    }

    return 0;
}

/* Try to connect to all the user-specified remotes. */
void
IPoRINA::connect_to_remotes()
{
    string myname = local.app_name;

    for (;;) {
        for (auto &kv : remotes) {
            for (unsigned i = 0; i < IPOR_MAX; i++) {
                struct rina_flow_spec spec;
                struct pollfd pfd;
                int rfd;
                int ret;
                int wfd;

                if (!kv.second.flow_alloc_needed[i]) {
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
                }
                spec.max_loss  = (uint16_t)g->max_loss;
                spec.max_delay = (uint32_t)g->max_delay;
                wfd            = rina_flow_alloc(
                    kv.second.dif_name.c_str(), myname.c_str(),
                    kv.second.app_name.c_str(), &spec, RINA_F_NOWAIT);
                if (wfd < 0) {
                    perror("rina_flow_alloc()");
                    cout << "Failed to connect to remote " << kv.second.app_name
                         << " through DIF " << kv.second.dif_name << endl;
                    continue;
                }
                pfd.fd     = wfd;
                pfd.events = POLLIN;
                ret        = poll(&pfd, 1, 3000);
                if (ret <= 0) {
                    if (ret < 0) {
                        perror("poll(wfd)");
                    } else if (verbose) {
                        cout << "Failed to connect to remote "
                             << kv.second.app_name << " through DIF "
                             << kv.second.dif_name << endl;
                    }
                    close(wfd);
                    continue;
                }

                rfd = rina_flow_alloc_wait(wfd);
                if (rfd < 0) {
                    if (errno != EPERM || verbose > 1) {
                        if (errno != EPERM) {
                            perror("rina_flow_alloc_wait()");
                        }
                        cout << "Failed to connect to remote "
                             << kv.second.app_name << " through DIF "
                             << kv.second.dif_name << endl;
                    }
                    continue;
                }

                if (verbose > 1) {
                    cout << "Flow allocated to remote " << kv.second.app_name
                         << " through DIF " << kv.second.dif_name << endl;
                }

                CDAPConn conn(rfd);
                CDAPMessage m;
                std::unique_ptr<CDAPMessage> rm;
                Hello hello;

                /* CDAP connection setup. */
                if (conn.connect(myname, kv.second.app_name, gpb::AUTH_NONE,
                                 NULL)) {
                    cerr << "CDAP connection failed" << endl;
                    goto abor;
                }

                if (i == IPOR_DATA) {
                    /* This is a data connection, send an empty CDAP start
                     * message to inform the remote peer. */
                    m.m_start("data", "/data");
                    if (cdap_obj_send(&conn, &m, 0, NULL) < 0) {
                        cerr << "Failed to send M_START(data)" << endl;
                        goto abor;
                    }
                    kv.second.rfd = rfd;
                    kv.second.mss_configure();

                    /* Submit the new fd mapping to a worker thread. */
                    submit(&kv.second);

                } else {
                    /* This is a control connection. */

                    /* Assign tunnel IP addresses if needed. */
                    if (kv.second.tun_local_addr.empty() ||
                        kv.second.tun_remote_addr.empty()) {
                        kv.second.tun_local_addr =
                            kv.second.tun_subnet.hostaddr(1);
                        kv.second.tun_remote_addr =
                            kv.second.tun_subnet.hostaddr(2);
                    }

                    /* Exchange routes. */
                    m.m_start("hello", "/hello");
                    hello.num_routes   = local_routes.size();
                    hello.tun_subnet   = kv.second.tun_subnet;
                    hello.tun_src_addr = kv.second.tun_local_addr;
                    hello.tun_dst_addr = kv.second.tun_remote_addr;
                    if (cdap_obj_send(&conn, &m, 0, &hello) < 0) {
                        cerr << "Failed to send M_START(hello)" << endl;
                        goto abor;
                    }

                    for (const auto &route : local_routes) {
                        RouteObj robj(route.subnet);

                        m.m_write("route", "/routes");
                        if (cdap_obj_send(&conn, &m, 0, &robj) < 0) {
                            cerr << "Failed to send M_WRITE" << endl;
                            goto abor;
                        }
                    }
                    kv.second.ip_configure();
                }

                kv.second.flow_alloc_needed[i] = false;
            abor:
                /* Don't close a data file descriptor which is going
                 * to be used. */
                if (i == IPOR_CTRL) {
                    close(rfd);
                }
            }
        }

        sleep(3);
    }
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
usage(void)
{
    cout << "iporinad [OPTIONS]" << endl
         << "   -h : show this help" << endl
         << "   -c CONF_FILE: path to config file "
            "(default /etc/rina/iporinad.conf)" << endl
         << "   -L NUM : maximum loss probability introduced by the flow "
            "(NUM/"
         << RINA_FLOW_SPEC_LOSS_MAX << ")" << endl
         << "   -E NUM : maximum delay introduced by the flow (microseconds)"
         << endl
	 << "   -t NUM : tunnel TUN device tx queue length (packets)"
	 << endl
         << "   -v : be verbose" << endl;
}

int
main(int argc, char **argv)
{
    const char *confpath = "/etc/rina/iporinad.conf";
    int background       = 0;
    int opt;

    while ((opt = getopt(argc, argv, "hc:vL:E:t:w")) != -1) {
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

        case 'L':
            g->max_loss = atoi(optarg);
            if (g->max_loss < 0 || g->max_loss > RINA_FLOW_SPEC_LOSS_MAX) {
                cout << "    Invalid 'max loss' " << g->max_loss << endl;
                return -1;
            }
            break;

        case 'E':
            g->max_delay = atoi(optarg);
            if (g->max_delay < 0 || g->max_delay > 5000000) {
                cout << "    Invalid 'max delay' " << g->max_delay << endl;
                return -1;
            }
            break;

        case 't':
            g->tx_q_len = atoi(optarg);
            if (g->tx_q_len < 0 || g->tx_q_len > 10000) {
                cout << "    Invalid 'tx_q_len' " << g->tx_q_len << endl;
                return -1;
            }
            break;

        case 'w':
            background = 1;
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
    if (g->parse_conf(confpath)) {
        return -1;
    }

    if (g->verbose) {
        g->dump_conf();
    }

    /* Name registration and creation of TUN devices. */
    if (g->setup()) {
        return -1;
    }

    if (background) {
        daemonize();
    }

    /* Start the threads that will carry out the forwarding work. */
    g->start_workers();

    /* Start a thread that periodically tries to connect to
     * the remote peers specified by the configuration. */
    std::thread fa_th(&IPoRINA::connect_to_remotes, g);

    return g->main_loop();
}
