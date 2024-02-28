#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
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
  if (argc != 4) {
    cerr << "Usage: ringmaster <port_num> <num_players> <num_hops>" << endl;
    exit(EXIT_FAILURE);
  }

  const char *port_num = argv[1];
  const char *num_players = argv[2];
  const char *num_hops = argv[3];
  const string side = "server";

  if (atoi(num_players) <= 1) {
    cerr << "Number of players must be greater than 1." << endl;
    exit(EXIT_FAILURE);
  }
  if (atoi(num_hops) < 0 || atoi(num_hops) > 512) {
    cerr << "Number of hops must be between 0 and 512." << endl;
    exit(EXIT_FAILURE);
  }

  // prompt
  cout << "Potato Ringmaster" << endl;
  cout << "Players = " << num_players << endl;
  cout << "Hops = " << num_hops << endl;

  Potato potato(atoi(num_hops));
  vector<Player> players;

  int listener = socket_build(NULL, port_num, side);

  if (listener == -1) {
    cerr << "Failed to listen" << endl;
    exit(EXIT_FAILURE);
  }

  fd_set master;    // master file descriptor list
  fd_set read_fds;  // temp file descriptor list for select()
  int fdmax;        // maximum file descriptor number
  int newfd;        // newly accept()ed socket descriptor

  socklen_t addrlen;
  struct sockaddr_storage remoteaddr;

  string prior_info;

  // potential msg
  string total_players_msg;
  string player_id_msg;
  string left_info_msg;
  string right_info_msg;

  struct sockaddr_storage;

  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  FD_SET(listener, &master);
  fdmax = listener;

  int connected_players = 0;
  int all_ready = 0;

  // game initialization
  while (all_ready != atoi(num_players)) {
    read_fds = master;  // copy it
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
      cerr << "select" << endl;
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == listener) {
          // handle new connections
          addrlen = sizeof(remoteaddr);
          newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
          if (newfd == -1) {
            cerr << "Failed to accept new connection from Player." << endl;
            exit(EXIT_FAILURE);
          } else {
            FD_SET(newfd, &master);  // add to master set
            if (newfd > fdmax) {     // keep track of the max
              fdmax = newfd;
            }
            players.push_back(Player(connected_players, newfd, remoteaddr));
            connected_players++;
          }
        } else {
          // handle data from a client
          if (!recv_str_with_header(i, prior_info)) {
            close(i);            // bye!
            FD_CLR(i, &master);  // remove from master set
          } else if (prior_info == "local_ready") {
            int player_id = which_player_socket(i, players);
            if (player_id == -1) {
              cerr << "Failed to find player id." << endl;
              exit(EXIT_FAILURE);
            }
            all_ready += 1;
            cout << "Player " << player_id << " is ready to play" << endl;
          } else {
            int player_id = which_player_socket(i, players);
            if (player_id == -1) {
              cerr << "Failed to find player id." << endl;
              exit(EXIT_FAILURE);
            }
            players[player_id].set_server_port(prior_info);
            total_players_msg = num_players;
            if (!send_str_with_header(i, total_players_msg)) {
              cerr << "Failed to send total players." << endl;
              exit(EXIT_FAILURE);
            }

            player_id_msg = to_string(player_id);
            if (!send_str_with_header(i, player_id_msg)) {
              cerr << "Failed to send player id." << endl;
              exit(EXIT_FAILURE);
            }

            // build the ring
            // player_id - 1 is the "host" of the current player
            if (player_id > 0) {
              left_info_msg = serialize_addr(players[player_id - 1].get_addr());
              left_info_msg += ":" + players[player_id - 1].get_server_port();
              if (!send_str_with_header(i, left_info_msg)) {
                cerr << "Failed to send left info." << endl;
                exit(EXIT_FAILURE);
              }
            }

            // player_id = num_players - 1 is the "host" of the player_id = 0
            if (player_id == atoi(num_players) - 1) {
              left_info_msg = serialize_addr(players[player_id].get_addr());
              left_info_msg += ":" + players[player_id].get_server_port();
              if (!send_str_with_header(socket_of_player(0, players),
                                        left_info_msg)) {
                cerr << "Failed to send left info." << endl;
                exit(EXIT_FAILURE);
              }
            }
          }
        }
      }
    }
  }

  // game start
  bool game_over = false;
  string potato_msg;

  if (potato.get_hops() == 0) {
    send_end_msg(players);
    game_over = true;
  } else {
    // send potato to a random player
    srand((unsigned int)time(NULL) + stoul(num_players));
    int init_player = rand() % atoi(num_players);
    cout << "Ready to start the game, sending potato to player " << init_player
         << endl;

    potato_msg = potato.serialize();
    if (!send_str_with_header(socket_of_player(init_player, players),
                              potato_msg)) {
      cerr << "Failed to send potato to player " << init_player << endl;
      exit(EXIT_FAILURE);
    }
  }

  while (!game_over) {
    read_fds = master;  // copy it
    if (select(fdmax + 1, &read_fds, NULL, NULL, NULL) == -1) {
      cerr << "select" << endl;
      exit(EXIT_FAILURE);
    }

    for (int i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {
        if (i != listener) {
          if (!recv_str_with_header(i, potato_msg)) {
            cerr << "Failed to receive potato." << endl;
            exit(EXIT_FAILURE);
          }
          Potato p = Potato::deserialize(potato_msg);
          if (p.get_hops() == 0) {
            cout << "Trace of potato:" << endl;
            p.print_trace();
            // cout << "Game over" << endl;
            for (Player player : players) {
              if (!send_str_with_header(player.get_socket(), "game_over")) {
                cerr << "Failed to send game over." << endl;
                exit(EXIT_FAILURE);
              }
            }
            game_over = true;
          }
        }
      }
    }
  }

  // close all sockets and exit
  close(listener);
  return EXIT_SUCCESS;
}