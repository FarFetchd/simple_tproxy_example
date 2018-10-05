# simple_tproxy_example

The simplest possible working example of TPROXY transparent proxying.

In transparent proxying, someone's path to the internet runs through you, and
you choose to have some/all of their traffic to remote hosts pass through your
logic. Maybe you just inspect it and forward it directly on to the intended
destination. Maybe you send it to the intended destination, but tunneled through
a VPN. Maybe you forward it, but alter one or both sides of the conversation.
Maybe you just outright impersonate the remote host! (Not that it has to be
sinister; maybe you're a hotel wifi login portal).

It's "transparent" because the client doesn't have to configure anything
(browser proxy settings, etc) to make it work: both the human and the device
they're using just see a "normal" internet connection.

TPROXY is an iptables + Linux kernel feature that makes transparent proxying
extremely straightforward: your code does a single exotic setsockopt(), and
then you bind() listen() accept() etc exactly the same as if you were writing
an ordinary TCP server. Behind the scenes, packets you send through your
accepted sockets will be spoofing the client's intended destination. Of course,
since you're working with ordinary socket file descriptors, you can plug right
into frameworks like libevent.

To run this example, you'll want one machine to be the proxy, and another to
be the client. On the proxy:
* Set up DHCP, giving the proxy itself out as a default gateway.
* Run the commands described in the .c file.
* `gcc -o tproxy_captive_portal tproxy_captive_portal.c`
* `sudo ./tproxy_captive_portal 192.168.123.1`, replacing 192.168.123.1 
  appropriately.

Connect (by ethernet, wifi, whatever) the client to the proxy. Then, in a
browser on the client, try visiting http://11.22.33.44/whatever.html.

TODO: add instructions to configure DHCP properly to allow running this example.
I happen to currently use a Raspberry Pi as a wifi adapter for my desktop, so I
didn't have to go through that in order to write this tutorial.
