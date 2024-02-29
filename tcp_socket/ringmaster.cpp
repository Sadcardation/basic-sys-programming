#include <netdb.h>
#include <sys/select.h>
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
  int num_players_int = atoi(num_players);

  if (num_players_int <= 1) {
    cerr << "Number of players must be greater than 1." << endl;
    return EXIT_FAILURE;
  }
  if (atoi(num_hops) < 0 || atoi(num_hops) > 512) {
    cerr << "Number of hops must be between 0 and 512." << endl;
    return EXIT_FAILURE;
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

  fd_set master;         // master file descriptor list
  fd_set read_fds;       // temp file descriptor list for select()
  int newfd;             // newly accept()ed socket descriptor
  int fdmax = listener;  // maximum file descriptor number

  FD_ZERO(&master);  // clear the master and temp sets
  FD_ZERO(&read_fds);

  socklen_t addrlen;
  struct sockaddr_storage remoteaddr;

  string prior_info;

  // potential msg

  struct sockaddr_storage;

  int connected_players = 0;

  // manage player connections
  while (connected_players < num_players_int) {
    addrlen = sizeof(remoteaddr);
    newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
    FD_SET(newfd, &master);  // add to master set
    if (newfd > fdmax) {
      fdmax = newfd;
    }
    players.push_back(Player(connected_players, newfd, remoteaddr));
    recv_str_with_header(newfd, prior_info);
    players[connected_players].set_server_port(prior_info);
    connected_players += 1;
  }

  // send start info to players
  string addr_info_msg;
  int player_id;
  for (Player player : players) {
    player_id = player.get_id();
    send(player.get_socket(), &player_id, sizeof(int), 0);
    send(player.get_socket(), &num_players_int, sizeof(int), 0);
  }

  // send neighbor info to players
  for (Player player : players) {
    player_id = player.get_id();
    addr_info_msg = serialize_addr(player.get_addr());
    addr_info_msg += ":" + player.get_server_port();
    if (player_id != num_players_int - 1) {
      send_str_with_header(players[player_id + 1].get_socket(), addr_info_msg);
    } else {
      send_str_with_header(players[0].get_socket(), addr_info_msg);
    }
  }

  // make sure all players are ready
  string ready_msg;
  for (Player player : players) {
    recv_str_with_header(player.get_socket(), ready_msg);
    cout << "Player " << player.get_id() << " is ready to play" << endl;
  }

  // game start
  string potato_msg;

  if (potato.get_hops() == 0) {
    send_end_msg(players);
    return EXIT_SUCCESS;
  } else {
    // send potato to a random player
    srand((unsigned int)time(NULL) + stoul(num_players));
    int init_player = rand() % num_players_int;
    cout << "Ready to start the game, sending potato to player " << init_player
         << endl;

    potato_msg = potato.serialize();
    send_str_with_header(socket_of_player(init_player, players), potato_msg);
  }

  read_fds = master;  // copy it
  select(fdmax + 1, &read_fds, NULL, NULL, NULL);

  for (int i = 0; i <= fdmax; i++) {
    if (FD_ISSET(i, &read_fds)) {
      recv_str_with_header(i, potato_msg);
      break;
    }
  }

  Potato p = Potato::deserialize(potato_msg);
  if (p.get_hops() == 0) {
    cout << "Trace of potato:" << endl;
    p.print_trace();
    // cout << "Game over" << endl;
    send_end_msg(players);
  }

  // close all sockets and exit
  close(listener);
  return EXIT_SUCCESS;
}