#include "potato.hpp"

Potato::Potato(int num_hops) : hops(num_hops) {}

Potato::~Potato() {}

Potato::Potato(const Potato &p) : hops(p.hops), trace(p.trace){}

Potato &Potato::operator=(const Potato &p) {
  if (this != &p) {
    hops = p.hops;
    trace = p.trace;
  }
  return *this;
}

int Potato::get_hops() const { return hops; }

void Potato::add_trace(int id) {
  trace.push_back(id);
  hops -= 1;
}

vector<int> Potato::get_trace() const { return trace; }

void Potato::print_trace() const {
  for (int id : trace) {
    std::cout << id;
    if (id != -1) {
      std::cout << ",";
    }
  }
  std::cout << std::endl;
}

string Potato::serialize() const {
  std::ostringstream oss;
  oss << hops << ' ';
  for (int id : trace) {
    oss << id << ' ';
  }
  return oss.str();
}

Potato Potato::deserialize(const std::string &s) {
  std::istringstream iss(s);
  int num_hops;
  iss >> num_hops;
  Potato p(num_hops);
  int id;
  while (iss >> id) {
    p.add_trace(id);
  }
  return p;
}