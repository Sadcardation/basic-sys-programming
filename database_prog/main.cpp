#include <fstream>
#include <iostream>
#include <pqxx/connection.hxx>
#include <pqxx/pqxx>

#include "exerciser.h"

using namespace std;
using namespace pqxx;

template <typename Func>
void processFile(connection* C, const std::string& filename, Func processLine) {
  std::ifstream ifs(filename);
  std::string line;
  while (std::getline(ifs, line)) {
    processLine(C, line);
  }
}

void loadState(connection* C, const std::string& line) {
  std::stringstream ss(line);
  std::string state_id, name;
  ss >> state_id >> name;
  add_state(C, name);
}

void loadColor(connection* C, const std::string& line) {
  std::stringstream ss(line);
  std::string color_id, name;
  ss >> color_id >> name;
  add_color(C, name);
}

void loadTeam(connection* C, const std::string& line) {
  std::stringstream ss(line);
  std::string team_id, name, state_id, color_id, wins, losses;
  ss >> team_id >> name >> state_id >> color_id >> wins >> losses;
  add_team(C, name, std::stoi(state_id), std::stoi(color_id), std::stoi(wins),
           std::stoi(losses));
}

void loadPlayer(connection* C, const std::string& line) {
  std::stringstream ss(line);
  std::string player_id, team_id, uniform_num, first_name, last_name, mpg, ppg,
      rpg, apg, spg, bpg;
  ss >> player_id >> team_id >> uniform_num >> first_name >> last_name >> mpg >>
      ppg >> rpg >> apg >> spg >> bpg;
  add_player(C, std::stoi(team_id), std::stoi(uniform_num), first_name,
             last_name, std::stoi(mpg), std::stoi(ppg), std::stoi(rpg),
             std::stoi(apg), std::stod(spg), std::stod(bpg));
}

void dropTables(connection* C) {
  exec(C, "DROP TABLE IF EXISTS PLAYER, TEAM, STATE, COLOR;");
}

void initTables(connection* C) {
  string STATE =
      "CREATE TABLE STATE ("
      "STATE_ID SERIAL PRIMARY KEY,"
      "NAME VARCHAR(100) NOT NULL"
      ");";

  string COLOR =
      "CREATE TABLE COLOR ("
      "COLOR_ID SERIAL PRIMARY KEY,"
      "NAME VARCHAR(100) NOT NULL"
      ");";

  string TEAM =
      "CREATE TABLE TEAM ("
      "TEAM_ID SERIAL PRIMARY KEY,"
      "NAME VARCHAR(100) NOT NULL,"
      "STATE_ID INT REFERENCES STATE(STATE_ID),"
      "COLOR_ID INT REFERENCES COLOR(COLOR_ID),"
      "WINS INT NOT NULL,"
      "LOSSES INT NOT NULL"
      ");";

  string PLAYER =
      "CREATE TABLE PLAYER ("
      "PLAYER_ID SERIAL PRIMARY KEY,"
      "TEAM_ID INT REFERENCES TEAM(TEAM_ID),"
      "UNIFORM_NUM INT NOT NULL,"
      "FIRST_NAME VARCHAR(100) NOT NULL,"
      "LAST_NAME VARCHAR(100) NOT NULL,"
      "MPG INT NOT NULL,"
      "PPG INT NOT NULL,"
      "RPG INT NOT NULL,"
      "APG INT NOT NULL,"
      "SPG DOUBLE PRECISION NOT NULL,"
      "BPG DOUBLE PRECISION NOT NULL"
      ");";

  exec(C, STATE);
  exec(C, COLOR);
  exec(C, TEAM);
  exec(C, PLAYER);
}

void loadData(connection* C) {
  processFile(C, "state.txt", loadState);
  processFile(C, "color.txt", loadColor);
  processFile(C, "team.txt", loadTeam);
  processFile(C, "player.txt", loadPlayer);
}

int main(int argc, char* argv[]) {
  // Allocate & initialize a Postgres connection object
  connection* C;

  try {
    // Establish a connection to the database
    // Parameters: database name, user name, user password
    C = new connection("dbname=ACC_BBALL user=postgres password=passw0rd");
    if (C->is_open()) {
      cout << "Opened database successfully: " << C->dbname() << endl;
    } else {
      cout << "Can't open database" << endl;
      return 1;
    }
  } catch (const std::exception& e) {
    cerr << e.what() << std::endl;
    return 1;
  }

  // TODO: create PLAYER, TEAM, STATE, and COLOR tables in the ACC_BBALL
  // database
  //       load each table with rows from the provided source txt files
  dropTables(C);
  initTables(C);
  loadData(C);

  exercise(C);

  // Close database connection
  C->disconnect();

  return 0;
}
