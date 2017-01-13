#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <string.h>


static int
get_mac_by_ifname(const char *ifname, uint8_t *hwaddr)
{
    struct ifreq ifr;
    int sock;

    if ((NULL == ifname) || (NULL == hwaddr)) {
        return -1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return -1;
    }

    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name) - 1);
    ifr.ifr_name[sizeof(ifr.ifr_name) - 1] = '\0';

    if (ioctl(sock, SIOCGIFHWADDR, &ifr) == -1) {
        perror("ioctl(SIOCGIFHWADDR)");
        return -1;
    }

    memcpy(hwaddr, ifr.ifr_ifru.ifru_hwaddr.sa_data, IFHWADDRLEN);

    close(sock);

    return 0;
}

int main(int argc, char *argv[])
{
    struct ifaddrs *ifaddr, *ifa;
    unsigned int scanfbuf[6];
    uint8_t mac_addr_one[6];
    uint8_t mac_addr_two[6];
    int n;

    if (argc < 2) {
        printf("    usage: mac2ifname MAC_ADDRESS\n");
        return -1;
    }

    n = sscanf(argv[1], "%x:%x:%x:%x:%x:%x",
                &scanfbuf[0],
                &scanfbuf[1],
                &scanfbuf[2],
                &scanfbuf[3],
                &scanfbuf[4],
                &scanfbuf[5]);
    if (n != 6) {
        printf("    invalid MAC address %s\n", argv[1]);
        return -1;
    }

    for (n = 0; n < 6; n++) {
        mac_addr_one[n] = (uint8_t)scanfbuf[n];
    }

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    /* Walk through linked list, maintaining head pointer so we
       can free list later */

    for (ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
        if (ifa->ifa_addr == NULL)
            continue;

        get_mac_by_ifname(ifa->ifa_name, mac_addr_two);
#if 0
        printf("%s %02x:%02x:%02x:%02x:%02x:%02x\n", ifa->ifa_name,
                mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3],
                mac_addr[4], mac_addr[5]);
#endif
        if (memcmp(mac_addr_one, mac_addr_two, sizeof(mac_addr_one)) == 0) {
            printf("%s\n", ifa->ifa_name);
            freeifaddrs(ifaddr);

            return 0;
        }
    }

    freeifaddrs(ifaddr);

    return -1;
}
