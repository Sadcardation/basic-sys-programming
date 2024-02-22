#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <string>

#include "utils.hpp"

#define PORT \
  "23456"  // port to which the server will be bound for neighboring connections

using namespace std;

int main(int argc, char* argv[]) {
  if (argc != 3) {
    cerr << "Usage: player <machine_name> <port_num>" << endl;
    exit(EXIT_FAILURE);
  }

  const char* machine_name = argv[1];
  const char* port_num = argv[2];
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

  fd_set master;    // master file descriptor list
  fd_set read_fds;  // temp file descriptor list for select()
  int fdmax;        // maximum file descriptor number
  int newfd;        // newly accept()ed socket descriptorp

  FD_ZERO(&master);  // clear the master and temp sets
  FD_ZERO(&read_fds);

  FD_SET(player_sk, &master);
  FD_SET(server_sk, &master);
  fdmax = server_sk;

  int init_messages_recv = 0;
  string received_msg;

  // info from ringmaster
  string total_players;
  string player_id;
  string host_player_addr;

  const string init_msg = port_num_socket(player_sk);
  if (init_msg.empty()) {
    cerr << "Failed to get port number of player" << endl;
    exit(EXIT_FAILURE);
  }
  send_all_str(server_sk, init_msg.c_str(), init_msg.length());

  // 0->left player (host) (player_id - 1), 1->right player (client) (player_id + 1)
  map<int, int> idx_to_socket;

  // loop for game initialization
  while (init_messages_recv < 3) {
    read_fds = master;  // copy it
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
      perror("select");
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {  // we got one!!
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
              break;
            case 3:
              host_player_addr = received_msg;
              // build connection with host player
              newfd = connect_player_serv(host_player_addr);
              if (newfd == -1) {
                cerr << "Failed to connect to host player" << endl;
                exit(EXIT_FAILURE);
              }
              FD_SET(newfd, &master);  // add to master set
              if (newfd > fdmax) {     // keep track of the max
                fdmax = newfd;
              }
              idx_to_socket[0] = newfd;
              break;
          }
          init_messages_recv += 1;
        } else if (i == player_sk) {
          // handle new connections
          addrlen = sizeof(remoteaddr);
          newfd = accept(player_sk, (struct sockaddr*)&remoteaddr, &addrlen);
          if (newfd == -1) {
            cerr << "Failed to accept new connection from Neighbor." << endl;
            exit(EXIT_FAILURE);
          } else {
            FD_SET(newfd, &master);  // add to master set
            if (newfd > fdmax) {     // keep track of the max
              fdmax = newfd;
            }
            idx_to_socket[1] = newfd;
          }
        }
      }
    }
  }

  // reminder the ringmaster that local players are ready
  const string ready_msg = "local_ready";
  send_all_str(server_sk, ready_msg.c_str(), ready_msg.length());

  // game loop
}