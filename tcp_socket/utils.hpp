#ifndef __UTILS_POTATO__
#define __UTILS_POTATO__

#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <string>

#include "potato.hpp"

using namespace std;

class Player {
 private:
  int id;
  int socket;
  struct sockaddr_storage addr;
  string server_port;

 public:
  Player(int id, int socket, struct sockaddr_storage addr);

  int get_id();
  int get_socket();
  struct sockaddr_storage get_addr();
  string get_server_port();
  void set_server_port(string port);
};

void print_string_details(const std::string& str);
int socket_build(const char* nodename, const char* servname, const string side);
int which_player_socket(int socket, vector<Player>& players);
int socket_of_player(int id, vector<Player>& players);
string serialize_addr(struct sockaddr_storage addr);
bool send_all_str(int socket, const void* buffer, size_t length);
bool send_str_with_header(int socket, const std::string& message);
bool recv_all_str(int socket, std::string& outStr, size_t length);
bool recv_str_with_header(int socket, std::string& outStr);
int connect_player_serv(string player_addr);
string port_num_socket(int socket);

#endif