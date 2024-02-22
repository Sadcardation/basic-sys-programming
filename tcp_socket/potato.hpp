#include <vector>
#include <string>
#include <iostream>
#include <sstream>

using namespace std;
class Potato {
private:
  int hops;
  vector<int> trace;

public:
  Potato(int num_hops);
  ~Potato();
  Potato(const Potato &p);
  Potato &operator=(const Potato &p);
  int get_hops() const;
  void add_trace(int id);
  vector<int> get_trace() const;
  void print_trace() const;
  string serialize() const;
  static Potato deserialize(const std::string& s);
};