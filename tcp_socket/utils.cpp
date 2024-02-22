#include "utils.hpp"

#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <iostream>

using namespace std;

Player::Player(int id, int socket, struct sockaddr_storage addr)
    : id(id), socket(socket), addr(addr), server_port("") {}

int Player::get_id() { return id; }
int Player::get_socket() { return socket; }
struct sockaddr_storage Player::get_addr() { return addr; }
string Player::get_server_port() { return server_port; }
void Player::set_server_port(string port) { server_port = port; }

void print_string_details(const std::string& str) {
    for (char c : str) {
        std::cout << "Character: " << c << ", ASCII: " << static_cast<int>(c) << std::endl;
    }
}

int socket_build(const char *nodename, const char *servname,
                 const string side) {
  int sockfd;   // Listening socket descriptor
  int yes = 1;  // For setsockopt() SO_REUSEADDR, below
  int rv;

  struct addrinfo hints, *ai, *p;

  // Get us a socket and bind it
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if ((rv = getaddrinfo(nodename, servname, &hints, &ai)) != 0) {
    cerr << "getaddrinfo: " << gai_strerror(rv) << endl;
    exit(1);
  }

  for (p = ai; p != NULL; p = p->ai_next) {
    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockfd < 0) {
      continue;
    }

    // Lose the pesky "address already in use" error message
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if (side == "server" && (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0)) {
      close(sockfd);
      continue;
    }

    if (side == "player" && (connect(sockfd, p->ai_addr, p->ai_addrlen) < 0)) {
      close(sockfd);
      continue;
    }

    break;
  }

  // If we got here, it means we didn't get bound
  if (p == NULL) {
    return -1;
  }

  freeaddrinfo(ai);  // All done with this

  // Listen
  if (side == "server" && (listen(sockfd, 10) == -1)) {
    return -1;
  }

  return sockfd;
}

int which_player_socket(int socket, vector<Player> &players) {
  for (size_t i = 0; i < players.size(); i++) {
    if (players[i].get_socket() == socket) {
      return players[i].get_id();
    }
  }
  return -1;
}

int socket_of_player(int id, vector<Player> &players) {
  for (size_t i = 0; i < players.size(); i++) {
    if (players[i].get_id() == id) {
      return players[i].get_socket();
    }
  }
  return -1;
}

string serialize_addr(struct sockaddr_storage addr) {
  char ip_str[INET6_ADDRSTRLEN];
  std::string addr_str;

  if (addr.ss_family == AF_INET) {  // IPv4
    struct sockaddr_in *s = (struct sockaddr_in *)&addr;
    inet_ntop(AF_INET, &s->sin_addr, ip_str, sizeof(ip_str));
  } else {  // IPv6
    struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
    inet_ntop(AF_INET6, &s->sin6_addr, ip_str, sizeof(ip_str));
  }

  addr_str = std::string(ip_str);
  return addr_str;
}

bool send_all_str(int socket, const void *buffer, size_t length) {
  const char *ptr = (const char *)buffer;
  while (length > 0) {
    ssize_t i = send(socket, ptr, length, 0);
    if (i < 1) return false;  // Return false on send error
    ptr += i;                 // Move pointer past bytes sent
    length -= i;              // Decrease remaining bytes to send
  }
  return true;  // Return true when all data is sent
}

bool send_str_with_header(int socket, const std::string &message) {
  // Calculate message length
  uint32_t msg_length = htonl(message.length());  // Ensure network byte order

  // Send header (message length)
  if (!send_all_str(socket, &msg_length, sizeof(msg_length))) {
    cerr << "Failed to send message length." << endl;
    return false;
  }

  // Send actual message
  if (!send_all_str(socket, message.c_str(), message.length())) {
    cerr << "Failed to send message." << endl;
    return false;
  }

  return true;
}

bool recv_all_str(int socket, std::string &out_str, size_t length) {
  out_str.clear();
  out_str.reserve(length);
  std::vector<char> buffer(length);
  size_t total_received = 0;
  ssize_t n_received;

  while (total_received < length) {
    n_received = recv(socket, buffer.data() + total_received,
                      length - total_received, 0);
    if (n_received < 1) {
      // Handle errors or closed connection
      return false;
    }
    total_received += n_received;
  }

  out_str.assign(buffer.begin(), buffer.begin() + total_received);
  return true;
}

bool recv_str_with_header(int socket, std::string &out_str) {
  uint32_t msg_length;
  ssize_t n_received;

  // Receive the header
  n_received = recv(socket, &msg_length, sizeof(msg_length), 0);
  if (n_received < 1) return false;

  // Convert message length from network byte order to host byte order
  msg_length = ntohl(msg_length);

  // Now receive the actual message based on the length
  return recv_all_str(socket, out_str, msg_length);
}

int connect_player_serv(string player_addr) {
  // player_addr is in the form of "ip:port"
  // return the socket of the player
  size_t colon_pos = player_addr.find(":");
  if (colon_pos != string::npos) {
    string nodename = player_addr.substr(0, colon_pos);
    string servname = player_addr.substr(colon_pos + 1);

    return socket_build(nodename.c_str(), servname.c_str(), "player");
  }
  return -1;
}

string port_num_socket(int socket) {
  struct sockaddr_storage addr;
  socklen_t addr_size = sizeof(addr);

  int result = getsockname(socket, (struct sockaddr *)&addr, &addr_size);
  if (result == -1) {
    cerr << "Failed to get socket name." << endl;
    exit(EXIT_FAILURE);
  } else {
    if (addr.ss_family == AF_INET) {
      struct sockaddr_in *s = (struct sockaddr_in *)&addr;
      return to_string(ntohs(s->sin_port));
    } else {
      struct sockaddr_in6 *s = (struct sockaddr_in6 *)&addr;
      return to_string(ntohs(s->sin6_port));
    }
  }
  return "";
}