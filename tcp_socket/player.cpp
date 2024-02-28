#include <netdb.h>
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

  socklen_t addrlen;
  struct sockaddr_storage remoteaddr;

  fd_set master;   // master file descriptor list
  fd_set read_fds; // temp file descriptor list for select()
  int fdmax;       // maximum file descriptor number
  int newfd;       // newly accept()ed socket descriptorp

  FD_ZERO(&master); // clear the master and temp sets
  FD_ZERO(&read_fds);

  FD_SET(player_sk, &master);
  FD_SET(server_sk, &master);
  fdmax = player_sk > server_sk ? player_sk : server_sk;

  int init_messages_recv = 0;
  bool server_role = false;
  string received_msg;

  // info from ringmaster
  string total_players;
  string player_id;
  string host_player_addr;

  string init_msg = port_num_socket(player_sk);
  if (init_msg.empty()) {
    cerr << "Failed to get port number of player" << endl;
    exit(EXIT_FAILURE);
  }
  send_str_with_header(server_sk, init_msg);

  // 0->left player (host) (player_id - 1), 1->right player (client) (player_id
  // + 1)
  map<int, int> idx_to_socket;
  map<int, int> idx_to_id;

  // loop for game initialization
  while (init_messages_recv != 3 || !server_role) {
    read_fds = master; // copy it
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
      cerr << "select" << endl;
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) { // we got one!!
        if (i == server_sk) {
          if (!recv_str_with_header(server_sk, received_msg)) {
            cerr << "Failed to receive message" << endl;
            exit(EXIT_FAILURE);
          }
          switch (init_messages_recv) {
          case 0:
            total_players = received_msg;
            break;
          case 1:
            player_id = received_msg;
            cout << "Connected as player " + player_id + " out of " +
                        total_players + " total players"
                 << endl;
            idx_to_id[0] = stoi(player_id) - 1;
            idx_to_id[1] = stoi(player_id) + 1;
            if (stoi(player_id) == 0) {
              idx_to_id[0] = stoi(total_players) - 1;
            } else if (stoi(player_id) == stoi(total_players) - 1) {
              idx_to_id[1] = 0;
            }
            break;
          case 2:
            host_player_addr = received_msg;
            // build connection with host player
            newfd = connect_player_serv(host_player_addr);
            if (newfd == -1) {
              cerr << "Failed to connect to host player" << endl;
              exit(EXIT_FAILURE);
            }
            FD_SET(newfd, &master); // add to master set
            if (newfd > fdmax) {    // keep track of the max
              fdmax = newfd;
            }
            idx_to_socket[0] = newfd;
            break;
          }
          init_messages_recv += 1;
        } else if (i == player_sk) {
          // handle new connections
          addrlen = sizeof(remoteaddr);
          newfd = accept(player_sk, (struct sockaddr *)&remoteaddr, &addrlen);
          if (newfd == -1) {
            cerr << "Failed to accept new connection from Neighbor." << endl;
            exit(EXIT_FAILURE);
          } else {
            FD_SET(newfd, &master); // add to master set
            if (newfd > fdmax) {    // keep track of the max
              fdmax = newfd;
            }
            idx_to_socket[1] = newfd;
          }
          server_role = true;
        }
      }
    }
  }

  // reminder the ringmaster that local players are ready
  string ready_msg = "local_ready";
  send_str_with_header(server_sk, ready_msg);

  string potato_msg;

  // game loop
  bool game_over = false;
  while (!game_over) {
    read_fds = master; // copy it
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
      cerr << "select" << endl;
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) { // we got one!!
        if (!recv_str_with_header(i, potato_msg)) {
          cerr << "Failed to receive message" << endl;
          exit(EXIT_FAILURE);
        }
        if (potato_msg == "game_over") {
          game_over = true;
          break;
        } else {
          // receive potato (init player) and send to random neighbor
          Potato p = Potato::deserialize(potato_msg);
          p.add_trace(stoi(player_id));
          p.set_hops(p.get_hops() - 1);
          if (p.get_hops() == 0) {
            if (!send_str_with_header(server_sk, p.serialize())) {
              cerr << "Failed to send potato" << endl;
              exit(EXIT_FAILURE);
            }
            cout << "I'm it" << endl;
          } else {
            int neighbor = rand() % 2;
            cout << "Sending potato to " << idx_to_id[neighbor] << endl;
            if (!send_str_with_header(idx_to_socket[neighbor], p.serialize())) {
              cerr << "Failed to send potato" << endl;
              exit(EXIT_FAILURE);
            }
          }
        }
      }
    }
  }

  close(player_sk);
  close(server_sk);
  return EXIT_SUCCESS;
}