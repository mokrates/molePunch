#include "molepunch.h"

unsigned short serverport=4223;
int udp_socket;

const char *usage = "molePunchServer [ -p port ]";

void init(void) {
  struct sockaddr_in sa;
  int val=1;
  
  // create socket
  
  udp_socket = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);

  sa = make_sockaddr(INADDR_ANY, serverport);
    
  if (-1 == bind(udp_socket, (struct sockaddr *)&sa, sizeof sa)) {
    perror("molePunchServer");
    exit(1);
  }
}

void do_params(int argc, char **argv)
{
  for (;argc--; argv++) {
    if ((*argv)[0] == '-')
      switch ((*argv)[1]) {
      case 'p':	// select listener_port
	serverport = atoi(*(++argv));
	--argc;
	break;
	
      default:
	fprintf(stderr, "%s\n", usage);
	exit(1);
	break;
      }
  }
}

int main(int argc, char **argv)
{
  char packet_buffer[1024];
  struct sockaddr_in sa;
  socklen_t sl = sizeof sa;
  struct timespec sleeptime = { 0, 100000 };
  struct molepunch_address ma;
  struct pollfd pfd;

  do_params(argc, argv);
  
  init();

  printf("molePunchServer listening on 0.0.0.0:%i\n", serverport);

  pfd.fd = udp_socket;
  pfd.events = POLLIN;

  for (;;) {
    poll(&pfd, 1, -1); // wait indefinitely

    sl = sizeof sa;
    if (-1 != recvfrom(udp_socket, packet_buffer, 1024, MSG_DONTWAIT, (struct sockaddr *)&sa, &sl)) {
      printf("got packet from %s:%i\n", ip_from_sockaddr(&sa), ntohs(sa.sin_port));
      
      ma.ip = sa.sin_addr.s_addr ;
      ma.port = sa.sin_port; // already in network-byte-order (alledgedly)

      sendto(udp_socket, &ma, sizeof ma, 0, (struct sockaddr *)&sa, sl);
    }
  }
}
