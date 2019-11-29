#include "molepunch.h"

struct sockaddr_in make_sockaddr(ipaddress ip, unsigned short port)
{
	struct sockaddr_in sa;
	
	sa.sin_family = AF_INET;
	sa.sin_port   = htons(port); // this is definitely correct, we couldn't connect otherwise.
	sa.sin_addr.s_addr = ip;
	
	return sa;
}

// returns the ip address as string
char *ip_from_sockaddr(struct sockaddr_in *sa)
{
  return inet_ntoa(sa->sin_addr);
}

// because inet_ntoa takes no ints. I want to give it an int.
char *my_inet_ntoa(unsigned long ip)
{
  struct sockaddr_in sa;
  sa.sin_addr.s_addr = (in_addr_t)ip;
  return ip_from_sockaddr(&sa);
}
