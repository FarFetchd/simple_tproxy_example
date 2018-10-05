#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Before running this program, run these commands:
//
// 1) sudo iptables -t mangle -A PREROUTING -i eth0 -p tcp --dport 80 -m tcp \
//                  -j TPROXY --on-ip 192.168.123.1 --on-port 1234           \
//                  --tproxy-mark 1/1
// 2) sudo sysctl -w net.ipv4.ip_forward=1
// 3) sudo ip rule add fwmark 1/1 table 1
// 4) sudo ip route add local 0.0.0.0/0 dev lo table 1
// NOTE: I specified --dport 80 here to avoid messing up ssh etc. You could also
//       use an iptables -j ACCEPT ahead of this rule to make such exceptions.
//       It's much more interesting to leave --dport out, so your TPROXY
//       iptables rule will grab ALL the traffic! Especially if you also change
//       eth0 to wlan0 and run an open wifi access point ;)
// NOTE: don't forget to substitute your eth0's IP for 192.168.123.1!
//
// The above commands (#1) establish the TPROXY action; (#2) allow your system
// to forward packets from one interface to another; (#3) specify a special new
// routing table for packets given the 1/1 mark by TPROXY; and in that new
// routing table, (#4) redirect all packets to loopback, so your listener can
// see them. The redirect to lo is necessary because when the packets come in
// eth0 with a non-local dest address, they're not considered for delivery to
// local sockets, just forwarding logic.


// Although it goes into the kernel's machinery as if it were a real port, it's
// not, or at least not in a way that's relevant to us TPROXY-using coders. The
// value you pick has absolutely no relation to either src or dst ports in any
// packets that you exchange with the client. It just needs to match --on-port.
#define TPROXY_BIND_PORT 1234

void crash(const char* msg)
{
  perror(msg);
  exit(1);
}

int main(int argc, char** argv)
{
  if (argc != 2)
  {
    printf("usage: tproxy_captive_portal <ip addr to bind>\n\n" \
           "IMPORTANT: you must bind to the same address + port you passed\n" \
           "via --on-ip and --on-port to the TPROXY iptables rule. Use the\n" \
           "IP address of the interface that traffic will come in through.\n");
    exit(1);
  }
  int listener_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (listener_fd < 0)
    crash("couldn't allocate socket");
  const int yes = 1;
  if (setsockopt(listener_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
    crash("SO_REUSEADDR failed");
  if (setsockopt(listener_fd, SOL_IP, IP_TRANSPARENT, &yes, sizeof(yes)) < 0)
    crash("IP_TRANSPARENT failed. Are you root?");

  struct sockaddr_in bind_addr;
  bind_addr.sin_family = AF_INET;
  inet_pton(AF_INET, argv[1], &bind_addr.sin_addr.s_addr);
  bind_addr.sin_port = htons(TPROXY_BIND_PORT);
  if (bind(listener_fd, (struct sockaddr*)&bind_addr, sizeof(bind_addr)) < 0)
    crash("bind failed");

  if (listen(listener_fd, 10) < 0)
    crash("listen failed");
  printf("now TPROXY-listening on %s, '''''port''''' %d",
         argv[1], TPROXY_BIND_PORT);
  printf("...\nbut actually accepting any TCP SYN with dport 80, regardless of"\
         " dest IP, that hits the loopback interface!\n\n");
  while (1)
  {
    // client_addr will contain the client's IP+port, just like any other
    // TCP connection accept()ance.
    // intended_dest_addr will contain the IP+port that the client THINKS it's
    // talking to.
    struct sockaddr_in client_addr, intended_dest_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    socklen_t dest_addr_len = sizeof(intended_dest_addr);

    int client_fd = accept(listener_fd, (struct sockaddr*)&client_addr,
                                                          &client_addr_len);
    getsockname(client_fd, (struct sockaddr*)&intended_dest_addr,
                                             &dest_addr_len);
    printf("accepted socket from %s:%d; ",
           inet_ntoa(client_addr.sin_addr),
           ntohs(client_addr.sin_port));
    printf("they think they're talking to %s:%d\n",
           inet_ntoa(intended_dest_addr.sin_addr),
           ntohs(intended_dest_addr.sin_port));
    char http_response[1000];
    sprintf(http_response,
            "HTTP/1.0 200 OK\r\n\r\nYou thought you were connecting "
            "to %s:%d, but it was actually me, TPROXY!",
            inet_ntoa(intended_dest_addr.sin_addr),
            ntohs(intended_dest_addr.sin_port));
    send(client_fd, http_response, strlen(http_response), 0);

    shutdown(client_fd, SHUT_RDWR);
    close(client_fd);
  }
}
