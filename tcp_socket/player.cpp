#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <string>

#include "utils.hpp"

// Refer to code in below resources:
// https://beej.us/guide/bgnet/examples/selectserver.c
// https://beej.us/guide/bgnet/examples/client.c
// https://beej.us/guide/bgnet/examples/listener.c
// https://beej.us/guide/bgnet/examples/server.c
// https://beej.us/guide/bgnet/examples/talker.c
// https://canvas.duke.edu/files/867349/ (06 - tcp_example.zip)

using namespace std;

int main(int argc, char *argv[]) {
  if (argc != 3) {
    cerr << "Usage: player <machine_name> <port_num>" << endl;
    exit(EXIT_FAILURE);
  }

  const char *machine_name = argv[1];
  const char *port_num = argv[2];
  const string side = "player";

  int player_sk = socket_build(NULL, "", "server");
  int server_sk = socket_build(machine_name, port_num, side);

  if (server_sk == -1) {
    cerr << "Failed to connect" << endl;
    exit(EXIT_FAILURE);
  }

  if (player_sk == -1) {
    cerr << "Failed to open server" << endl;
    exit(EXIT_FAILURE);
  }

  string init_msg = port_num_socket(player_sk);
  if (init_msg.empty()) {
    cerr << "Failed to get port number of player" << endl;
    exit(EXIT_FAILURE);
  }
  send_str_with_header(server_sk, init_msg);

  socklen_t addrlen;
  struct sockaddr_storage remoteaddr;

  fd_set master;    // master file descriptor list
  fd_set read_fds;  // temp file descriptor list for select()
  int fdmax;        // maximum file descriptor number
  int newfd;        // newly accept()ed socket descriptorp

  FD_ZERO(&master);  // clear the master and temp sets
  FD_ZERO(&read_fds);

  FD_SET(player_sk, &master);
  FD_SET(server_sk, &master);
  fdmax = player_sk > server_sk ? player_sk : server_sk;

  // info from ringmaster
  string total_players;
  string player_id;
  string host_player_addr;

  // 0->left player (host) (player_id - 1), 1->right player (client) (player_id
  // + 1)
  map<int, int> idx_to_socket;
  map<int, int> idx_to_id;

  int player_id_int;
  int total_players_int;

  // receive info from ringmaster: total_players, player_id
  recv(server_sk, &player_id_int, sizeof(int), MSG_WAITALL);
  recv(server_sk, &total_players_int, sizeof(int), MSG_WAITALL);
  cout << "Connected as player " << player_id_int << " out of "
       << total_players_int << " total players" << endl;

  // build mapping with neighbors
  idx_to_id[0] = player_id_int - 1;
  idx_to_id[1] = player_id_int + 1;
  if (player_id_int == 0) {
    idx_to_id[0] = total_players_int - 1;
  } else if (player_id_int == total_players_int - 1) {
    idx_to_id[1] = 0;
  }

  // receive host player address and connect
  recv_str_with_header(server_sk, host_player_addr);
  newfd = connect_player_serv(host_player_addr);
  FD_SET(newfd, &master);  // add to master set
  if (newfd > fdmax) {     // keep track of the max
    fdmax = newfd;
  }
  idx_to_socket[0] = newfd;

  // accept connection from client player
  addrlen = sizeof(remoteaddr);
  newfd = accept(player_sk, (struct sockaddr *)&remoteaddr, &addrlen);
  FD_SET(newfd, &master);  // add to master set
  if (newfd > fdmax) {     // keep track of the max
    fdmax = newfd;
  }
  idx_to_socket[1] = newfd;

  // remind the ringmaster that local players are ready
  string ready_msg = "local_ready";
  send_str_with_header(server_sk, ready_msg);

  // game loop
  string potato_msg;
  while (true) {
    read_fds = master;  // copy it
    select(fdmax + 1, &read_fds, NULL, NULL, NULL);

    for (int i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {  // we got one!!
        recv_str_with_header(i, potato_msg);
        break;
      }
    }
    // receive potato (init player) and send to random neighbor
    Potato p = Potato::deserialize(potato_msg);
    if (p.get_hops() == -1) {
      break;
    }
    p.add_trace(player_id_int);
    p.set_hops(p.get_hops() - 1);
    if (p.get_hops() == 0) {
      send_str_with_header(server_sk, p.serialize());
      cout << "I'm it" << endl;
    } else {
      int neighbor = rand() % 2;
      cout << "Sending potato to " << idx_to_id[neighbor] << endl;
      send_str_with_header(idx_to_socket[neighbor], p.serialize());
    }
  }

  close(idx_to_socket[0]);
  close(server_sk);
  return EXIT_SUCCESS;
}