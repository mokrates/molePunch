#include "molepunch.h"

// TODO
// molepunch.org

unsigned short serverport=4223;
ipaddress serveraddress = 0;

unsigned short server2port = 4223;
ipaddress server2address = 0;

int udp_socket;
int verbose = 0; // log packets
unsigned short sourceport = 0;
int dont_ask_own   = 0;   // don't ask server for own-ip
int dont_ask_other = 0;   // don't ask for peer ip

char devname[IFNAMSIZ] = "mp%d"; // name for tun-device

ipaddress ownip;        // this is the address which the server tells us we use
unsigned short ownport; // this is the port which the server tells us we use - HOST byte order

ipaddress otherip;      // this is the address which the user tells us to connect to
unsigned short otherport;  // this is the port which the user tells us to connect to - HOST byte order

int tunfd;

const char *usage = "molepunchclient \n"
  "[ -s <mp-server-address> ] \n"
  "[ -p <mp-server-port> ] \n"
  "[ -v ]                       # verbose \n"
  "[ -S <source-port> ]         # maybe not all ports are free, try a low one\n"
  "[ -2s <mp2-server-address> ] # second molepunch-server. check that both server's answers match!\n"
  "[ -2p <mp2-server-port> ]    # second molepunch-port."
  //  "[ -C <peer ip> <peer port> ] # don't ask mp-server for own-ip \n"
  //  "[ -a ]                       # don't ask for the peer address, use arriving first packet info\n"
  "############################## \n"
  " Either you have to use -s on both sides, or -1 on one and -f on the other side";

void init_socket(void) {
  struct sockaddr_in sa;
  
  // create socket
  
  udp_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  fcntl(udp_socket, F_SETFL, O_NONBLOCK);

  sa = make_sockaddr(INADDR_ANY, sourceport); // try sourceport, by default 0 (any free port)
    
  if (-1 == bind(udp_socket, (struct sockaddr *)&sa, sizeof sa)) {
    perror("molePunchClient");
    exit(1);
  }
}

void do_params(int argc, char **argv)
{
  for (;argc--; argv++) {
    if ((*argv)[0] == '-')
      switch ((*argv)[1]) {
      case 's':	// select listener_port
	serveraddress = inet_addr(*(++argv));
	//serveraddress = gethostbyname();  // TODO
	--argc;
	break;

      case 'p':
	serverport = atoi(*(++argv));
	--argc;
	break;

      case '2':
	switch ((*argv)[2]) {
	case 's':
	  server2address = inet_addr(*(++argv));
	  //serveraddress = gethostbyname();  // TODO
	  --argc;
	  break;

	case 'p':
	  server2port = atoi(*(++argv));
	  --argc;
	  break;

	default:
	  fprintf(stderr, "%s\n", usage);
	  exit(1);
	  break;
	}
	break;

      case 'v':
	verbose = 1;
	break;

      case 'S':
	sourceport = atoi(*(++argv));
	--argc;
	break;

      default:
	fprintf(stderr, "%s\n", usage);
	exit(1);
	break;
      }
  }

  if (serveraddress == 0 && ! dont_ask_own) {
	fprintf(stderr, "%s\n", usage);
	exit(1);
  }
}

// get address of peer from userinput. uses scanf. unsafe?
// sets otherip, otherport
void get_other_address(void)
{
  char otheripbuf[20];
  
  printf("please enter the peer address [x.x.x.x port]: "); fflush(stdout);
  scanf("%16s %hu", otheripbuf, &otherport);

  otherip = inet_addr(otheripbuf);
}

// get own address via molePunchServer
void get_own_address(void)
{
  char packet_buffer[1024];
  char *molepunch_message = "molepunch";
  struct sockaddr_in sa;
  socklen_t sl = sizeof sa;
  struct pollfd pfd;
  
  printf("getting own address...\n");

  pfd.fd = udp_socket;
  pfd.events = POLLIN;

  for (;;) {  // try to get own IP from molepunchserver
    sa = make_sockaddr(serveraddress, serverport);
    sendto(udp_socket, molepunch_message, strlen(molepunch_message), 0, (void *)&sa, sl);

    if (poll(&pfd, 1, 1000)) {
      sl = sizeof sa;
      if (-1 != recv(udp_socket, packet_buffer, 1024, MSG_DONTWAIT)) {
	ownip = ((struct molepunch_address *)&packet_buffer)->ip;
	ownport = ntohs((((struct molepunch_address *)&packet_buffer)->port));

	printf("got response: we are %s %i\n", my_inet_ntoa(ownip), ownport);

	return;
      }
    }
  }
}

/////////////////////////////////////////////////////////
// copied from https://www.kernel.org/doc/Documentation/networking/tuntap.txt
// corrected style

int tun_alloc(char *dev)
{
  struct ifreq ifr;
  int fd, err;
  
  if ((fd = open("/dev/net/tun", O_RDWR)) < 0)
    perror("open(/dev/net/tun) < 0");
  
  memset(&ifr, 0, sizeof(ifr));
  
  /* Flags: IFF_TUN   - TUN device (no Ethernet headers) 
   *        IFF_TAP   - TAP device  
   *
   *        IFF_NO_PI - Do not provide packet information  
   */ 
  ifr.ifr_flags = IFF_TUN | IFF_NO_PI;   //Mo: ohne IFF_NO_PI hatte ich ein padding-Problem.
  //ifr.ifr_mtu   = 1500;      // Mo: Hinzugefuegt.
  
  if (*dev)
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  
  if ((err = ioctl(fd, TUNSETIFF, (void *) &ifr)) < 0) {
    close(fd);
    return err;
  }
  strcpy(dev, ifr.ifr_name);
  return fd;
}

//////////////////////////////////////////////////////////////////////

void mainloop(void)
{
  struct pollfd pfd[2];
  char packet[PACKET_SIZE];
  struct sockaddr_in sa;    // peer-address
  socklen_t sl = sizeof sa;

  int packetlen;
  int timeout = 5000;  // 1/10 second (faster punch until connected)

  pfd[0].fd = tunfd;
  pfd[0].events = POLLIN;

  pfd[1].fd = udp_socket;
  pfd[1].events = POLLIN;

  sa = make_sockaddr(otherip, otherport);
  if (-1 == connect(udp_socket, (void *)&sa, sizeof sa))
    perror("molePunchClient");

  for (;;) {
    poll(pfd, 2, timeout);  // two fds
    if (pfd[0].revents & POLLIN) { // tunfd can be read
      packetlen = read(tunfd, packet, sizeof packet);
      if (-1 == packetlen)
	continue;

      if (verbose)
	printf("packet sent, len = %u\n", packetlen);

      /* sa = make_sockaddr(otherip, otherport); */
      /* if (-1 == sendto(udp_socket, packet, packetlen, 0, (void *)&sa, sl)) */
      /* 	perror("molePunchClient"); */
      if (-1 == write(udp_socket, packet, packetlen))
	perror("molePunchClient");
    }
    
    else if (pfd[1].revents & POLLIN) {  // udp_socket can be read
      packetlen = recvfrom(udp_socket, packet, PACKET_SIZE,
			   MSG_DONTWAIT, (void *)&sa, &sl);
      if (-1 == packetlen)
	continue;

      timeout = 5000;  // once we received a packet, we can wait 5 seconds without keepalive

      if (verbose)
	printf("packet received, len = %u\n", packetlen);

      if (-1 == write(tunfd, packet, packetlen))
	perror("molePunchClient");
    }

    else {
      // no communication since timeout seconds
      // it should be no problem to just send malformed data, because this
      // will be dropped anyways as it can't be parsed as a packet on the other side.
      char *ping_package = "molepunch_ping";

      if (verbose)
	printf("ping\n");
      
      sa = make_sockaddr(otherip, otherport);
      if (-1 == sendto(udp_socket, ping_package, strlen(ping_package), 0, (void *)&sa, sl))
	perror("molePunchClient");
    }
  }
}

int main(int argc, char **argv)
{
  do_params(argc-1, argv+1);
  init_socket();

  get_own_address();
  if (server2address != 0) {
    serverport = server2port;
    serveraddress = server2address;
    get_own_address();
  }
  
  get_other_address();
  
  tunfd = tun_alloc(devname);
  printf("allocated netdev %s\n", devname);
  system("bash molepunch_conf.sh");
  printf("executed molepunch_conf.sh to configure %s\n", devname);

  mainloop();
}
