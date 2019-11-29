# molePunch #

molePunchVPN is my udp-hole-punch implementation.
All programs give help if called without or with the wrong arguments.

## Limitations ##

UDP hole punching works by first checking from which ip and port
oneself 'leaves' the local network into the internet. (what is the
return address for packets we send into the internet?)  There are
firewalls which translate to an external sender port just by looking
at the internal sender-ip and sender-port. These I should call the
nice firewalls.

There are firewalls, though, which translate to a sender port based
on - among other things - the port *we send our packets
to!*. Molepunch can not connect two sites even if only one of the
involved firewalls does this. Luckily, sometimes both firewalls behave
nicely.

