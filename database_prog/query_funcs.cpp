#include "query_funcs.h"

#include <sstream>
#include <string>

void exec(connection *C, string sql) {
  work W(*C);
  W.exec(sql);
  W.commit();
}

void add_player(connection *C, int team_id, int jersey_num, string first_name,
                string last_name, int mpg, int ppg, int rpg, int apg,
                double spg, double bpg) {
  stringstream ss;
  work W(*C);
  ss << "INSERT INTO PLAYER "
        "(team_id,uniform_num,first_name,last_name,mpg,ppg,rpg,apg,spg,bpg) "
        "VALUES ("
     << team_id << "," << jersey_num << "," << W.quote(first_name) << ","
     << W.quote(last_name) << "," << mpg << "," << ppg << "," << rpg << ","
     << apg << "," << spg << "," << bpg << ");";
  W.abort();
  exec(C, ss.str());
}

void add_team(connection *C, string name, int state_id, int color_id, int wins,
              int losses) {
  stringstream ss;
  work W(*C);
  ss << "INSERT INTO TEAM "
        "(name,state_id,color_id,wins,losses) "
        "VALUES ("
     << W.quote(name) << "," << state_id << "," << color_id << "," << wins
     << "," << losses << ");";
  W.abort();
  exec(C, ss.str());
}

void add_state(connection *C, string name) {
  stringstream ss;
  work W(*C);
  ss << "INSERT INTO STATE "
        "(name) "
        "VALUES ("
     << W.quote(name) << ");";
  W.abort();
  exec(C, ss.str());
}

void add_color(connection *C, string name) {
  stringstream ss;
  work W(*C);
  ss << "INSERT INTO COLOR "
        "(name) "
        "VALUES ("
     << W.quote(name) << ");";
  W.abort();
  exec(C, ss.str());
}

string generateWhereClause(const string &attribute, int use, double min,
                           double max) {
  if (use) {
    stringstream ss;
    ss << " AND " << attribute << " >= " << min << " AND " << attribute
       << " <= " << max;
    return ss.str();
  }
  return "";
}

template <typename T>
void printColumn(const result::const_iterator &row, int index) {
  cout << row[index].as<T>() << " ";
}

/*
 * All use_ params are used as flags for corresponding attributes
 * a 1 for a use_ param means this attribute is enabled (i.e. a WHERE clause is
 * needed) a 0 for a use_ param means this attribute is disabled
 */
void query1(connection *C, int use_mpg, int min_mpg, int max_mpg, int use_ppg,
            int min_ppg, int max_ppg, int use_rpg, int min_rpg, int max_rpg,
            int use_apg, int min_apg, int max_apg, int use_spg, double min_spg,
            double max_spg, int use_bpg, double min_bpg, double max_bpg) {
  stringstream ss;
  ss << "SELECT * FROM PLAYER WHERE 1=1";  // Always true condition to simplify
                                           // the code

  ss << generateWhereClause("mpg", use_mpg, min_mpg, max_mpg);
  ss << generateWhereClause("ppg", use_ppg, min_ppg, max_ppg);
  ss << generateWhereClause("rpg", use_rpg, min_rpg, max_rpg);
  ss << generateWhereClause("apg", use_apg, min_apg, max_apg);
  ss << generateWhereClause("spg", use_spg, min_spg, max_spg);
  ss << generateWhereClause("bpg", use_bpg, min_bpg, max_bpg);

  ss << ";";

  nontransaction N(*C);
  result R(N.exec(ss.str()));
  cout << "PLAYER_ID TEAM_ID UNIFORM_NUM FIRST_NAME LAST_NAME MPG PPG RPG "
          "APG SPG BPG"
       << endl;
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    printColumn<int>(c, 0);
    printColumn<int>(c, 1);
    printColumn<int>(c, 2);
    printColumn<string>(c, 3);
    printColumn<string>(c, 4);
    printColumn<int>(c, 5);
    printColumn<int>(c, 6);
    printColumn<int>(c, 7);
    printColumn<int>(c, 8);
    cout << fixed << setprecision(1);
    printColumn<double>(c, 9);
    printColumn<double>(c, 10);
    cout << endl;
  }
}

void query2(connection *C, string team_color) {
  work W(*C);
  stringstream ss;
  ss << "SELECT TEAM.NAME FROM TEAM, COLOR WHERE COLOR.COLOR_ID = "
        "TEAM.COLOR_ID AND COLOR.NAME = ";
  ss << W.quote(team_color);
  ss << ";";
  W.abort();
  nontransaction N(*C);
  result R(N.exec(ss.str()));
  cout << "NAME" << endl;
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    printColumn<string>(c, 0);
    cout << endl;
  }
}

void query3(connection *C, string team_name) {
  work W(*C);
  stringstream ss;
  ss << "SELECT FIRST_NAME, LAST_NAME FROM PLAYER, TEAM WHERE PLAYER.TEAM_ID = "
        "TEAM.TEAM_ID AND TEAM.NAME = ";
  ss << W.quote(team_name);
  ss << " ORDER BY PPG DESC";
  ss << ";";
  W.abort();
  nontransaction N(*C);
  result R(N.exec(ss.str()));
  cout << "FIRST_NAME LAST_NAME" << endl;
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    printColumn<string>(c, 0);
    printColumn<string>(c, 1);
    cout << endl;
  }
}

void query4(connection *C, string team_state, string team_color) {
  work W(*C);
  stringstream ss;
  ss << "SELECT FIRST_NAME, LAST_NAME, UNIFORM_NUM FROM PLAYER, TEAM, STATE, "
        "COLOR WHERE TEAM.STATE_ID = STATE.STATE_ID AND TEAM.COLOR_ID = "
        "COLOR.COLOR_ID AND PLAYER.TEAM_ID = TEAM.TEAM_ID AND COLOR.NAME = ";
  ss << W.quote(team_color);
  ss << " AND STATE.NAME = ";
  ss << W.quote(team_state);
  ss << ";";
  W.abort();
  nontransaction N(*C);
  result R(N.exec(ss.str()));
  cout << "FIRST_NAME LAST_NAME UNIFORM_NUM" << endl;
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    printColumn<string>(c, 0);
    printColumn<string>(c, 1);
    printColumn<int>(c, 2);
    cout << endl;
  }
}

void query5(connection *C, int num_wins) {
  stringstream ss;
  ss << "SELECT FIRST_NAME, LAST_NAME, TEAM.NAME, WINS FROM PLAYER, TEAM WHERE "
        "PLAYER.TEAM_ID = TEAM.TEAM_ID AND TEAM.WINS > ";
  ss << num_wins;
  ss << ";";
  nontransaction N(*C);
  result R(N.exec(ss.str()));
  cout << "FIRST_NAME LAST_NAME NAME WINS" << endl;
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    printColumn<string>(c, 0);
    printColumn<string>(c, 1);
    printColumn<string>(c, 2);
    printColumn<int>(c, 3);
    cout << endl;
  }
}
