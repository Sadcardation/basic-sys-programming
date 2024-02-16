#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>

#include "utils.hpp"

int main(int argc, char* argv[]) {
  if (argc != 4) {
    cerr << "Usage: ringmaster <port_num> <num_players> <num_hops>" << endl;
    exit(EXIT_FAILURE);
  }

  const char* port_num = argv[1];
  const char* num_players = argv[2];
  const char* num_hops = argv[3];

  assert(atoi(num_players) > 1);
  assert(atoi(num_hops) > 0 && atoi(num_hops) <= 512);

  Potato potato(atoi(num_hops));

  // pre add trace
  for (int i = 0; i < atoi(num_hops); i++) {
    potato.add_trace(-1);
  }

  int listener = get_listener_socket(NULL, port_num);

  if (listener == -1) {
    cerr << "Failed to listen" << endl;
    exit(EXIT_FAILURE);
  }

  fd_set master;    // master file descriptor list
  fd_set read_fds;  // temp file descriptor list for select()
  int fdmax;        // maximum file descriptor number

  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  FD_SET(listener, &master);
  fdmax = listener;

  int connected_players = 0;
  while (connected_players != atoi(num_players)) {
    read_fds = master;  // copy it
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(EXIT_FAILURE);
    }
  }
}