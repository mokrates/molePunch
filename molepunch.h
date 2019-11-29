#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sched.h>
#include <poll.h>
#include <arpa/inet.h>
#include <poll.h>

#include <linux/if.h>
#include <linux/if_tun.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netdb.h>

#include <time.h>

// #include <sys/random.h>  // to use /dev/urandom

#define PACKET_SIZE 2048

typedef unsigned long ipaddress;
struct molepunch_address {
  uint32_t ip;         // in network-byte-order
  uint16_t port;      // in network-byte-order
} __attribute__((packed));

struct sockaddr_in make_sockaddr(ipaddress ip, unsigned short port);
char *ip_from_sockaddr(struct sockaddr_in *sa);

// need this, because ips a struct in_addrs an not just ints.
char *my_inet_ntoa(unsigned long ip);
